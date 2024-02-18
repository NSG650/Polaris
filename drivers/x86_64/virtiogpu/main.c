#include "virtiogpu.h"
#include <asm/asm.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <klibc/module.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

struct virtio_gpu_device gpu = {0};

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

void virtio_setup_queue(struct virtio_gpu_device *g, size_t queue_index) {
	struct virtio_queue *q = kmalloc(sizeof(struct virtio_queue));

	g->virtio_queues[queue_index] = q;

	// 2 pages. Playing safe here.
	q->descriptors =
		(struct queue_descriptor *)((uintptr_t)pmm_allocz(2) + MEM_PHYS_OFFSET);
	q->available =
		(struct queue_available *)((uintptr_t)pmm_allocz(2) + MEM_PHYS_OFFSET);
	q->used = (struct queue_used *)((uintptr_t)pmm_allocz(2) + MEM_PHYS_OFFSET);

	g->common_config->queue_select = queue_index;

	q->queue_size = g->common_config->queue_size;

	g->common_config->queue_desc = (uintptr_t)q->descriptors - MEM_PHYS_OFFSET;
	g->common_config->queue_available =
		(uintptr_t)q->available - MEM_PHYS_OFFSET;
	g->common_config->queue_used = (uintptr_t)q->used - MEM_PHYS_OFFSET;
	g->common_config->queue_enable = 1;
}

int *virtio_alloc_desc(struct virtio_queue *queue, int *desc, size_t num) {
	for (int i = 0; i < num; i++) {
		desc[i] = -1;
		for (int j = 0; j < queue->queue_size; j++) {
			if (queue->descriptors[j].address == 0) {
				queue->descriptors[j].address = 0xffffffff;
				desc[i] = j;
				break;
			}
		}
		if (desc[i] == -1) {
			return 0;
		}
	}
	return desc;
}

void virtio_free_desc(struct virtio_queue *queue, int desc) {
	do {
		queue->descriptors[desc].address = 0;
		if (queue->descriptors[desc].flags & VIRTIO_QUEUE_DESC_F_NEXT) {
			desc = queue->descriptors[desc].next;
		} else {
			desc = -1;
		}
	} while (desc != -1);
}

void virtio_queue_notify(struct virtio_gpu_device *g,
						 struct virtio_queue *queue) {
	*g->notify = 0;
}

void virtio_queue_notify_wait(struct virtio_gpu_device *g,
							  struct virtio_queue *queue) {
	int prev = queue->used->index;

	*g->notify = 0;

	while (prev == queue->used->index) {
		pause();
	}
}

void virtio_queue_avail_insert(struct virtio_queue *queue, int desc) {
	queue->available->rings[queue->available->index % queue->queue_size] = desc;
	queue->available->index++;
}

bool virtio_gpu_get_display_info(struct virtio_gpu_mode *current_mode,
								 uint32_t *scanout_id) {
	bool ret = false;

	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_control_header *req =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_display_info *resp =
		(struct virtio_gpu_display_info *)((uintptr_t)pmm_allocz(1) +
										   MEM_PHYS_OFFSET);

	req->type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
	req->flags = 0;
	req->fence_id = 0;
	req->context_id = 0;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_display_info);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	if (resp->header.type == VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
		for (int i = 0; i < 16; i++) {
			if (resp->modes[i].enabled) {
				*scanout_id = i;
				*current_mode = resp->modes[i];
				ret = true;
				break;
			}
		}
	}

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_create_2d_resource(uint32_t resource_id,
								   enum virtio_gpu_formats format,
								   uint32_t width, uint32_t height) {
	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_resource_create_2d *req =
		(struct virtio_gpu_resource_create_2d *)((uintptr_t)pmm_allocz(1) +
												 MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = 0;
	req->resource_id = resource_id;
	req->format = format;
	req->width = width;
	req->height = height;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_resource_create_2d);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_transfer_2d_resource_to_gpu(uint32_t resource_id, uint32_t x,
											uint32_t y, uint32_t width,
											uint32_t height) {
	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_transfer_to_host_2d *req =
		(struct virtio_gpu_transfer_to_host_2d *)((uintptr_t)pmm_allocz(1) +
												  MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = 0;
	req->resource_id = resource_id;
	req->offset = 0;
	req->rect.x = x;
	req->rect.y = y;
	req->rect.width = width;
	req->rect.height = height;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_transfer_to_host_2d);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_setup_scanout(uint32_t scanout_id, uint32_t resource_id,
							  uint32_t x, uint32_t y, uint32_t width,
							  uint32_t height) {
	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_set_scanout *req =
		(struct virtio_gpu_set_scanout *)((uintptr_t)pmm_allocz(1) +
										  MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_SET_SCANOUT;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = 0;
	req->scanout_id = scanout_id;
	req->resource_id = resource_id;
	req->rect.x = x;
	req->rect.y = y;
	req->rect.width = width;
	req->rect.height = height;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_set_scanout);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_flush(uint32_t resource_id, uint32_t x, uint32_t y,
					  uint32_t width, uint32_t height) {
	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_resource_flush *req =
		(struct virtio_gpu_resource_flush *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = 0;
	req->resource_id = resource_id;
	req->padding = 0;
	req->rect.x = x;
	req->rect.y = y;
	req->rect.width = width;
	req->rect.height = height;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_resource_flush);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_attach_backing(uint32_t resource_id, uintptr_t phys_addr,
							   size_t length) {
	int desc[3] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 3)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_resource_attach_backing *req =
		(struct virtio_gpu_resource_attach_backing *)((uintptr_t)pmm_allocz(1) +
													  MEM_PHYS_OFFSET);
	struct virtio_gpu_mem_entry *mem_req =
		(struct virtio_gpu_mem_entry *)((uintptr_t)pmm_allocz(1) +
										MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = 0;
	req->resource_id = resource_id;
	req->entry_count = 1;

	mem_req->address = phys_addr;
	mem_req->length = length;

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_resource_attach_backing);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)mem_req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_mem_entry);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = desc[2];

	gpu.virtio_queues[0]->descriptors[desc[2]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[2]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[2]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[2]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)mem_req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

bool virtio_gpu_create_context(uint32_t context_id, char *name) {
	int desc[2] = {0};
	if (!virtio_alloc_desc(gpu.virtio_queues[0], desc, 2)) {
		kprintf("Virtio GPU out of descriptors!\n");
		return false;
	}

	struct virtio_gpu_context_create *req =
		(struct virtio_gpu_context_create *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);
	volatile struct virtio_gpu_control_header *resp =
		(struct virtio_gpu_control_header *)((uintptr_t)pmm_allocz(1) +
											 MEM_PHYS_OFFSET);

	req->header.type = VIRTIO_GPU_CMD_CTX_CREATE;
	req->header.flags = 0;
	req->header.fence_id = 0;
	req->header.context_id = context_id;
	req->name_length = strlen(name);
	strncpy(req->debug_name, name, 64);

	gpu.virtio_queues[0]->descriptors[desc[0]].address =
		(uintptr_t)req - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[0]].flags = VIRTIO_QUEUE_DESC_F_NEXT;
	gpu.virtio_queues[0]->descriptors[desc[0]].length =
		sizeof(struct virtio_gpu_context_create);
	gpu.virtio_queues[0]->descriptors[desc[0]].next = desc[1];

	gpu.virtio_queues[0]->descriptors[desc[1]].address =
		(uintptr_t)resp - MEM_PHYS_OFFSET;
	gpu.virtio_queues[0]->descriptors[desc[1]].flags =
		VIRTIO_QUEUE_DESC_F_WRITE;
	gpu.virtio_queues[0]->descriptors[desc[1]].length =
		sizeof(struct virtio_gpu_control_header);
	gpu.virtio_queues[0]->descriptors[desc[1]].next = 0;

	virtio_queue_avail_insert(gpu.virtio_queues[0], desc[0]);
	virtio_queue_notify_wait(&gpu, gpu.virtio_queues[0]);

	bool ret = (resp->type == VIRTIO_GPU_RESP_OK_NODATA);

	pmm_free((void *)((uintptr_t)req - MEM_PHYS_OFFSET), 1);
	pmm_free((void *)((uintptr_t)resp - MEM_PHYS_OFFSET), 1);
	virtio_free_desc(gpu.virtio_queues[0], desc[0]);

	return ret;
}

void driver_exit(void) {
	kprintf("Bye bye!\n");
}

uint64_t driver_entry(struct module *driver_module) {
	driver_module->exit = driver_exit;

	gpu.pci_dev = pci_get_pci_device(VIRTIO_VENDOR_ID, VIRTIO_GPU_DEVICE_ID);

	if (gpu.pci_dev == NULL) {
		kprintf("No Virtio GPU card installed!\n");
		return 2;
	}

	uint16_t pci_cmd_ret = PCI_READ_W(gpu.pci_dev, 0x04);

	pci_cmd_ret &= ~0b111;
	// Enable MMIO and Bus mastering.
	pci_cmd_ret |= ((1 << 1) | (1 << 2)) & 0b111;

	PCI_WRITE_W(gpu.pci_dev, 0x4, pci_cmd_ret);

	int cap_ptr = PCI_READ_B(gpu.pci_dev, 0x34);
	while (cap_ptr) {
		if (PCI_READ_B(gpu.pci_dev, cap_ptr) == 9) {
			uint8_t type = PCI_READ_B(gpu.pci_dev, cap_ptr + CAP_TYPE);
			uint8_t bar_num = PCI_READ_B(gpu.pci_dev, cap_ptr + CAP_BAR);
			uint32_t bar_offset =
				PCI_READ_D(gpu.pci_dev, cap_ptr + CAP_BAR_OFFSET);
			if (!pci_get_bar_n(gpu.pci_dev, &gpu.pci_bars[bar_num], bar_num)) {
				kprintf("Failed to get BAR%d\n", bar_num);
				return 2;
			}
			switch (type) {
				case CAP_TYPE_COMMON_CONFIG:
					if (!gpu.pci_bars[bar_num].is_mmio) {
						kprintf("PCI device is not MMIO\n");
						return 2;
					}
					gpu.common_cfg_bar = bar_num;
					gpu.common_cfg_pci_offset = bar_offset;
					gpu.common_config =
						(struct virtio_common_config
							 *)(gpu.pci_bars[bar_num].base + bar_offset);
					break;
				case CAP_TYPE_ISR_CONFIG:
					if (!gpu.pci_bars[bar_num].is_mmio) {
						kprintf("PCI device is not MMIO\n");
						return 2;
					}
					gpu.isr_status =
						(uint32_t *)(gpu.pci_bars[bar_num].base + bar_offset);
					break;
				case CAP_TYPE_DEVICE_CONFIG:
					if (!gpu.pci_bars[bar_num].is_mmio) {
						kprintf("PCI device is not MMIO\n");
						return 2;
					}
					gpu.device_cfg_bar = bar_num;
					gpu.device_cfg_pci_offset = bar_offset;
					gpu.gpu_config =
						(struct virtio_gpu_config
							 *)(gpu.pci_bars[bar_num].base + bar_offset);
					break;
				case CAP_TYPE_NOTIFICATION:
					if (!gpu.pci_bars[bar_num].is_mmio) {
						kprintf("PCI device is not MMIO\n");
						return 2;
					}
					gpu.notify_bar = bar_num;
					gpu.notify_pci_offset = bar_offset;
					gpu.notify =
						(uint16_t *)(gpu.pci_bars[bar_num].base + bar_offset);
					gpu.notify_multiplier = PCI_READ_D(
						gpu.pci_dev, cap_ptr + CAP_NOTIFICATION_MULTIPLIER);
					break;
			}
		}
		cap_ptr = PCI_READ_B(gpu.pci_dev, cap_ptr + 1);
	}

	if (!gpu.common_config || !gpu.gpu_config) {
		kprintf("Failed to get device config\n");
		return 2;
	}

	// Reset the GPU
	gpu.common_config->status = 0;
	while (gpu.common_config->status != 0) {
		pause();
	}

	// Yes we know the device exists and we know how to drive the GPU
	VIRTIO_CONFIG_STATUS_SET(
		gpu.common_config->status,
		(VIRTIO_CONFIG_STATUS_ACK | VIRTIO_CONFIG_STATUS_DRIVER));

	// We want VirGL 3D
	gpu.common_config->device_feature_select = 1;
	uint32_t device_features_hi = gpu.common_config->device_feature;
	gpu.common_config->device_feature_select = 0;
	uint32_t device_features_low = gpu.common_config->device_feature;

	uint64_t device_features =
		((uint64_t)device_features_hi << 32) | device_features_low;

	if (!(device_features & VIRTIO_GPU_VIRGL_SUPPORTED)) {
		kprintf("This Virtio GPU does not support VirGL :(\n");
		return 3;
	}

	gpu.common_config->driver_feature_select = 0;
	gpu.common_config->driver_feature = 1;

	gpu.common_config->driver_feature_select = 1;
	gpu.common_config->driver_feature = 1;

	// We are fine with these
	VIRTIO_CONFIG_STATUS_SET(gpu.common_config->status,
							 (VIRTIO_CONFIG_STATUS_ACK |
							  VIRTIO_CONFIG_STATUS_DRIVER |
							  VIRTIO_CONFIG_STATUS_FEATURES_OK));

	if (!(gpu.common_config->status & VIRTIO_CONFIG_STATUS_FEATURES_OK)) {
		kprintf("This Virtio GPU is NOT fine with our choices :(\n");
		return 3;
	}

	virtio_setup_queue(&gpu, 0);
	virtio_setup_queue(&gpu, 1);

	VIRTIO_CONFIG_STATUS_SET(gpu.common_config->status,
							 (VIRTIO_CONFIG_STATUS_ACK |
							  VIRTIO_CONFIG_STATUS_DRIVER |
							  VIRTIO_CONFIG_STATUS_DRIVER_OK));

	// Resource IDs start from 1
	gpu.resource_id = 1;

	struct virtio_gpu_mode current_mode = {0};
	uint32_t scanout_id = 0;
	if (virtio_gpu_get_display_info(&current_mode, &scanout_id)) {
		kprintf("Running at %ux%u with scanout_id: %u\n",
				current_mode.rect.width, current_mode.rect.height, scanout_id);
	}

	uint32_t width = current_mode.rect.width;
	uint32_t height = current_mode.rect.height;

	uint32_t x = 0;
	uint32_t y = 0;

	if (!virtio_gpu_create_2d_resource(
			gpu.resource_id, VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM, width, height)) {
		kprintf("Failed to create 2D resource :((\n");
		return 4;
	}

	kprintf("Created 2D resource of %ux%u\n", width, height);

	uint32_t *backing =
		(uint32_t *)((uintptr_t)pmm_allocz(((width * height * 4) / PAGE_SIZE) +
										   1) +
					 MEM_PHYS_OFFSET);

	if (!virtio_gpu_attach_backing(gpu.resource_id,
								   (uintptr_t)backing - MEM_PHYS_OFFSET,
								   width * height * 4)) {
		kprintf("Failed to attach a backing to the resource :((\n");
		return 4;
	}

	if (!virtio_gpu_setup_scanout(scanout_id, gpu.resource_id, x, y, width,
								  height)) {
		kprintf("Failed to setup scanout :((\n");
		return 4;
	}

	// black out!
	memset(backing, 0x00, width * height * 4);

	if (!virtio_gpu_transfer_2d_resource_to_gpu(gpu.resource_id, x, y, width,
												height)) {
		kprintf("Failed to transfer resource to the GPU :((\n");
		return 4;
	}

	if (!virtio_gpu_flush(gpu.resource_id, x, y, width, height)) {
		kprintf("Failed to flush :((\n");
		return 4;
	}

	return 0;
}
