use super::thread::{Thread, ThreadAdapter, ThreadState};
use crate::arch::Context;
use crate::arch::{self, PROCESSORS, Prcb};
use alloc::sync::Arc;
use core::sync::atomic::{AtomicUsize, Ordering};
use intrusive_collections::{RBTree, UnsafeRef};

const NICE_0_LOAD: usize = 1024;

// Did you know 1.25 is 5/4 !
const fn wpow(exp: i32) -> (u64, u64) {
    let n: u64 = 5;
    let d: u64 = 4;
    if exp < 0 {
        return (d.pow(-exp as u32), n.pow(-exp as u32));
    }
    (n.pow(exp as u32), d.pow(exp as u32))
}

// weight(nice) = NICE_0_LOAD / (1.25 ** nice)
const fn weight(nice: i8) -> usize {
    let powwow = wpow(nice as i32);
    (((NICE_0_LOAD as u64) * powwow.1) / powwow.0) as usize
}

const fn build_weight_table() -> [usize; 40] {
    let mut table = [0usize; 40];
    let mut i = 0;
    while i < 40 {
        table[i] = weight((i as i32 - 20) as i8);
        i += 1;
    }
    table
}

const WEIGHT_TABLE: [usize; 40] = build_weight_table();

fn niceness_to_weight(niceness: i8) -> usize {
    let idx = (niceness as i32 + 20).clamp(0, 39) as usize;
    WEIGHT_TABLE[idx]
}

fn into_link(thread: Arc<Thread>) -> UnsafeRef<Thread> {
    unsafe { UnsafeRef::from_raw(Arc::into_raw(thread)) }
}

fn from_link(node: UnsafeRef<Thread>) -> Arc<Thread> {
    unsafe { Arc::from_raw(UnsafeRef::into_raw(node)) }
}

pub struct RunQueue {
    ready: RBTree<ThreadAdapter>,
    waiting: RBTree<ThreadAdapter>,
    count: AtomicUsize,
    waiting_count: AtomicUsize,
}

impl RunQueue {
    pub fn new() -> Self {
        Self {
            ready: RBTree::new(ThreadAdapter::new()),
            waiting: RBTree::new(ThreadAdapter::new()),
            count: AtomicUsize::new(0),
            waiting_count: AtomicUsize::new(0),
        }
    }

    pub fn enqueue(&mut self, thread: Arc<Thread>) {
        if thread
            .queued
            .compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire)
            .is_err()
        {
            return;
        }
        self.ready.insert(into_link(thread));
        self.count.fetch_add(1, Ordering::Release);
    }

    pub fn enqueue_new(&mut self, thread: Arc<Thread>) {
        let least_vruntime = self
            .ready
            .front()
            .get()
            .map(|t| t.vruntime.load(Ordering::Relaxed))
            .unwrap_or(0);

        thread.vruntime.store(least_vruntime, Ordering::Relaxed);
        self.enqueue(thread);
    }

    pub fn enqueue_waiting(&mut self, thread: Arc<Thread>) {
        if thread
            .queued
            .compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire)
            .is_err()
        {
            return;
        }
        self.waiting.insert(into_link(thread));
        self.waiting_count.fetch_add(1, Ordering::Release);
    }

    fn wake_up_waiters(&mut self) {
        let mut cursor = self.waiting.front_mut();
        while let Some(thread) = cursor.get() {
            let (waiting_on_all, wait_time_out) = {
                let ws = thread.wait_state.lock();
                (ws.waiting_on_all, ws.wait_time_out)
            };

            if thread.test_objects(waiting_on_all).is_some()
                || (wait_time_out != usize::MAX && wait_time_out <= arch::time::now_ns())
            {
                let thread = from_link(cursor.remove().unwrap());
                thread.queued.store(false, Ordering::Release);

                self.waiting_count.fetch_sub(1, Ordering::Release);
                *thread.state.lock() = ThreadState::Ready;

                self.ready.insert(into_link(thread));
                self.count.fetch_add(1, Ordering::Release);
            } else {
                cursor.move_next();
            }
        }
    }

    fn get_next(&mut self) -> Option<Arc<Thread>> {
        let node = self.ready.front_mut().remove()?;
        let thread = from_link(node);
        thread.queued.store(false, Ordering::Release);
        self.count.fetch_sub(1, Ordering::Release);
        Some(thread)
    }

    fn len(&self) -> usize {
        self.count.load(Ordering::Acquire) + self.waiting_count.load(Ordering::Acquire)
    }
}

pub fn enqueue_thread(thread: Arc<Thread>) {
    let processors = PROCESSORS.lock();

    let mut best: Option<&Prcb> = None;
    let mut smallest = usize::MAX;

    for prcb in processors.iter().filter_map(|p| *p) {
        let len = prcb.run_queue.lock().len();
        if len < smallest {
            smallest = len;
            best = Some(prcb);
        }
    }

    best.expect("No processors available??")
        .run_queue
        .lock()
        .enqueue_new(thread);
}

pub fn yield_execution() {
    let running_thread =
        arch::get_running_thread().expect("yield_execution called with no thread running??");

    {
        let mut thread_state = running_thread.state.lock();
        if *thread_state == ThreadState::Running {
            *thread_state = ThreadState::Ready;
        }
    }

    while *running_thread.state.lock() != ThreadState::Running {
        arch::request_yield();
    }
}

pub fn schedule(context: Context) -> Option<&'static Thread> {
    let prcb = arch::get_current_processor();
    let mut prcb = unsafe { &mut *prcb };

    let mut run_queue = prcb.run_queue.lock();
    run_queue.wake_up_waiters();

    let next = run_queue.get_next()?;

    if let Some(running_thread) = prcb.running_thread.take() {
        unsafe {
            *running_thread.context_mut() = context;
        }

        let now = arch::time::now_ns();
        let time_thread_ran_for = now - running_thread.last_scheduled_at.load(Ordering::Relaxed);
        let weight = niceness_to_weight(running_thread.niceness.load(Ordering::Relaxed));
        running_thread.vruntime.fetch_add(
            (time_thread_ran_for * NICE_0_LOAD) / weight,
            Ordering::Relaxed,
        );

        let state = {
            let mut thread_state = running_thread.state.lock();
            if *thread_state == ThreadState::Running {
                *thread_state = ThreadState::Ready;
            }
            *thread_state
        };

        match state {
            ThreadState::Ready => run_queue.enqueue(running_thread),
            ThreadState::Waiting => run_queue.enqueue_waiting(running_thread),
            ThreadState::Terminating | ThreadState::Terminated => {}
            ThreadState::Running => unreachable!(),
        }
    }

    *next.state.lock() = ThreadState::Running;
    next.last_scheduled_at
        .store(arch::time::now_ns(), Ordering::Relaxed);
    prcb.running_thread = Some(next);
    prcb.running_thread.as_deref()
}
