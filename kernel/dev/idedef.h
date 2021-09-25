#ifndef IDE_DEF_H
#define IDE_DEF_H
#include <stdint.h>

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

#define ATA_ER_BBK 0x80
#define ATA_ER_UNC 0x40
#define ATA_ER_MC 0x20
#define ATA_ER_IDNF 0x10
#define ATA_ER_MCR 0x08
#define ATA_ER_ABRT 0x04
#define ATA_ER_TK0NF 0x02
#define ATA_ER_AMNF 0x01

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET 0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY 0xEC

#define ATAPI_CMD_READ 0xA8
#define ATAPI_CMD_EJECT 0x1B

#define ATA_IDENT_DEVICETYPE 0
#define ATA_IDENT_CYLINDERS 2
#define ATA_IDENT_HEADS 6
#define ATA_IDENT_SECTORS 12
#define ATA_IDENT_SERIAL 20
#define ATA_IDENT_MODEL 54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID 106
#define ATA_IDENT_MAX_LBA 120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

#define IDE_ATA 0x00
#define IDE_ATAPI 0x01

#define ATA_MASTER 0x00
#define ATA_SLAVE 0x01

#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07
#define ATA_REG_SECCOUNT1 0x08
#define ATA_REG_LBA3 0x09
#define ATA_REG_LBA4 0x0A
#define ATA_REG_LBA5 0x0B
#define ATA_REG_CONTROL 0x0C
#define ATA_REG_ALTSTATUS 0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Channels:
#define ATA_PRIMARY 0x00
#define ATA_SECONDARY 0x01

// Directions:
#define ATA_READ 0x00
#define ATA_WRITE 0x01

struct ide_channel_regs_t {
	uint16_t base;
	uint16_t ctrl;
	uint16_t bmide;
	uint16_t nien;
};

struct ide_device {
	// 0 (Empty) or 1 (This Drive really exists)
	uint8_t exists;
	// 0 (Primary Channel) or 1 (Secondary Channel)
	uint8_t channel;
	// 0 (Master Drive) or 1 (Slave Drive)
	uint8_t drive;
	// 0: ATA, 1:ATAPI
	uint16_t type;
	// Drive Signature
	uint16_t signature;
	// Features
	uint16_t capabilities;
	// Command Sets Supported
	uint32_t command_sets;
	//Size in Sectors
	uint32_t size;
	//Device model
	uint8_t model[41];
};

struct ata_identify_t {
	uint16_t flags;
	uint16_t unused1[9];
	char serial[20];
	uint16_t unused2[3];
	char firmware[8];
	char model[40];
	uint16_t sectors_per_int;
	uint16_t unused3;
	uint16_t capabilities[2];
	uint16_t unused4[2];
	uint16_t valid_ext_data;
	uint16_t unused5[5];
	uint16_t size_of_rw_mult;
	uint32_t sectors_28;
	uint16_t unused6[38];
	uint64_t sectors_48;
	uint16_t unused7[152];
} __attribute__((packed));

#endif
