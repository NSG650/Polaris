use crate::locks::mutex::Mutex;
use crate::object::handle::{self, Handle};
use crate::object::{self, KernelObject};
use crate::sched::dispatch::{Dispatcher, DispatcherObject, Event, wait_on_single_object};
use crate::status_codes::{PxResult, PxStatus};
use alloc::boxed::Box;
use alloc::collections::VecDeque;
use alloc::sync::{Arc, Weak};
use alloc::vec::Vec;
use core::slice;
use spin::once;

pub struct Message {
    data: Box<[u8]>,
    objects: Vec<KernelObject>,
}

impl Message {
    fn new(data: &[u8], objects: &[KernelObject]) -> Self {
        Message {
            data: Box::from(data),
            objects: objects.to_vec(),
        }
    }
}

struct Inner {
    messages: VecDeque<Message>,
    peer: Weak<Channel>,
}

pub struct Channel {
    inner: Mutex<Inner>,
    event: Arc<Event>,
}

impl Channel {
    pub fn new() -> Self {
        Self {
            inner: Mutex::new(Inner {
                messages: VecDeque::new(),
                peer: Weak::new(),
            }),
            event: Arc::new(Event::new()),
        }
    }

    pub fn new_pair() -> (Arc<Channel>, Arc<Channel>) {
        let (a, b) = (Arc::new(Channel::new()), Arc::new(Channel::new()));
        a.inner.lock().peer = Arc::downgrade(&b);
        b.inner.lock().peer = Arc::downgrade(&a);
        (a, b)
    }

    pub fn send(&self, data: &[u8], objects: &[KernelObject]) -> PxResult<()> {
        let msg = Message::new(data, objects);

        let peer = self.inner.lock().peer.upgrade();
        let Some(peer) = peer else {
            return Err(PxStatus::PeerClosed);
        };

        let mut g = peer.inner.lock();
        g.messages.push_back(msg);

        peer.event.trigger(true);
        Ok(())
    }

    pub fn try_receive(
        &self,
        buffer: &mut [u8],
        max_object_count: usize,
    ) -> PxResult<(usize, Vec<KernelObject>)> {
        let msg = {
            let mut g = self.inner.lock();

            let Some(front) = g.messages.front() else {
                return Err(if g.peer.strong_count() == 0 {
                    PxStatus::PeerClosed
                } else {
                    PxStatus::ShouldWait
                });
            };

            if buffer.len() < front.data.len() {
                return Err(PxStatus::BufferTooSmall);
            }

            if max_object_count > 0 && max_object_count < front.objects.len() {
                return Err(PxStatus::BufferTooSmall);
            }

            let m = g.messages.pop_front().unwrap();

            if g.messages.is_empty() && g.peer.strong_count() != 0 {
                self.event.trigger(false);
            }
            m
        };

        let len = msg.data.len();
        buffer[..len].copy_from_slice(&msg.data);

        Ok((len, msg.objects))
    }

    pub fn receive(
        self: &Arc<Self>,
        buffer: &mut [u8],
        max_object_count: usize,
    ) -> PxResult<(usize, Vec<KernelObject>)> {
        loop {
            match self.try_receive(buffer, max_object_count) {
                Err(PxStatus::ShouldWait) => {
                    let r = wait_on_single_object(Arc::new(self.event.clone().into()), usize::MAX);
                    if r != PxStatus::Success {
                        return Err(r);
                    }
                }
                r => return r,
            }
        }
    }
}

impl Drop for Channel {
    fn drop(&mut self) {
        let (peer, queued) = {
            let mut g = self.inner.lock();
            (g.peer.upgrade(), core::mem::take(&mut g.messages))
        };
        if let Some(peer) = peer {
            let _g = peer.inner.lock();
            peer.event.trigger(true);
        }
        drop(queued);
    }
}

pub fn syscall_new_channel_pair(handle_ids: *mut isize) -> PxStatus {
    let running_proc = crate::arch::get_running_thread()
        .unwrap()
        .mother_proc
        .clone();

    let channel_pair = Channel::new_pair();
    let handle_pair = (
        Handle::new(KernelObject::Channel(channel_pair.0)),
        Handle::new(KernelObject::Channel(channel_pair.1)),
    );

    let mut handle_table = running_proc.handle_table.lock();

    let handle_id_pair = (
        handle_table.insert(handle_pair.0),
        handle_table.insert(handle_pair.1),
    );

    unsafe {
        *handle_ids = handle_id_pair.0;
        *(handle_ids.add(1)) = handle_id_pair.1;
    }

    PxStatus::Success
}

pub fn syscall_channel_send(
    channel_handle: isize,
    buffer: *const u8,
    length: usize,
    handles: *const isize,
    handle_count: usize,
) -> PxStatus {
    let channel = match handle::get_object_as::<Arc<Channel>>(channel_handle) {
        Ok(c) => c,
        Err(e) => {
            return e;
        }
    };

    let data: &[u8] = unsafe { slice::from_raw_parts(buffer, length) };

    let mut objects = Vec::new();
    if !handles.is_null() {
        let ids: &[isize] = unsafe { slice::from_raw_parts(handles, handle_count) };
        for id in ids {
            match handle::get_object(*id) {
                Ok(o) => objects.push(o),
                Err(e) => return e,
            }
        }
    }

    channel
        .send(data, &objects)
        .err()
        .unwrap_or(PxStatus::Success)
}

pub fn syscall_channel_receive(
    channel_handle: isize,
    buffer: *mut u8,
    length: *mut usize,
    handles: *mut isize,
    handle_count: *mut usize,
) -> PxStatus {
    let running_proc = crate::arch::get_running_thread()
        .unwrap()
        .mother_proc
        .clone();

    let channel = match handle::get_object_as::<Arc<Channel>>(channel_handle) {
        Ok(c) => c,
        Err(e) => return e,
    };

    let (buf_len, max_handles) =
        unsafe { (*length, if handles.is_null() { 0 } else { *handle_count }) };
    let data: &mut [u8] = unsafe { slice::from_raw_parts_mut(buffer, buf_len) };

    match channel.receive(data, max_handles) {
        Ok((len, objects)) => {
            unsafe { *length = len };

            if !handles.is_null() {
                let count = objects.len();
                let mut table = running_proc.handle_table.lock();
                for (i, object) in objects.into_iter().enumerate() {
                    unsafe { *handles.add(i) = table.insert(Handle::new(object)) };
                }
                unsafe { *handle_count = count };
            }
            PxStatus::Success
        }
        Err(e) => e,
    }
}
