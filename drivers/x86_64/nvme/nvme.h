#ifndef NVME_H
#define NVME_H

#include <fs/devtmpfs.h>
#include <klibc/resource.h>
#include <stddef.h>
#include <stdint.h>

// Ripped from lyre, I ain't got time for this
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
};

#define VERSION_MAJOR(v) (((v) >> 16) & 0xffff)
#define VERSION_MINOR(v) (((v) >> 8) & 0xff)
#define VERSION_PATCH(v) ((v)&0xff)

#define CC_ENABLE(cc) cc = (cc) | 1
#define CC_DISABLE(cc) cc = (cc) & ~1
#define CC_SET_COMMAND_SET(cc, cmd) cc = ((cc) & ~0b1110000) | ((cmd) << 4)
#define CC_SET_PAGE_SIZE(cc, pagesize) \
	cc = ((cc) & ~0b11110000000) | ((pagesize) << 7)
#define CC_SET_ARBITRATION(cc, arbitration) \
	cc = ((cc) & ~0b11100000000000) | ((arbitration) << 11)
#define CC_SET_SUBENTRY_SIZE(cc, size) \
	cc = ((cc) & ~0b11110000000000000000) | ((size) << 16)
#define CC_SET_COMPENTRY_SIZE(cc, size) \
	cc = ((cc) & ~0b111100000000000000000000) | ((size) << 20)
#define CC_COMMANDSET_NVM 0
#define CC_ARBITRATION_ROUNDROBIN 0

#define CAP_MAX_ENTRIES(cap) ((cap)&0xff)
#define CAP_DOORBELL_STRIDE(cap) (((cap) >> 32) & 0xf)
#define CAP_COMMAND_SET(cap) (((cap) >> 37) & 0xff)
#define CAP_MAX_PAGE_SIZE(cap) (((cap) >> 52) & 0xf)
#define CAP_MIN_PAGE_SIZE(cap) (((cap) >> 48) & 0xf)

#endif