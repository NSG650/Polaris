#include "nvme.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <errno.h>
#include <fs/partition.h>
#include <klibc/event.h>
#include <klibc/kargs.h>
#include <klibc/mem.h>
#include <klibc/misc.h>
#include <klibc/module.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/isr.h>
#include <sys/prcb.h>
#include <sys/timer.h>

struct nvme_device *nvme_devices[MAX_NVME_DRIVE_COUNT] = {0};
size_t total_device_count = 0;

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *pdest = (uint8_t *)dest;
	const uint8_t *psrc = (const uint8_t *)src;

	for (size_t i = 0; i < n; i++) {
		pdest[i] = psrc[i];
	}

	return dest;
}

void *memset(void *b, int c, size_t len) {
	size_t i = 0;
	unsigned char *p = b;
	while (len > 0) {
		*p = c;
		p++;
		len--;
	}
	return b;
}

void driver_exit(void) {
	kprintf("Bye bye!\n");
}

static void nvme_create_admin_queue(struct nvme_device *ctrl,
									struct nvme_queue *queue, uint64_t slots,
									uint64_t id) {
	queue->submit = kmalloc(sizeof(struct nvme_cmd) * slots); // command queue
	queue->submit_db =
		(uint32_t *)((uint64_t)ctrl->nvme_bar0 + PAGE_SIZE +
					 (2 * id * (4 << ctrl->stride)) + MEM_PHYS_OFFSET);
	queue->sq_head = 0;
	queue->sq_tail = 0;
	queue->completion =
		kmalloc(sizeof(struct nvme_cmd_comp) * slots); // command result queue
	queue->complete_db =
		(uint32_t *)((uint64_t)ctrl->nvme_bar0 + PAGE_SIZE +
					 ((2 * id + 1) * (4 << ctrl->stride)) + MEM_PHYS_OFFSET);
	queue->cq_vec = 0;
	queue->cq_head = 0;
	queue->cq_phase = 1;
	queue->elements = slots;
	queue->queue_id = id;
	queue->cmd_id = 0;
	queue->phys_regpgs = NULL;
}

static void nvme_create_queue(struct nvme_namespace_device *ns,
							  struct nvme_queue *queue, uint64_t slots,
							  uint64_t id) {
	queue->submit = kmalloc(sizeof(struct nvme_cmd) * slots); // command queue
	queue->submit_db =
		(uint32_t *)((uint64_t)ns->controller->nvme_bar0 + PAGE_SIZE +
					 (2 * id * (4 << ns->controller->stride)) +
					 MEM_PHYS_OFFSET);
	queue->sq_head = 0;
	queue->sq_tail = 0;
	queue->completion =
		kmalloc(sizeof(struct nvme_cmd_comp) * slots); // command result queue
	queue->complete_db =
		(uint32_t *)((uint64_t)ns->controller->nvme_bar0 + PAGE_SIZE +
					 ((2 * id + 1) * (4 << ns->controller->stride)) +
					 MEM_PHYS_OFFSET);
	queue->cq_vec = 0;
	queue->cq_head = 0;
	queue->cq_phase = 1;
	queue->elements = slots;
	queue->queue_id = id;
	queue->cmd_id = 0;
	queue->phys_regpgs =
		pmm_allocz(((ns->max_prps * slots * sizeof(uint64_t)) / PAGE_SIZE) + 1);
}

static void nvme_submit_cmd(struct nvme_queue *queue, struct nvme_cmd cmd) {
	uint16_t tail = queue->sq_tail; // tail of the submit queue
	queue->submit[tail] = cmd;		// add to tail (end of queue)
	tail++;
	if (tail == queue->elements) {
		tail = 0;
	}
	*(queue->submit_db) = tail; // set to tail
	queue->sq_tail = tail;		// update tail so now it'll point to the element
								// after (nothing until we submit a new command)
}

// submit a command and await for completion
static uint16_t nvme_await_submit_cmd(struct nvme_queue *queue,
									  struct nvme_cmd cmd) {
	uint16_t head = queue->cq_head;
	uint16_t phase = queue->cq_phase;
	cmd.common.cid = queue->cmd_id++;
	nvme_submit_cmd(queue, cmd);
	uint16_t status = 0;

	while (true) {
		status = queue->completion[queue->cq_head].status;
		if ((status & 0x01) == phase) {
			break;
		}
		pause();
	}

	head++;
	if (head == queue->elements) {
		head = 0;
		queue->cq_phase = !queue->cq_phase; // flip phase
	}

	*(queue->complete_db) = head;
	queue->cq_head = head;
	return status;
}

static bool nvme_identify(struct nvme_device *controller, struct nvme_id *id) {
	uint64_t len = sizeof(struct nvme_id);
	struct nvme_cmd cmd = {0};
	cmd.identify.opcode = NVME_OP_IDENTIFY;
	cmd.identify.nsid = 0;
	cmd.identify.cns = 1;
	cmd.identify.prp1 = (uint64_t)id - MEM_PHYS_OFFSET;
	ssize_t off = (uint64_t)id & (PAGE_SIZE - 1);
	len -= (PAGE_SIZE - off);
	if (len <= 0) {
		cmd.identify.prp2 = 0;
	} else {
		uint64_t addr = (uint64_t)id + (PAGE_SIZE - off);
		cmd.identify.prp2 = addr;
	}

	uint16_t status =
		(nvme_await_submit_cmd(&controller->admin_queue, cmd) >> 1);
	if (status != 0) {
		return false;
	}

	size_t shift = 12 + CAP_MIN_PAGE_SIZE(controller->nvme_bar0->capabilities);
	size_t max_trans_shift = 0;
	if (id->mdts) {
		max_trans_shift = shift + id->mdts;
	} else {
		max_trans_shift = 20;
	}
	controller->max_trans_shift = max_trans_shift;
	return true;
}

static bool nvme_namespace_identify(struct nvme_namespace_device *namespace,
									struct nvme_namespace_id *namespace_id) {
	struct nvme_cmd cmd = {0};
	cmd.identify.opcode = NVME_OP_IDENTIFY;
	cmd.identify.nsid =
		namespace->namespace_id; // differentiate from normal identify by
								 // passing name space id
	cmd.identify.cns = 0;
	cmd.identify.prp1 = (uint64_t)namespace_id - MEM_PHYS_OFFSET;
	uint16_t status =
		(nvme_await_submit_cmd(&namespace->controller->admin_queue, cmd) >> 1);

	return !(status != 0);
}

static bool nvme_create_queues(struct nvme_device *ctrl,
							   struct nvme_namespace_device *ns, uint16_t qid) {
	nvme_create_queue(ns, &ns->queue, ctrl->queue_slots, qid);

	// Create the completion queue
	struct nvme_cmd cmd1 = {0};
	cmd1.createcompq.opcode = NVME_OP_CREATE_CQ;
	cmd1.createcompq.prp1 = (uint64_t)ns->queue.completion - MEM_PHYS_OFFSET;
	cmd1.createcompq.cqid = qid;
	cmd1.createcompq.size = ctrl->queue_slots - 1;
	cmd1.createcompq.cqflags = (1 << 0);
	cmd1.createcompq.irqvec = 0;
	uint16_t status = nvme_await_submit_cmd(&ctrl->admin_queue, cmd1) >> 1;
	if (status != 0) {
		kprintf("\t\t\tNVMe: Failed create completion queue. status: %b\n",
				status);
		return false;
	}

	// Create the submission queue
	struct nvme_cmd cmd2 = {0};
	cmd2.createsubq.opcode = NVME_OP_CREATE_SQ;
	cmd2.createsubq.prp1 = (uint64_t)ns->queue.submit - MEM_PHYS_OFFSET;
	cmd2.createsubq.sqid = qid;
	cmd2.createsubq.cqid = qid;
	cmd2.createsubq.size = ctrl->queue_slots - 1;
	cmd2.createsubq.sqflags =
		(1 << 0) | (2 << 1); // queue phys + medium priority

	status = nvme_await_submit_cmd(&ctrl->admin_queue, cmd2) >> 1;
	if (status != 0) {
		kprintf("\t\t\tNVMe: Failed to create submission queue. status %b\n",
				status);
		return false;
	}

	return true;
}

static bool
nvme_namespace_read_or_write_block(struct nvme_namespace_device *this,
								   uint64_t start, uint64_t count, void *buffer,
								   bool rw) {
	if (rw &&
		!(kernel_arguments.kernel_args & KERNEL_ARGS_ALLOW_WRITES_TO_DISKS)) {
		return false;
	}

	uint32_t command_id = this->queue.cmd_id;

	if (start + count > this->lba_count) {
		count -= ((start + count) - this->lba_count);
	}
	size_t page_offset = (uint64_t)buffer & (PAGE_SIZE - 1);
	bool should_use_prp = false;
	bool should_use_prp_list = false;

	if ((count * this->lba_size) > PAGE_SIZE) {
		should_use_prp = true;
		if ((count * this->lba_size) > (PAGE_SIZE * 2)) {
			should_use_prp_list = true;
			should_use_prp = false;
		}
	}

	// Now that our buffer can't be fit into 2 PAGE_SIZE we need to setup the
	// PRP list.
	if (should_use_prp_list) {
		size_t prp_count = ((count - 1) * this->lba_size) / PAGE_SIZE;
		if (prp_count > this->max_prps) {
			kprintf("Warning: This %s exceeds the maximum amount of PRPs this "
					"namespace can offer\n",
					rw ? "write" : "read");
			kprintf("Warning: Dropping this request\n");
			return false;
		}
		for (size_t i = 0; i < prp_count; i++) {
			this->queue.phys_regpgs[i + command_id * this->max_prps] =
				((uint64_t)(buffer - MEM_PHYS_OFFSET - page_offset) +
				 PAGE_SIZE + i * PAGE_SIZE);
		}
	}

	struct nvme_cmd cmd = {0};
	cmd.rw.opcode = rw ? NVME_OP_WRITE : NVME_OP_READ;
	cmd.rw.flags = 0;
	cmd.rw.nsid = this->namespace_id;
	cmd.rw.control = 0;
	cmd.rw.dsmgmt = 0;
	cmd.rw.ref = 0;
	cmd.rw.apptag = 0;
	cmd.rw.appmask = 0;
	cmd.rw.metadata = 0;
	cmd.rw.slba = start;
	cmd.rw.len = count - 1;

	if (should_use_prp) {
		cmd.rw.prp2 =
			(uint64_t)((uint64_t)buffer + PAGE_SIZE - MEM_PHYS_OFFSET);
	} else {
		cmd.rw.prp1 = (uint64_t)buffer - MEM_PHYS_OFFSET;
		if (should_use_prp_list) {
			cmd.rw.prp2 =
				(uint64_t)(&this->queue
								.phys_regpgs[command_id * this->max_prps]) -
				MEM_PHYS_OFFSET;
		}
	}

	uint16_t status = (nvme_await_submit_cmd(&this->queue, cmd) >> 1);
	return (status == 0);
}

static int nvme_cache_fetch(struct nvme_namespace_device *this, uint64_t lba) {
	for (int i = 0; i < 512; i++) {
		if (this->cache[i].lba == lba && this->cache[i].status == CACHE_USED) {
			this->cache[i].hit_count++;
			this->cache[i].last_hit = timer_count();
			return i;
		}
	}
	return -1;
}

static int nvme_cache_cleanup(struct nvme_namespace_device *this) {
	int last_hit_index = -1;

	struct nvme_block_cache *to_be_freed = &this->cache[0];

	for (int i = 0; i < 512; i++) {
		struct nvme_block_cache *c = &this->cache[i];
		if (c->status != CACHE_USED) {
			continue;
		}
		if (c->hit_count < to_be_freed->hit_count) {
			last_hit_index = i;
			to_be_freed = c;
		}
		if (c->hit_count == to_be_freed->hit_count) {
			if (c->last_hit > to_be_freed->last_hit) {
				last_hit_index = i;
				to_be_freed = c;
			}
		}
	}

	to_be_freed->status = CACHE_FREE;
	pmm_free((void *)((uintptr_t)to_be_freed->cache - MEM_PHYS_OFFSET),
			 (((this->lba_size) / PAGE_SIZE) + 1));

	return last_hit_index;
}

static int nvme_cache_it(struct nvme_namespace_device *this, uint64_t lba) {
	int target = -1;
look_again:
	for (int i = 0; i < 512; i++) {
		if (this->cache[i].status == CACHE_FREE) {
			target = i;
			break;
		}
	}
	if (target == -1) {
		nvme_cache_cleanup(this);
		goto look_again;
	}

	struct nvme_block_cache *t = &this->cache[target];
	t->cache = (uint8_t *)((uintptr_t)pmm_allocz(
							   (((this->lba_size) / PAGE_SIZE) + 1)) +
						   MEM_PHYS_OFFSET);
	t->hit_count = 0;
	t->status = CACHE_USED;
	t->last_hit = timer_count();
	t->lba = lba;

	if (!nvme_namespace_read_or_write_block(this, lba, 1, t->cache, false)) {
		kprintf("Failed to read\n");
		pmm_free((void *)((uintptr_t)t->cache - MEM_PHYS_OFFSET),
				 (((this->lba_size) / PAGE_SIZE) + 1));
		t->status = CACHE_FREE;
		target = -1;
	}

	return target;
}

static ssize_t nvme_read(struct resource *_this,
						 struct f_description *description, void *buffer,
						 off_t loc, size_t count) {
	struct nvme_namespace_device *this =
		(struct nvme_namespace_device *)(_this);
	spinlock_acquire_or_wait(&this->res.lock);

	size_t block_size = this->lba_size;

	ssize_t ret = count;
	size_t i = 0;
	for (i = 0; i < count;) {
		uint64_t lba = (loc + i) / this->lba_size;
		int cache_index = nvme_cache_fetch(this, lba);
		if (cache_index == -1) { // time to cache it
			cache_index = nvme_cache_it(this, lba);
			if (cache_index == -1) {
				ret = -1;
				break;
			}
		}
		uint64_t chunk = count - i;
		size_t offset = (loc + i) % this->lba_size;
		if (chunk > this->lba_size - offset) {
			chunk = this->lba_size - offset;
		}

		memcpy((void *)((uintptr_t)buffer + i),
			   &this->cache[cache_index].cache[offset], chunk);
		i += chunk;
	}
end:
	spinlock_drop(&this->res.lock);
	return ret;
}

static ssize_t nvme_write(struct resource *_this,
						  struct f_description *description, const void *buffer,
						  off_t loc, size_t count) {
	struct nvme_namespace_device *this =
		(struct nvme_namespace_device *)(_this);
	spinlock_acquire_or_wait(&this->res.lock);

	size_t block_size = this->lba_size;

	ssize_t ret = count;

	for (size_t i = 0; i < count;) {
		uint64_t lba = (loc + i) / this->lba_size;
		int cache_index = nvme_cache_fetch(this, lba);
		if (cache_index == -1) { // time to cache it
			cache_index = nvme_cache_it(this, lba);
			if (cache_index == -1) {
				ret = -1;
				break;
			}
		}
		uint64_t chunk = count - i;
		size_t offset = (loc + i) % this->lba_size;
		if (chunk > this->lba_size - offset) {
			chunk = this->lba_size - offset;
		}

		memcpy(&this->cache[cache_index].cache[offset],
			   (void *)((uintptr_t)buffer + i), chunk);
		this->cache[cache_index].status = CACHE_USED;

		if (!nvme_namespace_read_or_write_block(
				this, lba, 1, this->cache[cache_index].cache, true)) {
			kprintf("Failed to write\n");
			ret = -1;
			break;
		}

		i += chunk;
	}

end:
	spinlock_drop(&this->res.lock);
	return ret;
}

bool nvme_init_namespace(size_t namespace_id,
						 struct nvme_namespace_device *this,
						 struct nvme_device *controller) {
	this->controller = controller;
	this->namespace_id = namespace_id;

	this->id =
		(struct nvme_namespace_id *)((uintptr_t)pmm_allocz(
										 (sizeof(struct nvme_namespace_id) /
										  PAGE_SIZE) +
										 1) +
									 MEM_PHYS_OFFSET);

	if (!nvme_namespace_identify(this, this->id)) {
		pmm_free((void *)((uintptr_t)this->id - MEM_PHYS_OFFSET),
				 (sizeof(struct nvme_namespace_id) / PAGE_SIZE) + 1);
		kprintf("\t\tError: Failed to identify NVMe namespace\n");
		return false;
	}

	uint64_t formatted_lba = this->id->flbas & 0x0f;
	uint64_t lba_shift = this->id->lbaf[formatted_lba].lba_data_size;

	uint64_t max_lba_size = 1 << (controller->max_trans_shift - lba_shift);
	this->max_prps = (max_lba_size * (1 << lba_shift)) / PAGE_SIZE;

	if (!nvme_create_queues(controller, this, namespace_id)) {
		pmm_free((void *)((uintptr_t)this->id - MEM_PHYS_OFFSET),
				 (sizeof(struct nvme_namespace_id) / PAGE_SIZE) + 1);
		kprintf("\t\tError: Failed to create NVMe namespace queues\n");
		return false;
	}

	this->lba_size = 1 << this->id->lbaf[formatted_lba].lba_data_size;
	this->lba_count = this->id->size;

	this->res.can_mmap = false;
	this->res.write = nvme_write;
	this->res.read = nvme_read;
	this->res.ioctl = resource_default_ioctl;

	this->res.stat.st_size = this->id->size * this->lba_size;
	this->res.stat.st_blocks = this->id->size;
	this->res.stat.st_blksize = this->lba_size;
	this->res.stat.st_mode = 0666 | S_IFBLK;
	this->res.stat.st_rdev = resource_create_dev_id();

	char name[32] = "nvme";
	char num[21] = {0};
	ultoa(total_device_count, num, 10);
	strcat(name, num);
	memzero(num, 21);
	ultoa(this->namespace_id, num, 10);
	strcat(name, "n");
	strcat(name, num);

	devtmpfs_add_device((struct resource *)this, name);

	partition_enumerate((struct resource *)this, name);
	return true;
}

void nvme_init_controller(struct pci_device *pci_dev) {
	struct nvme_device *this = resource_create(sizeof(struct nvme_device));
	this->pci_dev = pci_dev;

	if (!pci_get_bar_n(pci_dev, &this->pci_bar0, 0)) {
		kprintf("Error: Failed to get BAR0 for this NVMe controller %x:%x with "
				"device id %x and vendor id %x",
				pci_dev->bus, pci_dev->device, pci_dev->device_id,
				pci_dev->vendor_id);
		return;
	}

	if (this->pci_bar0.is_mmio == false) {
		kprintf("Error: This NVMe controller is not MMIO???\n");
		return;
	}

	uint16_t pci_cmd_ret = PCI_READ_W(this->pci_dev, 0x04);

	pci_cmd_ret &= ~0b111;
	// Enable MMIO and Bus mastering.
	pci_cmd_ret |= ((1 << 1) | (1 << 2)) & 0b111;

	PCI_WRITE_W(this->pci_dev, 0x04, pci_cmd_ret);

	this->nvme_bar0 = (struct nvme_bar *)this->pci_bar0.base;

	uint32_t conf = this->nvme_bar0->controller_config;
	if (conf & (1 << 0)) { // controller enabled?
		conf &= ~1;		   // disable controller
		this->nvme_bar0->controller_config = conf;
	}

	while ((this->nvme_bar0->controller_status) &
		   (1 << 0)) { // wait till the controller is back up
		pause();
	}

	kprintf("NVMe controller version %d.%d.%d at %x:%x with device id %x and "
			"vendor id %x\n",
			VERSION_MAJOR(this->nvme_bar0->version),
			VERSION_MINOR(this->nvme_bar0->version),
			VERSION_PATCH(this->nvme_bar0->version), pci_dev->bus,
			pci_dev->device, pci_dev->device_id, pci_dev->vendor_id);

	if ((CAP_COMMAND_SET(this->nvme_bar0->capabilities) & 1) == 0) {
		kprintf("\tError: This NVMe controller does not support NVM command "
				"set???\n");
		return;
	}

	this->stride = CAP_DOORBELL_STRIDE(this->nvme_bar0->capabilities);
	this->queue_slots = CAP_MAX_ENTRIES(this->nvme_bar0->capabilities);

	nvme_create_admin_queue(this, &this->admin_queue, this->queue_slots,
							0); // intialise first queue

	uint32_t admin_queue_attrs = this->queue_slots - 1;
	admin_queue_attrs |= admin_queue_attrs << 16;
	admin_queue_attrs |= admin_queue_attrs << 16;
	this->nvme_bar0->admin_queue_attrs = admin_queue_attrs;

	conf = CC_COMMANDSET_NVM | CC_ARBITRATION_ROUNDROBIN |
		   CC_SHUTDOWN_NOTIFICATIONS_NONE | CC_IO_SUBMISSION_QUEUE_SIZE |
		   CC_IO_COMPLETION_QUEUE_SIZE | CC_ENABLE;

	this->nvme_bar0->admin_submit_queue =
		(uint64_t)this->admin_queue.submit - MEM_PHYS_OFFSET;
	this->nvme_bar0->admin_completion_attrs =
		(uint64_t)this->admin_queue.completion - MEM_PHYS_OFFSET;

	this->nvme_bar0->controller_config = conf;

	while (true) {
		uint32_t status = this->nvme_bar0->controller_status;
		if (status & (1 << 0)) {
			break; // ready
		}
		if (!(status & (1 << 1))) {
			kprintf("\tError: Controller status is fatal\n");
			return;
		}
		pause();
	}

	this->id =
		(struct nvme_id *)((uintptr_t)pmm_allocz(
							   (sizeof(struct nvme_id) / PAGE_SIZE) + 1) +
						   MEM_PHYS_OFFSET);

	if (!nvme_identify(this, this->id)) {
		pmm_free((void *)((uintptr_t)this->id - MEM_PHYS_OFFSET),
				 (sizeof(struct nvme_id) / PAGE_SIZE) + 1);
		kprintf("\tError: Failed to identify NVMe controller\n");
		return;
	}

	char old[3] = {0};
	old[0] = this->id->mn[39];
	old[1] = this->id->sn[19];
	old[2] = this->id->fr[7];

	this->id->mn[39] = '\0';
	this->id->sn[19] = '\0';
	this->id->fr[7] = '\0';

	kprintf("\tNVMe controller %x:%x with model number %s and serial number %s "
			"running firmware version %s\n",
			this->id->vid, this->id->ssvid, this->id->mn, this->id->sn,
			this->id->fr);

	this->id->mn[39] = old[0];
	this->id->sn[19] = old[1];
	this->id->fr[7] = old[2];

	this->namespace_ids =
		(uint32_t *)((uintptr_t)pmm_allocz(((this->id->nn * 4) / PAGE_SIZE) +
										   1) +
					 MEM_PHYS_OFFSET);

	struct nvme_cmd get_namespaces = {0};
	get_namespaces.identify.opcode = NVME_OP_IDENTIFY;
	get_namespaces.identify.cns = 2;
	get_namespaces.identify.prp1 =
		(uint64_t)this->namespace_ids - MEM_PHYS_OFFSET;

	if ((nvme_await_submit_cmd(&this->admin_queue, get_namespaces) >> 1)) {
		pmm_free((void *)((uintptr_t)this->id - MEM_PHYS_OFFSET),
				 (sizeof(struct nvme_id) / PAGE_SIZE) + 1);
		pmm_free((void *)((uintptr_t)this->namespace_ids - MEM_PHYS_OFFSET),
				 ((this->id->nn * 4) / PAGE_SIZE) + 1);
		kprintf("\tError: Failed to get namespace id for this controller\n");
		return;
	}

	this->namespace_devices =
		resource_create(sizeof(struct nvme_namespace_device) * this->id->nn);

	for (size_t i = 0; i < this->id->nn; i++) {
		if (this->namespace_ids[i] && this->namespace_ids[i] <= this->id->nn) {
			kprintf("\tFound namespace: %lx\n", this->namespace_ids[i]);
			if (!nvme_init_namespace(this->namespace_ids[i],
									 &this->namespace_devices[i], this)) {
				pmm_free((void *)((uintptr_t)this->id - MEM_PHYS_OFFSET),
						 (sizeof(struct nvme_id) / PAGE_SIZE) + 1);
				pmm_free(
					(void *)((uintptr_t)this->namespace_ids - MEM_PHYS_OFFSET),
					((this->id->nn * 4) / PAGE_SIZE) + 1);

				kfree(this->namespace_devices);

				kprintf("\tError: Failed to setup namespace id %lu for this "
						"controller\n",
						this->namespace_ids[i]);

				return;
			}
		}
	}

	this->res.can_mmap = false;
	this->res.stat.st_mode = 0666 | S_IFCHR;
	this->res.stat.st_rdev = resource_create_dev_id();

	char name[32] = "nvme";
	char num[21] = {0};
	ultoa(total_device_count, num, 10);
	strcat(name, num);
	devtmpfs_add_device((struct resource *)this, name);

	nvme_devices[total_device_count] = this;
	total_device_count++;
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = driver_exit;

	for (int i = 0; i < 256; i++) {
		struct pci_device *pci_dev = pci_devices[i];
		// We want a mass storage device which is non volatile and it is NVM
		// Express
		if (pci_dev) {
			if (pci_dev->classcode == 0x1 && pci_dev->subclass == 0x8 &&
				pci_dev->progintf == 0x2) {
				nvme_init_controller(pci_dev);
			}
		}
	}

	if (total_device_count == 0) {
		kprintf("No NVMe controllers found on the system!\n");
		return 2;
	}

	return 0;
}
