#ifndef NVME_H
#define NVME_H

#include <fs/devtmpfs.h>
#include <io/pci.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

// Ripped from lyre, I ain't got time for this
struct nvme_id {
	uint16_t vid;
	uint16_t ssvid;
	char sn[20];
	char mn[40];
	char fr[8];
	uint8_t rab;
	uint8_t ieee[3];
	uint8_t mic;
	uint8_t mdts;
	uint16_t ctrlid;
	uint32_t version;
	uint32_t unused1[43];
	uint16_t oacs;
	uint8_t acl;
	uint8_t aerl;
	uint8_t fw; // firmware
	uint8_t lpa;
	uint8_t elpe;
	uint8_t npss;
	uint8_t avscc;
	uint8_t apsta;
	uint16_t wctemp;
	uint16_t cctemp;
	uint16_t unused2[121];
	uint8_t sqes;
	uint8_t cqes;
	uint16_t unused3;
	uint32_t nn;
	uint16_t oncs;
	uint16_t fuses;
	uint8_t fna;
	uint8_t vwc;
	uint16_t awun;
	uint16_t awupf;
	uint8_t nvscc;
	uint8_t unused4;
	uint16_t acwu;
	uint16_t unused5;
	uint32_t sgls;
	uint32_t unused6[1401];
	uint8_t vs[1024];
};

struct nvme_cmd {
	union {
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t nsid;
			uint32_t cdw1[2];
			uint64_t metadata;
			uint64_t prp1;
			uint64_t prp2;
			uint32_t cdw2[6];
		} common; // generic command
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t nsid;
			uint64_t unused;
			uint64_t metadata;
			uint64_t prp1;
			uint64_t prp2;
			uint64_t slba;
			uint16_t len;
			uint16_t control;
			uint32_t dsmgmt;
			uint32_t ref;
			uint16_t apptag;
			uint16_t appmask;
		} rw; // read or write
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t nsid;
			uint64_t unused1;
			uint64_t unused2;
			uint64_t prp1;
			uint64_t prp2;
			uint32_t cns;
			uint32_t unused3[5];
		} identify; // identify
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t nsid;
			uint64_t unused1;
			uint64_t unused2;
			uint64_t prp1;
			uint64_t prp2;
			uint32_t fid;
			uint32_t dword;
			uint64_t unused[2];
		} features;
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t unused1[5];
			uint64_t prp1;
			uint64_t unused2;
			uint16_t cqid;
			uint16_t size;
			uint16_t cqflags;
			uint16_t irqvec;
			uint64_t unused3[2];
		} createcompq;
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t unused1[5];
			uint64_t prp1;
			uint64_t unused2;
			uint16_t sqid;
			uint16_t size;
			uint16_t sqflags;
			uint16_t cqid;
			uint64_t unused3[2];
		} createsubq;
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t unused1[9];
			uint16_t qid;
			uint16_t unused2;
			uint32_t unused3[5];
		} deleteq;
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t cid;
			uint32_t unused1[9];
			uint16_t sqid;
			uint16_t cqid;
			uint32_t unused2[5];
		} abort;
	};
};

struct nvme_lbaf {
	uint16_t metadata_size;
	uint8_t lba_data_size;
	uint8_t relative_performance;
};

struct nvme_namespace_id {
	uint64_t size;
	uint64_t capabilities;
	uint64_t nuse;
	uint8_t features;
	uint8_t nlbaf;
	uint8_t flbas;
	uint8_t mc;
	uint8_t dpc;
	uint8_t dps;
	uint8_t nmic;
	uint8_t rescap;
	uint8_t fpi;
	uint8_t unused1;
	uint16_t nawun;
	uint16_t nawupf;
	uint16_t nacwu;
	uint16_t nabsn;
	uint16_t nabo;
	uint16_t nabspf;
	uint16_t unused2;
	uint64_t nvmcap[2];
	uint64_t unusued3[5];
	uint8_t nguid[16];
	uint8_t eui64[8];
	struct nvme_lbaf lbaf[16];
	uint64_t unused3[24];
	uint8_t vs[3712];
};

struct nvme_cmd_comp {
	uint32_t result;
	uint32_t reserved;
	uint16_t sq_head;
	uint16_t sq_id;
	uint16_t cmd_id;
	uint16_t status;
};

struct nvme_bar {
	uint64_t capabilities;
	uint32_t version;
	uint32_t interrupt_mask_set;
	uint32_t interrupt_mask_clear;
	uint32_t controller_config;
	uint32_t reserved0;
	uint32_t controller_status;
	uint32_t reserved1;
	uint32_t admin_queue_attrs;
	uint64_t admin_submit_queue;
	uint64_t admin_completion_attrs;
};

struct nvme_queue {
	volatile struct nvme_cmd *submit;
	volatile struct nvme_cmd_comp *completion;
	volatile uint32_t *submit_db;
	volatile uint32_t *complete_db;
	uint16_t elements; // elements in queue
	uint16_t cq_vec;
	uint16_t sq_head;
	uint16_t sq_tail;
	uint16_t cq_head;
	uint8_t cq_phase;
	uint16_t queue_id;	   // queue id
	uint32_t cmd_id;	   // command id
	uint64_t *phys_regpgs; // pointer to the PRPs
};

struct nvme_block_cache {
	uint8_t *cache;
	size_t lba;
	uint8_t status;
	uint64_t hit_count;
	uint64_t last_hit;
};

#define CACHE_FREE 0
#define CACHE_USED 1

struct nvme_namespace_device;

struct nvme_device {
	struct resource res;
	struct pci_device *pci_dev;
	struct pci_bar pci_bar0;
	struct nvme_bar *nvme_bar0;
	size_t stride;
	size_t queue_slots;
	struct nvme_queue admin_queue;
	size_t max_trans_shift;
	size_t namespaces;
	struct nvme_id *id;
	uint32_t *namespace_ids;
	struct nvme_namespace_device *namespace_devices;
	struct event interrupt_event;
};

struct nvme_namespace_device {
	struct resource res;
	struct nvme_device *controller;
	struct nvme_queue queue;
	struct nvme_namespace_id *id;
	size_t namespace_id;
	size_t lba_size;
	size_t lba_count;
	size_t max_prps;
	struct nvme_block_cache cache[512];
};

#define VERSION_MAJOR(v) (((v) >> 16) & 0xffff)
#define VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define VERSION_PATCH(v) ((v) & 0xff)

#define CC_COMMANDSET_NVM (0 << 4)
#define CC_ARBITRATION_ROUNDROBIN (0 << 11)
#define CC_SHUTDOWN_NOTIFICATIONS_NONE (0 << 14)
#define CC_IO_SUBMISSION_QUEUE_SIZE (6 << 16)
#define CC_IO_COMPLETION_QUEUE_SIZE (4 << 20)
#define CC_ENABLE (1)

#define CAP_MAX_ENTRIES(cap) ((cap) & 0xffff)
#define CAP_DOORBELL_STRIDE(cap) (((cap) >> 32) & 0xf)
#define CAP_COMMAND_SET(cap) (((cap) >> 37) & 0xff)
#define CAP_MAX_PAGE_SIZE(cap) (((cap) >> 52) & 0xf)
#define CAP_MIN_PAGE_SIZE(cap) (((cap) >> 48) & 0xf)

// generic
#define NVME_OP_FLUSH 0x00
#define NVME_OP_WRITE 0x01
#define NVME_OP_READ 0x02
// admin
#define NVME_OP_CREATE_SQ 0x01
#define NVME_OP_DEL_CQ 0x04
#define NVME_OP_CREATE_CQ 0x05
#define NVME_OP_IDENTIFY 0x06
#define NVME_OP_ABORT 0x08
#define NVME_OP_SET_FT 0x09
#define NVME_OP_GET_FT 0x0a

#define MAX_NVME_DRIVE_COUNT 32

#define IRQ_BASE_START 52

#endif