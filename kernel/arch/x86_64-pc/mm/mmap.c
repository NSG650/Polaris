#include <cpu/cr.h>
#include <debug/debug.h>
#include <errno.h>
#include <klibc/misc.h>
#include <mm/mmap.h>
#include <sched/sched.h>
#include <sys/prcb.h>

struct addr2range {
	struct mmap_range_local *range;
	size_t memory_page;
	size_t file_page;
};

struct addr2range addr2range(struct pagemap *pagemap, uintptr_t virt) {
	struct mmap_range_local *local_range = NULL;
	int i = 0;
	vec_foreach(&pagemap->mmap_ranges, local_range, i) {
		if (virt < local_range->base ||
			virt >= local_range->base + local_range->length) {
			continue;
		}

		size_t memory_page = virt / PAGE_SIZE;
		size_t file_page = local_range->offset / PAGE_SIZE +
						   (memory_page - local_range->base / PAGE_SIZE);
		return (struct addr2range){.range = local_range,
								   .memory_page = memory_page,
								   .file_page = file_page};
	}

	return (struct addr2range){.range = NULL, .memory_page = 0, .file_page = 0};
}

bool mmap_handle_pf(registers_t *reg) {
	if ((reg->errorCode & 0x1) != 0) {
		return false;
	}

	// TODO: mmap can be expensive, consider enabling interrupts
	// temporarily
	uint64_t cr2 = read_cr("2");

	struct thread *thread = sched_get_running_thread();
	if (thread == NULL) {
		return false;
	}
	struct process *process = thread->mother_proc;
	struct pagemap *pagemap = process->process_pagemap;

	spinlock_acquire_or_wait(&pagemap->lock);

	struct addr2range range = addr2range(pagemap, cr2);
	struct mmap_range_local *local_range = range.range;

	spinlock_drop(&pagemap->lock);

	if (local_range == NULL) {
		return false;
	}

	void *page = NULL;
	if ((local_range->flags & MAP_ANONYMOUS) != 0) {
		page = pmm_allocz(1);
	} else {
		struct resource *res = page = local_range->global->res;
		page = res->mmap(res, range.file_page, local_range->flags);
	}

	if (page == NULL) {
		return false;
	}

	return mmap_page_in_range(local_range->global,
							  range.memory_page * PAGE_SIZE, (uintptr_t)page,
							  local_range->prot);
}

bool mmap_range(struct pagemap *pagemap, uintptr_t virt, uintptr_t phys,
				size_t length, int prot, int flags) {
	flags |= MAP_ANONYMOUS;

	uintptr_t aligned_virt = ALIGN_DOWN(virt, PAGE_SIZE);
	size_t aligned_length = ALIGN_UP(length + (virt - aligned_virt), PAGE_SIZE);

	struct mmap_range_global *global_range = NULL;
	struct mmap_range_local *local_range = NULL;

	global_range = kmalloc(sizeof(struct mmap_range_global));
	if (global_range == NULL) {
		errno = ENOMEM;
		goto cleanup;
	}

	global_range->shadow_pagemap = vmm_new_pagemap();
	if (global_range->shadow_pagemap == NULL) {
		goto cleanup;
	}

	global_range->base = aligned_virt;
	global_range->length = aligned_length;

	local_range = kmalloc(sizeof(struct mmap_range_local));
	if (local_range == NULL) {
		errno = ENOMEM;
		goto cleanup;
	}

	local_range->pagemap = pagemap;
	local_range->global = global_range;
	local_range->base = aligned_virt;
	local_range->length = aligned_length;
	local_range->prot = prot;
	local_range->flags = flags;

	vec_push(&global_range->locals, local_range);

	spinlock_acquire_or_wait(&pagemap->lock);

	vec_push(&pagemap->mmap_ranges, local_range);

	spinlock_drop(&pagemap->lock);

	for (size_t i = 0; i < aligned_length; i += PAGE_SIZE) {
		if (!mmap_page_in_range(global_range, aligned_virt + i, phys + i,
								prot)) {
			// FIXME: Page map is in inconsistent state at this point!
			goto cleanup;
		}
	}

	return true;

cleanup:
	if (local_range != NULL) {
		kfree(local_range);
	}
	if (global_range != NULL) {
		if (global_range->shadow_pagemap != NULL) {
			vmm_destroy_pagemap(global_range->shadow_pagemap);
		}

		kfree(global_range);
	}
	return false;
}

bool mmap_page_in_range(struct mmap_range_global *global, uintptr_t virt,
						uintptr_t phys, int prot) {
	uint64_t pt_flags = PAGE_READ | PAGE_USER;

	if ((prot & PROT_WRITE) != 0) {
		pt_flags |= PAGE_WRITE;
	}
	if ((prot & PROT_EXEC) == 0) {
		pt_flags |= PAGE_NO_EXECUTE;
	}

	if (!vmm_map_page(global->shadow_pagemap, virt, phys, pt_flags, Size4KiB)) {
		return false;
	}

	struct mmap_range_local *local_range = NULL;
	int i = 0;
	vec_foreach(&global->locals, local_range, i) {
		if (virt < local_range->base ||
			virt >= local_range->base + local_range->length) {
			continue;
		}

		if (!vmm_map_page(local_range->pagemap, virt, phys, pt_flags,
						  Size4KiB)) {
			return false;
		}
	}

	return true;
}

void *mmap(struct pagemap *pagemap, uintptr_t addr, size_t length, int prot,
		   int flags, struct resource *res, off_t offset) {
	struct mmap_range_global *global_range = NULL;
	struct mmap_range_local *local_range = NULL;

	if (length == 0) {
		errno = EINVAL;
		goto cleanup;
	}
	length = ALIGN_UP(length, PAGE_SIZE);

	if ((flags & MAP_ANONYMOUS) == 0 && res != NULL && !res->can_mmap) {
		errno = ENODEV;
		return MAP_FAILED;
	}

	struct thread *thread = sched_get_running_thread();
	struct process *process = thread->mother_proc;

	uint64_t base = 0;
	if ((flags & MAP_FIXED) != 0) {
		if (!munmap(pagemap, addr, length)) {
			goto cleanup;
		}
		base = addr;
	} else {
		base = process->mmap_anon_base;
		process->mmap_anon_base += length + PAGE_SIZE;
	}

	global_range = kmalloc(sizeof(struct mmap_range_global));
	if (global_range == NULL) {
		errno = ENOMEM;
		goto cleanup;
	}

	global_range->shadow_pagemap = vmm_new_pagemap();
	if (global_range->shadow_pagemap == NULL) {
		goto cleanup;
	}

	global_range->base = base;
	global_range->length = length;
	global_range->res = res;
	global_range->offset = offset;

	local_range = kmalloc(sizeof(struct mmap_range_local));
	if (local_range == NULL) {
		goto cleanup;
	}

	local_range->pagemap = pagemap;
	local_range->global = global_range;
	local_range->base = base;
	local_range->length = length;
	local_range->prot = prot;
	local_range->flags = flags;
	local_range->offset = offset;

	vec_push(&global_range->locals, local_range);

	spinlock_acquire_or_wait(&pagemap->lock);

	vec_push(&pagemap->mmap_ranges, local_range);

	spinlock_drop(&pagemap->lock);

	if (res != NULL) {
		res->refcount++;
	}

	return (void *)base;

cleanup:
	if (local_range != NULL) {
		kfree(local_range);
	}
	if (global_range != NULL) {
		if (global_range->shadow_pagemap != NULL) {
			vmm_destroy_pagemap(global_range->shadow_pagemap);
		}

		kfree(global_range);
	}
	return MAP_FAILED;
}

bool munmap(struct pagemap *pagemap, uintptr_t addr, size_t length) {
	if (length == 0) {
		if (sched_get_running_thread()) {
			errno = EINVAL;
		}
		return false;
	}
	length = ALIGN_UP(length, PAGE_SIZE);

	for (uintptr_t i = addr; i < addr + length; i += PAGE_SIZE) {
		struct addr2range range = addr2range(pagemap, i);
		if (range.range == NULL) {
			continue;
		}

		struct mmap_range_local *local_range = range.range;
		struct mmap_range_global *global_range = local_range->global;

		uintptr_t snip_begin = i;
		for (;;) {
			i += PAGE_SIZE;
			if (i >= local_range->base + local_range->length ||
				i >= addr + length) {
				break;
			}
		}

		uintptr_t snip_end = i;
		size_t snip_length = snip_end - snip_begin;

		spinlock_acquire_or_wait(&pagemap->lock);

		if (snip_begin > local_range->base &&
			snip_end < local_range->base + local_range->length) {
			struct mmap_range_local *postsplit_range =
				kmalloc(sizeof(struct mmap_range_local));
			if (postsplit_range == NULL) {
				// FIXME: Page map is in inconsistent state at this point!
				if (sched_get_running_thread()) {
					errno = ENOMEM;
				}
				spinlock_drop(&pagemap->lock);
				return false;
			}

			postsplit_range->pagemap = local_range->pagemap;
			postsplit_range->global = global_range;
			postsplit_range->base = snip_end;
			postsplit_range->length =
				(local_range->base + local_range->length) - snip_end;
			postsplit_range->offset =
				local_range->offset + (off_t)(snip_end - local_range->base);
			postsplit_range->prot = local_range->prot;
			postsplit_range->flags = local_range->flags;

			vec_push(&pagemap->mmap_ranges, postsplit_range);

			local_range->length -= postsplit_range->length;
		}

		spinlock_drop(&pagemap->lock);

		for (uintptr_t j = snip_begin; j < snip_end; j += PAGE_SIZE) {
			vmm_unmap_page(pagemap, j, true);
		}

		if (snip_length == local_range->length) {
			vec_remove(&pagemap->mmap_ranges, local_range);
		}

		if (snip_length == local_range->length &&
			global_range->locals.length == 1) {
			if ((local_range->flags & MAP_ANONYMOUS) != 0) {
				for (uintptr_t j = global_range->base;
					 j < global_range->base + global_range->length;
					 j += PAGE_SIZE) {
					uintptr_t phys =
						vmm_virt_to_phys(global_range->shadow_pagemap, j);
					if (phys == INVALID_PHYS) {
						continue;
					}

					if (!vmm_unmap_page(global_range->shadow_pagemap, j,
										true)) {
						// FIXME: Page map is in inconsistent state at this
						// point!
						errno = EINVAL;
						return false;
					}
					pmm_free((void *)phys, 1);
				}
			} else {
				// TODO: res->unmap();
			}

			kfree(local_range);
		} else {
			if (snip_begin == local_range->base) {
				local_range->offset += snip_length;
				local_range->base = snip_end;
			}
			local_range->length -= snip_length;
		}
	}
	return true;
}

bool mprotect(struct pagemap *pagemap, uintptr_t addr, size_t length,
			  int prot) {
	if (length == 0) {
		errno = EINVAL;
		return false;
	}

	length = ALIGN_UP(length, PAGE_SIZE);

	for (uintptr_t i = addr; i < addr + length; i += PAGE_SIZE) {
		struct mmap_range_local *local_range = addr2range(pagemap, i).range;

		if (local_range->prot == prot) {
			continue;
		}

		uintptr_t snip_begin = i;
		for (;;) {
			i += PAGE_SIZE;
			if (i >= local_range->base + local_range->length ||
				i >= addr + length) {
				break;
			}
		}
		uintptr_t snip_end = i;
		uintptr_t snip_size = snip_end - snip_begin;

		spinlock_acquire_or_wait(&pagemap->lock);
		if (snip_begin > local_range->base &&
			snip_end < local_range->base + local_range->length) {
			struct mmap_range_local *postsplit_range =
				kmalloc(sizeof(struct mmap_range_local));

			postsplit_range->pagemap = local_range->pagemap;
			postsplit_range->global = local_range->global;
			postsplit_range->base = snip_end;
			postsplit_range->length =
				(local_range->base + local_range->length) - snip_end;
			postsplit_range->offset =
				local_range->offset + (off_t)(snip_end - local_range->base);
			postsplit_range->prot = local_range->prot;
			postsplit_range->flags = local_range->flags;

			vec_push(&pagemap->mmap_ranges, postsplit_range);

			local_range->length -= postsplit_range->length;
		}

		for (uintptr_t j = snip_begin; j < snip_end; j += PAGE_SIZE) {
			uint64_t pt_flags = PAGE_READ | PAGE_USER;

			if ((prot & PROT_WRITE) != 0) {
				pt_flags |= PAGE_WRITE;
			}
			if ((prot & PROT_EXEC) == 0) {
				pt_flags |= PAGE_NO_EXECUTE;
			}
			vmm_remap_page(pagemap, j, pt_flags, true);
		}

		uintptr_t new_offset =
			local_range->offset + (snip_begin - local_range->base);

		if (snip_begin == local_range->base) {
			local_range->offset += snip_size;
			local_range->base = snip_end;
		}
		local_range->length -= snip_size;

		struct mmap_range_local *new_range =
			kmalloc(sizeof(struct mmap_range_local));

		new_range->pagemap = local_range->pagemap;
		new_range->global = local_range->global;
		new_range->base = snip_begin;
		new_range->length = snip_size;
		new_range->offset = new_offset;
		new_range->prot = prot;
		new_range->flags = local_range->flags;

		vec_push(&pagemap->mmap_ranges, new_range);

		spinlock_drop(&pagemap->lock);
	}

	return true;
}

void syscall_mmap(struct syscall_arguments *args) {
	uintptr_t hint = args->args0;
	size_t length = args->args1;
	int prot = args->args2;
	int flags = args->args3;
	int fdnum = args->args4;
	off_t offset = args->args5;

	void *ret = MAP_FAILED;

	struct thread *thread = sched_get_running_thread();
	struct process *proc = thread->mother_proc;

	struct resource *res = NULL;
	if (fdnum != -1) {
		struct f_descriptor *fd = fd_from_fdnum(proc, fdnum);
		if (fd == NULL) {
			goto cleanup;
		}

		res = fd->description->res;
	} else if (offset != 0) {
		errno = EINVAL;
		goto cleanup;
	}

	ret = mmap(proc->process_pagemap, hint, length, prot, flags, res, offset);
cleanup:
	args->ret = (uint64_t)ret;
}

void syscall_munmap(struct syscall_arguments *args) {
	uintptr_t addr = args->args0;
	size_t length = args->args1;

	struct thread *thread = sched_get_running_thread();
	struct process *proc = thread->mother_proc;

	args->ret = munmap(proc->process_pagemap, addr, length) ? 0 : -1;
}

void syscall_mprotect(struct syscall_arguments *args) {
	uintptr_t addr = args->args0;
	size_t length = args->args1;
	int prot = args->args2;

	struct thread *thread = sched_get_running_thread();
	struct process *proc = thread->mother_proc;

	args->ret = mprotect(proc->process_pagemap, addr, length, prot) ? 0 : -1;
}
