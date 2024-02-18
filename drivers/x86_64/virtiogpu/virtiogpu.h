#ifndef VIRTIOGPU_H
#define VIRTIOGPU_H

#include <io/pci.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

struct virtio_common_config {
	uint32_t device_feature_select;
	uint32_t device_feature;
	uint32_t driver_feature_select;
	uint32_t driver_feature;
	uint16_t msix_vector;
	uint16_t queue_count;

	uint8_t status;
	uint8_t generation;

	uint16_t queue_select;
	uint16_t queue_size;
	uint16_t queue_msix_vector;
	uint16_t queue_enable;
	uint16_t queue_notify_offset;
	uint64_t queue_desc;
	uint64_t queue_available;
	uint64_t queue_used;

	uint16_t queue_notify_data;
	uint16_t queue_reset;
};

struct queue_descriptor {
	volatile uint64_t address;
	volatile uint32_t length;
	volatile uint16_t flags;
	volatile uint16_t next;
};

struct queue_available {
	volatile uint16_t flags;
	volatile uint16_t index;
	volatile uint16_t rings[];
};

struct queue_used_element {
	volatile uint32_t index;
	volatile uint32_t length;
};

struct queue_used {
	volatile uint16_t flags;
	volatile uint16_t index;
	volatile struct queue_used_element rings[];
};

struct virtio_queue {
	volatile struct queue_descriptor *descriptors;
	volatile struct queue_available *available;
	volatile struct queue_used *used;
	volatile uint16_t free_list;
	volatile uint16_t free_count;
	uint16_t queue_size;
	uint16_t queue_mask;
	volatile uint16_t last_used_index;
};

struct virtio_gpu_config {
	uint32_t events_read;
	uint32_t events_clear;
	uint32_t num_scanouts;
	uint32_t reserved;
};

// There are 2 queues for the GPU device.
// The Command queue and Cursor queue.
// Thus we will have only 2 virtio queues setup.

struct virtio_gpu_device {
	struct resource res;
	struct pci_device *pci_dev;
	struct pci_bar pci_bars[6];
	uint8_t common_cfg_bar;
	uint8_t device_cfg_bar;
	uint8_t notify_bar;
	uint32_t common_cfg_pci_offset;
	uint32_t device_cfg_pci_offset;
	uint32_t notify_pci_offset;
	volatile struct virtio_common_config *common_config;
	volatile struct virtio_gpu_config *gpu_config;
	volatile uint16_t *notify;
	size_t notify_multiplier;
	struct virtio_queue *virtio_queues[2];
	uint16_t active_queue;
	uint64_t notify_queue0;
	uint64_t notify_queue1;
	uint32_t *isr_status;
	uint32_t resource_id;
};

enum virtio_gpu_control_type {
	/* 2d commands */
	VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
	VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
	VIRTIO_GPU_CMD_RESOURCE_UNREF,
	VIRTIO_GPU_CMD_SET_SCANOUT,
	VIRTIO_GPU_CMD_RESOURCE_FLUSH,
	VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
	VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
	VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
	VIRTIO_GPU_CMD_GET_CAPSET_INFO,
	VIRTIO_GPU_CMD_GET_CAPSET,
	VIRTIO_GPU_CMD_GET_EDID,
	VIRTIO_GPU_CMD_RESOURCE_ASSIGN_UUID,

	/* 3d commands (OpenGL) */
	VIRTIO_GPU_CMD_CTX_CREATE = 0x0200,
	VIRTIO_GPU_CMD_CTX_DESTROY,
	VIRTIO_GPU_CMD_CTX_ATTACH_RESOURCE,
	VIRTIO_GPU_CMD_CTX_DETACH_RESOURCE,
	VIRTIO_GPU_CMD_RESOURCE_CREATE_3D,
	VIRTIO_GPU_CMD_TRANSFER_TO_HOST_3D,
	VIRTIO_GPU_CMD_TRANSFER_FROM_HOST_3D,
	VIRTIO_GPU_CMD_SUBMIT_3D,

	/* cursor commands */
	VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
	VIRTIO_GPU_CMD_MOVE_CURSOR,

	/* success responses */
	VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
	VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
	VIRTIO_GPU_RESP_OK_CAPSET_INFO,
	VIRTIO_GPU_RESP_OK_CAPSET,
	VIRTIO_GPU_RESP_OK_EDID,

	/* error responses */
	VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
	VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
	VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
	VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
	VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
	VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

enum virtio_gpu_formats {
	VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM = 1,
	VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM = 2,
	VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM = 3,
	VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM = 4,

	VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM = 67,
	VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM = 68,

	VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM = 121,
	VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM = 134,
};

struct virtio_gpu_control_header {
	uint32_t type;
	uint32_t flags;
	uint64_t fence_id;
	uint32_t context_id;
	uint32_t padding;
};

struct virtio_gpu_rect {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

struct virtio_gpu_mode {
	struct virtio_gpu_rect rect;
	uint32_t enabled;
	uint32_t flags;
};

struct virtio_gpu_display_info {
	struct virtio_gpu_control_header header;
	struct virtio_gpu_mode modes[16];
};

struct virtio_gpu_resource_create_2d {
	struct virtio_gpu_control_header header;
	uint32_t resource_id;
	uint32_t format;
	uint32_t width;
	uint32_t height;
};

struct virtio_gpu_mem_entry {
	uint64_t address;
	uint32_t length;
	uint32_t padding;
};

struct virtio_gpu_resource_attach_backing {
	struct virtio_gpu_control_header header;
	uint32_t resource_id;
	uint32_t entry_count;
};

struct virtio_gpu_transfer_to_host_2d {
	struct virtio_gpu_control_header header;
	struct virtio_gpu_rect rect;
	uint64_t offset;
	uint32_t resource_id;
	uint32_t padding;
};

struct virtio_gpu_set_scanout {
	struct virtio_gpu_control_header header;
	struct virtio_gpu_rect rect;
	uint32_t scanout_id;
	uint32_t resource_id;
};

struct virtio_gpu_resource_flush {
	struct virtio_gpu_control_header header;
	struct virtio_gpu_rect rect;
	uint32_t resource_id;
	uint32_t padding;
};

struct virtio_gpu_context_create {
	struct virtio_gpu_control_header header;
	uint32_t name_length;
	uint32_t padding;
	char debug_name[64];
};

/* This marks a buffer as continuing via the next field. */
#define VIRTIO_QUEUE_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTIO_QUEUE_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTIO_QUEUE_DESC_F_INDIRECT 4

#define CAP_TYPE 3
#define CAP_BAR 4
#define CAP_BAR_OFFSET 8
#define CAP_NOTIFICATION_MULTIPLIER 16

#define CAP_TYPE_COMMON_CONFIG 1
#define CAP_TYPE_ISR_CONFIG 3
#define CAP_TYPE_NOTIFICATION 2
#define CAP_TYPE_DEVICE_CONFIG 4

#define VIRTIO_VENDOR_ID 0x1AF4
#define VIRTIO_GPU_DEVICE_ID 0x1050
#define VIRTIO_DEVICE_MIN 0x1000
#define VIRTIO_DEVICE_MAX 0x103f

#define VIRTIO_CONFIG_STATUS_SET(x, v) x |= v
#define VIRTIO_CONFIG_STATUS_ACK 1
#define VIRTIO_CONFIG_STATUS_DRIVER 2
#define VIRTIO_CONFIG_STATUS_DRIVER_OK 4
#define VIRTIO_CONFIG_STATUS_FEATURES_OK 8
#define VIRTIO_CONFIG_STATUS_NEEDS_RESET 64
#define VIRTIO_CONFIG_STATUS_FAILED 128

#define VIRTIO_GPU_VIRGL_SUPPORTED 1
#define VIRTIO_GPU_EDID_SUPPORTED 2
#define VIRTIO_GPU_CONTEXT_INIT_SUPPORTED 16

#endif