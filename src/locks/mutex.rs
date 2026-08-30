use crate::sched::dispatch::{Dispatcher, DispatcherObject, Event, wait_on_single_object};
use alloc::sync::Arc;
use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{AtomicBool, Ordering};

pub struct KMutex {
    locked: AtomicBool,
}

impl KMutex {
    pub fn new() -> Self {
        Self {
            locked: AtomicBool::new(false),
        }
    }

    pub fn release(&self) {
        self.locked.store(false, Ordering::Release);
    }
}

impl Dispatcher for KMutex {
    fn test(&self) -> bool {
        self.locked
            .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
    }
}

pub struct Mutex<T> {
    kmutex: Arc<KMutex>,
    kmutex_obj: Arc<DispatcherObject>,
    data: UnsafeCell<T>,
}

unsafe impl<T: Send> Sync for Mutex<T> {}

pub struct MutexGuard<'a, T> {
    lock: &'a Mutex<T>,
}

impl<T> Mutex<T> {
    pub fn new(data: T) -> Self {
        let kmutex = Arc::new(KMutex::new());
        let kmutex_obj = Arc::new(kmutex.clone().into());

        Self {
            kmutex,
            kmutex_obj,
            data: UnsafeCell::new(data),
        }
    }

    pub fn lock(&self) -> MutexGuard<'_, T> {
        if !self.kmutex.test() {
            wait_on_single_object(self.kmutex_obj.clone(), usize::MAX);
        }
        MutexGuard { lock: self }
    }

    pub fn try_lock(&self) -> Option<MutexGuard<'_, T>> {
        if !self.kmutex.test() {
            return None;
        }
        Some(MutexGuard { lock: self })
    }
}

impl<T> Deref for MutexGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.lock.data.get() }
    }
}

impl<T> DerefMut for MutexGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.lock.data.get() }
    }
}

impl<T> Drop for MutexGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.kmutex.release();
    }
}
