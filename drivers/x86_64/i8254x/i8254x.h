#ifndef I8254X_H
#define I8254X_H

#include <stdint.h>

#define NUM_RX_DESCRIPTORS 768
#define NUM_TX_DESCRIPTORS 768

#define CTRL_FD (1 << 0)
#define CTRL_ASDE (1 << 5)
#define CTRL_SLU (1 << 6)

#define I8254X_REG_CTRL(x) (void *)(x + 0x0000)
#define I8254X_REG_STATUS(x) (void *)(x + 0x0008)
#define I8254X_REG_EECD(x) (void *)(x + 0x0010)
#define I8254X_REG_EERD(x) (void *)(x + 0x0014)

#define I8254X_REG_MDIC(x) (void *)(x + 0x0020)

#define I8254X_REG_IMS(x) (void *)(x + 0x00D0)
#define I8254X_REG_RCTL(x) (void *)(x + 0x0100)
#define I8254X_REG_TCTL(x) (void *)(x + 0x0400)

#define I8254X_REG_RDBAL(x) (void *)(x + 0x2800)
#define I8254X_REG_RDBAH(x) (void *)(x + 0x2804)
#define I8254X_REG_RDLEN(x) (void *)(x + 0x2808)
#define I8254X_REG_RDH(x) (void *)(x + 0x2810)
#define I8254X_REG_RDT(x) (void *)(x + 0x2818)
#define I8254X_REG_RDTR(x) (void *)(x + 0x2820)

#define I8254X_REG_TDBAL(x) (void *)(x + 0x3800)
#define I8254X_REG_TDBAH(x) (void *)(x + 0x3804)
#define I8254X_REG_TDLEN(x) (void *)(x + 0x3808)
#define I8254X_REG_TDH(x) (void *)(x + 0x3810)
#define I8254X_REG_TDT(x) (void *)(x + 0x3818)

#define I8254X_REG_MTA(x) (void *)(x + 0x5200)

#define I8254X_REG_RAL(x) (void *)(x + 0x5400)
#define I8254X_REG_RAH(x) (void *)(x + 0x5404)

#define RAH_AV (1U << 31)

// PHY REGISTERS (for use with the MDI/O Interface)
#define I8254X_PHYREG_PCTRL (0)
#define I8254X_PHYREG_PSTATUS (1)
#define I8254X_PHYREG_PSSTAT (17)

#define TCTL_EN (1 << 1)
#define TCTL_PSP (1 << 3)

#define RCTL_EN (1 << 1)
#define RCTL_SBP (1 << 2)
#define RCTL_UPE (1 << 3)
#define RCTL_MPE (1 << 4)
#define RCTL_LPE (1 << 5)
#define RDMTS_HALF (0 << 8)
#define RDMTS_QUARTER (1 << 8)
#define RDMTS_EIGHTH (2 << 8)
#define RCTL_BAM (1 << 15)
#define RCTL_BSIZE_256 (3 << 16)
#define RCTL_BSIZE_512 (2 << 16)
#define RCTL_BSIZE_1024 (1 << 16)
#define RCTL_BSIZE_2048 (0 << 16)
#define RCTL_BSIZE_4096 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384 ((1 << 16) | (1 << 25))
#define RCTL_SECRC (1 << 26)

#define I8254X_VENDOR_ID 0x8086

uint16_t I8254X_DEVICE_IDS[] = {
	0x1004, 0x100e, 0x100f, 0x1010, 0x1011, 0x1012, 0x1013, 0x1015, 0x1016,
	0x1017, 0x1018, 0x1019, 0x101a, 0x101d, 0x1026, 0x1027, 0x1028, 0x1076,
	0x1078, 0x1079, 0x107a, 0x107b, 0x1107, 0x1112, 0x100c};

// RX and TX descriptor structures
typedef struct __attribute__((packed)) i8254x_rx_desc_s {
	volatile uint64_t address;

	volatile uint16_t length;
	volatile uint16_t checksum;
	volatile uint8_t status;
	volatile uint8_t errors;
	volatile uint16_t special;
} i8254x_rx_desc_t;

typedef struct __attribute__((packed)) i8254x_tx_desc_s {
	volatile uint64_t address;

	volatile uint16_t length;
	volatile uint8_t cso;
	volatile uint8_t cmd;
	volatile uint8_t sta;
	volatile uint8_t css;
	volatile uint16_t special;
} i8254x_tx_desc_t;

#endif