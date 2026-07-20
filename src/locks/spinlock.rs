use crate::arch;
use core::ops::{Deref, DerefMut};
use core::{
    cell::UnsafeCell,
    sync::atomic::{AtomicBool, Ordering},
};

pub struct SpinLock<T> {
    locked: AtomicBool,
    data: UnsafeCell<T>,
}

unsafe impl<T: Send> Sync for SpinLock<T> {}

pub struct SpinLockGuard<'a, T> {
    lock: &'a SpinLock<T>,
    intr_enabled: bool,
}

impl<T> SpinLock<T> {
    pub const fn new(data: T) -> Self {
        Self {
            locked: AtomicBool::new(false),
            data: UnsafeCell::new(data),
        }
    }

    pub fn lock(&self) -> SpinLockGuard<'_, T> {
        let was_enabled = unsafe { arch::intr::get_interrupt_state() };
        unsafe { arch::intr::disable_interrupts() };

        while self
            .locked
            .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_err()
        {
            while self.locked.load(Ordering::Relaxed) {
                core::hint::spin_loop();
            }
        }

        SpinLockGuard {
            lock: self,
            intr_enabled: was_enabled,
        }
    }

    pub fn try_lock(&self) -> Option<SpinLockGuard<'_, T>> {
        let was_enabled = unsafe { arch::intr::get_interrupt_state() };
        unsafe { arch::intr::disable_interrupts() };

        let acquired = self
            .locked
            .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_ok();

        if acquired {
            Some(SpinLockGuard {
                lock: self,
                intr_enabled: was_enabled,
            })
        } else {
            if was_enabled {
                unsafe { arch::intr::enable_interrupts() };
            }
            None
        }
    }
}

impl<T> Deref for SpinLockGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.lock.data.get() }
    }
}

impl<T> DerefMut for SpinLockGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.lock.data.get() }
    }
}

impl<T> Drop for SpinLockGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.locked.store(false, Ordering::Release);
        if self.intr_enabled {
            unsafe { arch::intr::enable_interrupts() };
        }
    }
}
