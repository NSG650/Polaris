use super::dispatch::Dispatcher;
use super::thread::Thread;
use crate::locks::mutex::Mutex;
use crate::mm::virt::{AddressSpace, KERNEL_ADDRESS_SPACE};
use crate::object::handle::{self, Handle, HandleTable};
use crate::{locks::spinlock::SpinLock, sched::thread::ThreadState};
use alloc::collections::BTreeMap;
use alloc::sync::Arc;
use alloc::vec::Vec;
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
