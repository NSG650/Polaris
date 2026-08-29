use crate::sched::dispatch::DispatcherObject;
use alloc::collections::BTreeMap;
use alloc::sync::Arc;

pub struct Handle {
    object: Arc<DispatcherObject>,
}

pub struct HandleTable {
    handles: BTreeMap<usize, Handle>,
    next_id: usize,
}

impl HandleTable {
    pub fn new() -> Self {
        Self {
            handles: BTreeMap::new(),
            next_id: 4,
        }
    }

    pub fn insert(&mut self, handle: Handle) -> usize {
        let r = self.next_id;
        self.handles.insert(self.next_id, handle);
        self.next_id += 4;
        r
    }

    pub fn get(&mut self, handle_id: usize) -> Option<&Handle> {
        self.handles.get(&handle_id)
    }
}

impl Handle {
    pub fn new(object: Arc<DispatcherObject>) -> Self {
        Self { object }
    }

    pub fn get(&self) -> Arc<DispatcherObject> {
        self.object.clone()
    }
}
