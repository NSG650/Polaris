use super::dispatch::Dispatcher;
use super::thread::Thread;
use crate::locks::mutex::Mutex;
use crate::mm::virt::{self, AddressSpace, KERNEL_ADDRESS_SPACE};
use crate::object::handle::{self, Handle, HandleTable};
use crate::object::{KernelObject, PSEUDO_HANDLE_CURRENT_PROCESS};
use crate::status_codes::{PxResult, PxStatus};
use crate::syscall::FromArg;
use crate::{locks::spinlock::SpinLock, sched::thread::ThreadState};
use alloc::collections::BTreeMap;
use alloc::sync::Arc;
use alloc::vec::Vec;
use bitflags::bitflags;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{AtomicUsize, Ordering};
use spin::{Once, Spin};

static NEXT_PROCESS_ID: AtomicUsize = AtomicUsize::new(0);
static PROCESS_LIST: SpinLock<BTreeMap<usize, Arc<Process>>> = SpinLock::new(BTreeMap::new());

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProcessState {
    Normal,
    Terminated,
}

pub struct Process {
    pub id: usize,
    pub threads: SpinLock<Vec<Arc<Thread>>>,
    pub state: SpinLock<ProcessState>,
    pub handle_table: Mutex<HandleTable>,
    pub address_space: Arc<SpinLock<AddressSpace>>,
}

impl Process {
    pub fn new() -> Arc<Self> {
        let proc = Arc::new(Self {
            id: NEXT_PROCESS_ID.fetch_add(1, Ordering::Relaxed),
            threads: SpinLock::new(Vec::new()),
            state: SpinLock::new(ProcessState::Normal),
            handle_table: Mutex::new(HandleTable::new()),
            address_space: Arc::new(SpinLock::new(AddressSpace::new())),
        });
        PROCESS_LIST.lock().insert(proc.id, proc.clone());
        proc
    }

    pub fn new_given_address_space(address_space: Arc<SpinLock<AddressSpace>>) -> Arc<Self> {
        let proc = Arc::new(Self {
            id: NEXT_PROCESS_ID.fetch_add(1, Ordering::Relaxed),
            threads: SpinLock::new(Vec::new()),
            state: SpinLock::new(ProcessState::Normal),
            handle_table: Mutex::new(HandleTable::new()),
            address_space,
        });
        PROCESS_LIST.lock().insert(proc.id, proc.clone());
        proc
    }

    pub fn add_thread(&self, thread: Arc<Thread>) {
        self.threads.lock().push(thread);
    }

    pub fn remove_thread(&self, thread: Arc<Thread>) {
        let mut threads = self.threads.lock();
        if let Some(index) = threads.iter().position(|x| Arc::ptr_eq(&thread, x)) {
            threads.swap_remove(index);
        }
    }

    pub fn terminate(&self) {
        let threads = &*self.threads.lock();
        for thread in threads {
            *thread.state.lock() = ThreadState::Terminating;
        }

        *self.state.lock() = ProcessState::Terminated;
        PROCESS_LIST.lock().remove(&self.id);
    }
}

impl Dispatcher for Process {
    fn test(&self) -> bool {
        *self.state.lock() == ProcessState::Terminated
    }
}

static KERNEL_PROCESS: Once<Arc<Process>> = Once::new();

pub fn init() {
    KERNEL_PROCESS.call_once(|| {
        Process::new_given_address_space(
            KERNEL_ADDRESS_SPACE
                .get()
                .expect("Kernel address space is still not setup??")
                .clone(),
        )
    });
}

pub fn kernel_process() -> &'static Arc<Process> {
    KERNEL_PROCESS
        .get()
        .expect("Kernel Process was not created??")
}

pub fn syscall_new_process(process_handle: *mut isize) -> PxStatus {
    if process_handle.is_null() {
        return PxStatus::BufferTooSmall;
    }

    let running_proc = crate::arch::get_running_thread()
        .unwrap()
        .mother_proc
        .clone();

    let new_proc = Process::new();

    new_proc.address_space.lock().setup_for_user();

    let handle = running_proc
        .handle_table
        .lock()
        .insert(Handle::new(KernelObject::Process(new_proc)));

    unsafe {
        *process_handle = handle;
    }

    PxStatus::Success
}

pub fn syscall_terminate_process(process_handle: isize) -> PxStatus {
    let process = match handle::get_object_as::<Arc<Process>>(process_handle) {
        Ok(p) => p,
        Err(e) => {
            return e;
        }
    };

    process.terminate();

    if process_handle == PSEUDO_HANDLE_CURRENT_PROCESS {
        let running_thread = crate::arch::get_running_thread().unwrap();
        running_thread.terminate();
        unreachable!();
    }

    PxStatus::Success
}

bitflags! {
    #[derive(Clone, Copy, Eq, PartialEq)]
    pub struct ProcessCloneFlags: i32 {
        const COPY_ADDRESS_SPACE = 1 << 0;
        const COPY_HANDLES = 1 << 1;
    }
}

impl FromArg for ProcessCloneFlags {
    fn from_arg(r: usize) -> PxResult<Self> {
        let bits = i32::try_from(r).map_err(|_| PxStatus::InvalidArguments)?;
        Self::from_bits(bits).ok_or(PxStatus::InvalidArguments)
    }
}

pub fn syscall_clone(
    process_handle_from: isize,
    process_handle_to: isize,
    flags: ProcessCloneFlags,
) -> PxStatus {
    let running_proc = crate::arch::get_running_thread()
        .unwrap()
        .mother_proc
        .clone();

    let process_from = match handle::get_object_as::<Arc<Process>>(process_handle_from) {
        Ok(p) => p,
        Err(e) => {
            return e;
        }
    };

    let process_to = match handle::get_object_as::<Arc<Process>>(process_handle_to) {
        Ok(p) => p,
        Err(e) => {
            return e;
        }
    };

    if Arc::ptr_eq(&process_from, &process_to) {
        return PxStatus::AlreadyExists;
    }

    if Arc::ptr_eq(&process_to, &running_proc) {
        return PxStatus::InvalidArguments;
    }

    if flags.contains(ProcessCloneFlags::COPY_ADDRESS_SPACE) {
        let mut address_space_from = process_from.address_space.lock();
        let address_space_from = address_space_from.deref();

        let mut address_space_to = process_to.address_space.lock();
        let address_space_to = &mut address_space_to.deref_mut();

        if let Err(e) = virt::clone_address_space(address_space_from, address_space_to) {
            return e;
        }
    }

    if flags.contains(ProcessCloneFlags::COPY_HANDLES) {
        let mut handle_table_from = process_from.handle_table.lock();
        let handle_table_from = handle_table_from.deref();

        let mut handle_table_to = process_to.handle_table.lock();
        let handle_table_to = handle_table_to.deref_mut();

        if let Err(e) = handle::clone_handles(handle_table_from, handle_table_to) {
            return e;
        }
    }

    PxStatus::Success
}
