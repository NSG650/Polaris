use crate::mm::stack::*;
use crate::{arch::Context, locks::spinlock::SpinLock};
use core::sync::atomic::{AtomicUsize, Ordering};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ThreadState {
    Ready,
    Running,
    Waiting,
    Terminating,
    Terminated,
}

static CURRENT_THREAD_ID: AtomicUsize = AtomicUsize::new(1);

pub struct Thread {
    pub id: usize,
    pub vruntime: usize,
    pub niceness: i8,
    pub context: Context,
    pub state: SpinLock<ThreadState>,
    pub last_scheduled_at: usize,
    kernel_stack: KernelStack,
}

impl Thread {
    pub fn new_kernel(entry: extern "C" fn(usize), arg: usize) -> Option<Self> {
        let kernel_stack = KernelStack::new();

        let kernel_stack = match kernel_stack {
            None => return None,
            Some(ks) => ks,
        };

        let sp = kernel_stack.top();

        Some(Self {
            id: CURRENT_THREAD_ID.fetch_add(1, Ordering::SeqCst),
            vruntime: 0,
            niceness: 0,
            state: SpinLock::new(ThreadState::Ready),
            context: Context::init_kernel(entry as usize, sp, arg),
            last_scheduled_at: 0,
            kernel_stack,
        })
    }

    pub fn terminate(&mut self) {}
}

pub extern "C" fn idle_thread(_: usize) {
    loop {}
}
