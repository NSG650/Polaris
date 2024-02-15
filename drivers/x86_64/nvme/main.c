#include <debug/debug.h>
#include <io/pci.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <mm/slab.h>
#include <mm/vmm.h>

#include "nvme.h"

#define MAX_NVME_DRIVE_COUNT 32

struct nvme_device nvme_devices[MAX_NVME_DRIVE_COUNT] = {0};
size_t total_device_count = 0;

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

void nvme_init_controller(struct pci_device *pci_dev) {
	struct nvme_device *this = &nvme_devices[total_device_count];
	this->pci_dev = pci_dev;

	if (!pci_get_bar_n(pci_dev, &this->pci_bar0, 0)) {
		kprintf("Error: Failed to get BAR0 for this NVMe drive %x:%x with "
				"device id %x and vendor id %x",
				pci_dev->bus, pci_dev->device, pci_dev->device_id,
				pci_dev->vendor_id);
		return;
	}

	if (this->pci_bar0.is_mmio == false) {
		kprintf("Error: This NVMe controller is not MMIO???\n");
		return;
	}

	uint16_t pci_cmd_ret =
		pci_read(0, this->pci_dev->bus, this->pci_dev->device, 0, 0x04, 2);

	pci_cmd_ret |= (1 << 1);
	pci_cmd_ret |= (1 << 2);

	pci_write(0, this->pci_dev->bus, this->pci_dev->device, 0, 0x4, pci_cmd_ret,
			  2);

	this->nvme_bar0 = (struct nvme_bar *)this->pci_bar0.base;

	uint32_t conf = this->nvme_bar0->controller_config;
	if (conf & (1 << 0)) { // controller enabled?
		conf &= ~(1 << 0); // disable controller
		this->nvme_bar0->controller_config = conf;
	}

	while ((this->nvme_bar0->controller_status) & (1 << 0))
		; // await controller ready

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

	if ((1 << (CAP_MIN_PAGE_SIZE(this->nvme_bar0->capabilities) + 12)) >
			PAGE_SIZE ||
		(1 << (CAP_MAX_PAGE_SIZE(this->nvme_bar0->capabilities) + 12)) <
			PAGE_SIZE) {
		kprintf("\tError: This NVMe controller does not use the same page size "
				"as the CPU\n");
		return;
	}

#if 0
    this->stride = CAP_DOORBELL_STRIDE(this->nvme_bar0->capabilities);
    this->queue_slots = CAP_MAX_ENTRIES(this->nvme_bar0->capabilities);

    nvme_create_admin_queue(this, &this->admin_queue, this->queue_slots, 0); // intialise first queue

    uint32_t admin_queue_attrs = this->queue_slots - 1;
    admin_queue_attrs |= admin_queue_attrs << 16;
    admin_queue_attrs |= admin_queue_attrs << 16;
    this->nvme_bar0->admin_queue_attrs = admin_queue_attrs;
    conf = (0 << 4) | (0 << 11) | (0 << 14) | (6 << 16) | (4 << 20) | (1 << 0); // reinitialise config (along with enabling the controller again)
    this->nvme_bar0->admin_submit_queue = (uint64_t)this->nvme_bar0->admin_submit_queue - MEM_PHYS_OFFSET;
    this->nvme_bar0->admin_completion_attrs = (uint64_t)this->admin_queue.completion - MEM_PHYS_OFFSET;
    this->nvme_bar0->controller_config = conf;

    while (true) {
        uint32_t status = this->nvme_bar0->controller_status;
        if (status & (1 << 0)) {
            break; // ready
        }
        if(!(status & (1 << 1))) {
            kprintf("\tError: Controller status is fatal\n");
            return;
        }
    }
#endif

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
		return 0;
	}

	return 0;
}
