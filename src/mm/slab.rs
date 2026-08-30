use core::alloc::{GlobalAlloc, Layout};
use core::cell::Cell;
use core::mem::size_of;
use core::ptr::null_mut;

use intrusive_collections::{LinkedList, LinkedListLink, UnsafeRef, intrusive_adapter};

use crate::arch::{BIG_ALLOC_START, PAGE_SHIFT, PAGE_SIZE, PteFlags};
use crate::locks::spinlock::SpinLock;
use crate::mm::phys::*;
use crate::mm::virt::*;
use core::sync::atomic::{AtomicU64, Ordering};

fn align_up(x: usize, a: usize) -> usize {
    (x + a - 1) & !(a - 1)
}

fn align_down(x: usize, a: usize) -> usize {
    x & !(a - 1)
}

fn divide_up(x: usize, a: usize) -> usize {
    (x + a - 1) / a
}

#[repr(transparent)]
struct FreeSlot {
    next: *mut FreeSlot,
}

struct SlabHeader {
    link: LinkedListLink,
    slab: *const Slab,
    free: Cell<*mut FreeSlot>,
    used: Cell<usize>,
}

unsafe impl Send for SlabHeader {}
unsafe impl Sync for SlabHeader {}

intrusive_adapter!(SlabPageAdapter = UnsafeRef<SlabHeader>: SlabHeader { link => LinkedListLink });

struct Slab {
    ent_size: usize,
    partial: SpinLock<LinkedList<SlabPageAdapter>>,
}

impl Slab {
    const fn new(size: usize) -> Self {
        Self {
            ent_size: size,
            partial: SpinLock::new(LinkedList::new(SlabPageAdapter::NEW)),
        }
    }

    fn slot_offset(&self) -> usize {
        align_up(size_of::<SlabHeader>(), self.ent_size)
    }

    fn refill(&self) -> *mut SlabHeader {
        let phys = match PMM
            .lock()
            .as_mut()
            .and_then(|p| p.alloc(PageUsage::KernelHeap))
        {
            Some(p) => p,
            None => return null_mut(),
        };

        let page = (phys + HHDM_OFFSET.load(Ordering::Relaxed)) as *mut SlabHeader;
        let offset = self.slot_offset();
        let slots = (PAGE_SIZE - offset) / self.ent_size;
        debug_assert!(slots > 0);

        unsafe {
            let first = (page as *mut u8).byte_add(offset) as *mut FreeSlot;
            for i in 0..slots - 1 {
                (*first.byte_add(i * self.ent_size)).next = first.byte_add((i + 1) * self.ent_size);
            }
            (*first.byte_add((slots - 1) * self.ent_size)).next = null_mut();

            page.write(SlabHeader {
                link: LinkedListLink::new(),
                slab: &raw const *self,
                free: Cell::new(first),
                used: Cell::new(0),
            });
        }

        page
    }

    fn alloc(&self) -> *mut u8 {
        let mut list = self.partial.lock();

        if list.is_empty() {
            let page = self.refill();
            if page.is_null() {
                return null_mut();
            }
            list.push_front(unsafe { UnsafeRef::from_raw(page) });
        }

        let mut cursor = list.front_mut();
        let page = cursor.get().unwrap();

        let slot = page.free.get();
        debug_assert!(!slot.is_null());
        page.free.set(unsafe { (*slot).next });
        page.used.set(page.used.get() + 1);

        if page.free.get().is_null() {
            cursor.remove();
        }

        slot as *mut u8
    }

    unsafe fn free(&self, page: *mut SlabHeader, addr: *mut u8) {
        if addr.is_null() {
            return;
        }

        let slot = addr as *mut FreeSlot;
        let mut list = self.partial.lock();

        let (was_linked, now_empty) = {
            let page_ref = unsafe { &*page };
            let was_linked = page_ref.link.is_linked();
            unsafe {
                (*slot).next = page_ref.free.get();
            }
            page_ref.free.set(slot);
            let used = page_ref.used.get() - 1;
            page_ref.used.set(used);
            (was_linked, used == 0)
        };

        if now_empty {
            if was_linked {
                unsafe {
                    list.cursor_mut_from_ptr(page).remove();
                }
            }
            let phys = page as u64 - HHDM_OFFSET.load(Ordering::Relaxed);
            PMM.lock().as_mut().unwrap().free(phys);
        } else if !was_linked {
            unsafe {
                list.push_front(UnsafeRef::from_raw(page));
            }
        }
    }
}

pub struct SlabAllocator {
    slabs: [Slab; 8],
}

#[global_allocator]
pub static ALLOCATOR: SlabAllocator = SlabAllocator {
    slabs: [
        Slab::new(16),
        Slab::new(32),
        Slab::new(64),
        Slab::new(128),
        Slab::new(256),
        Slab::new(512),
        Slab::new(1024),
        Slab::new(2048),
    ],
};

static BIG_ALLOC_PTR: AtomicU64 = AtomicU64::new(BIG_ALLOC_START);

fn big_alloc(pages: usize) -> *mut u8 {
    let allocation_base =
        BIG_ALLOC_PTR.fetch_add(((pages + 1) * PAGE_SIZE) as u64, Ordering::Relaxed);

    let kernel_address_space = KERNEL_ADDRESS_SPACE.get().unwrap();
    let mut kernel_address_space = kernel_address_space.lock();

    for i in 0..pages + 1 {
        let page = match PMM.lock().as_mut().unwrap().alloc(PageUsage::KernelHeap) {
            Some(p) => p,
            None => return core::ptr::null_mut(),
        };

        kernel_address_space.map(
            allocation_base + (i << PAGE_SHIFT) as u64,
            page,
            PteFlags::PRESENT | PteFlags::WRITABLE,
        );
    }

    let header = allocation_base as *mut u64;
    unsafe {
        header.write(pages as u64);
    }

    (allocation_base + PAGE_SIZE as u64) as *mut u8
}

fn big_dealloc(ptr: *mut u8) {
    let allocation_base = ptr as u64 - PAGE_SIZE as u64;
    let header = allocation_base as *mut u64;
    let pages = unsafe { header.read() };

    let kernel_address_space = KERNEL_ADDRESS_SPACE.get().unwrap();
    let mut kernel_address_space = kernel_address_space.lock();

    for i in 0..pages + 1 {
        if let Some(phys) = kernel_address_space.unmap(allocation_base + i * PAGE_SIZE as u64) {
            PMM.lock().as_mut().unwrap().free(phys);
        }
    }
}

unsafe impl GlobalAlloc for SlabAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.size() == 0 {
            return null_mut();
        }

        let effective = layout.size().max(layout.align());
        if let Some(slab) = ALLOCATOR
            .slabs
            .iter()
            .find(|slab| slab.ent_size >= effective)
        {
            let result = slab.alloc();
            debug_assert!((result as usize).is_multiple_of(layout.align()));
            return result;
        }

        debug_assert!(layout.align() <= PAGE_SIZE);
        let num_pages = divide_up(layout.size(), PAGE_SIZE);

        big_alloc(num_pages)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        if ptr.is_null() {
            return;
        }

        unsafe {
            if ptr as usize == align_down(ptr as usize, PAGE_SIZE) {
                big_dealloc(ptr);
            } else {
                let page = align_down(ptr as usize, PAGE_SIZE) as *mut SlabHeader;
                (*(*page).slab).free(page, ptr);
            }
        }
    }
}
