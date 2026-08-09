use super::thread::Thread;
use crate::arch::Context;
use crate::arch::{self, PROCESSORS, Prcb};
use crate::sched::thread::ThreadState;
use alloc::boxed::Box;
use alloc::collections::BTreeMap;

const NICE_0_LOAD: usize = 1024;

// weight(nice) = 1024 / (1.25 ** nice)
const WEIGHT_TABLE: [usize; 40] = [
    88761, 71755, 56483, 46273, 36291, 29154, 23254, 18705, 14949, 11916, 9548, 7620, 6100, 4904,
    3906, 3121, 2501, 1991, 1586, 1277, 1024, 820, 655, 526, 423, 335, 272, 215, 172, 137, 110, 87,
    70, 56, 45, 36, 29, 23, 18, 15,
];

fn niceness_to_weight(niceness: i8) -> usize {
    let idx = (niceness as i32 + 20).clamp(0, 39) as usize;
    WEIGHT_TABLE[idx]
}

pub struct RunQueue {
    ready: BTreeMap<(usize, usize), Box<Thread>>,
}

impl RunQueue {
    pub fn new() -> Self {
        Self {
            ready: BTreeMap::new(),
        }
    }

    pub fn enqueue(&mut self, mut thread: Box<Thread>) {
        self.ready.insert((thread.vruntime, thread.id), thread);
    }

    pub fn enqueue_new(&mut self, mut thread: Box<Thread>) {
        let least_vruntime = self
            .ready
            .first_key_value()
            .map(|(&(vr, _), _)| vr)
            .unwrap_or(0);

        thread.vruntime = least_vruntime;
        self.enqueue(thread);
    }

    fn get_next(&mut self) -> Option<Box<Thread>> {
        self.ready.pop_first().map(|(_, thread)| thread)
    }

    pub fn len(&self) -> usize {
        self.ready.len()
    }
}

pub fn enqueue_thread(thread: Box<Thread>) {
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

pub fn schedule(context: Context) -> Option<&'static mut Thread> {
    let prcb = arch::get_current_processor();
    let mut prcb = unsafe { &mut *prcb };

    let mut run_queue = prcb.run_queue.lock();

    let Some(mut next) = run_queue.get_next() else {
        return None;
    };

    if let Some(mut running_thread) = prcb.running_thread.take() {
        running_thread.context = context;

        let now = arch::time::now_ns();
        let time_thread_ran_for = now - running_thread.last_scheduled_at;
        let weight = niceness_to_weight(running_thread.niceness);
        running_thread.vruntime += (time_thread_ran_for * NICE_0_LOAD) / weight;

        let state = {
            let mut thread_state = running_thread.state.lock();
            if *thread_state == ThreadState::Running {
                *thread_state = ThreadState::Ready;
            }
            *thread_state
        };
        if state == ThreadState::Ready {
            run_queue.enqueue(running_thread);
        }
    }

    *next.state.lock() = ThreadState::Running;
    next.last_scheduled_at = arch::time::now_ns();
    prcb.running_thread = Some(next);
    prcb.running_thread.as_deref_mut()
}
