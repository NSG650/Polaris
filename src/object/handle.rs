use crate::object::{KernelObject, PSEUDO_HANDLE_CURRENT_PROCESS, PSEUDO_HANDLE_CURRENT_THREAD};
use crate::sched::dispatch::DispatcherObject;
use crate::status_codes::{PxResult, PxStatus};
use alloc::collections::BTreeMap;
use alloc::sync::Arc;

#[derive(Clone)]
pub struct Handle {
    object: KernelObject,
}

pub struct HandleTable {
    handles: BTreeMap<isize, Handle>,
    next_id: isize,
}

impl HandleTable {
    pub fn new() -> Self {
        Self {
            handles: BTreeMap::new(),
            next_id: 4,
        }
    }

    pub fn insert(&mut self, handle: Handle) -> isize {
        let r = self.next_id;
        self.handles.insert(self.next_id, handle);
        self.next_id += 4;
        r
    }

    pub fn remove(&mut self, handle_id: isize) -> Option<Handle> {
        self.handles.remove(&handle_id)
    }

    pub fn get(&mut self, handle_id: isize) -> Option<&Handle> {
        self.handles.get(&handle_id)
    }
}

impl Handle {
    pub fn new(object: KernelObject) -> Self {
        Self { object }
    }

    pub fn get(&self) -> KernelObject {
        self.object.clone()
    }
}

pub fn clone_handles(
    handle_table_from: &HandleTable,
    handle_table_to: &mut HandleTable,
) -> PxResult<()> {
    for (handle_id, handle) in handle_table_from.handles.iter() {
        let mut h = handle.clone();
        handle_table_to.handles.insert(*handle_id, h);
    }

    Ok(())
}

pub fn get_object(handle_id: isize) -> PxResult<KernelObject> {
    let running_thread =
        crate::arch::get_running_thread().expect("get_object called with no thread running??");

    if handle_id == PSEUDO_HANDLE_CURRENT_PROCESS {
        let running_proc = running_thread.mother_proc.clone();
        return Ok(KernelObject::Process(running_proc));
    }

    if handle_id == PSEUDO_HANDLE_CURRENT_THREAD {
        return Ok(KernelObject::Thread(running_thread.clone()));
    }

    let object = {
        let mut table = running_thread.mother_proc.handle_table.lock();
        table.get(handle_id).ok_or(PxStatus::NotFound)?.get()
    };
    Ok(object)
}

pub fn get_object_as<T: TryFrom<KernelObject, Error = PxStatus>>(handle_id: isize) -> PxResult<T> {
    T::try_from(get_object(handle_id)?)
}

pub fn close(handle_id: isize) -> PxResult<()> {
    if handle_id == PSEUDO_HANDLE_CURRENT_PROCESS || handle_id == PSEUDO_HANDLE_CURRENT_THREAD {
        return Err(PxStatus::Unsuccessful);
    }

    let running_thread =
        crate::arch::get_running_thread().expect("close called with no thread running??");
    let mut table = running_thread.mother_proc.handle_table.lock();

    table.remove(handle_id).ok_or(PxStatus::NotFound)?;

    Ok(())
}

pub fn syscall_close(handle_id: isize) -> PxStatus {
    close(handle_id).err().unwrap_or(PxStatus::Success)
}
