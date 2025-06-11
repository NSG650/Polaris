#include "i8254x.h"
#include <debug/debug.h>
#include <io/mmio.h>
#include <io/pci.h>
#include <klibc/mem.h>
#include <klibc/misc.h>
#include <klibc/module.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <net/net.h>
#include <sched/sched.h>
#include <sys/prcb.h>

struct i8254x_device {
	uintptr_t mmio_address;
	uint8_t mac_addr[6];

	volatile uint8_t *rx_desc_base;
	volatile i8254x_rx_desc_t *rx_desc[NUM_RX_DESCRIPTORS];
	volatile uint16_t rx_tail;

	volatile uint8_t *tx_desc_base;
	volatile i8254x_tx_desc_t *tx_desc[NUM_TX_DESCRIPTORS];
	volatile uint16_t tx_tail;
};

struct net_nic_interfaces nic_i8254x = {0};
struct i8254x_device i8254x_dev = {0};
struct pci_device *i8254x_pci_device = NULL;

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

static uint16_t i8254x_eeprom_read(struct i8254x_device *dev, uint8_t addr) {
	uint16_t data = 0;
	uint32_t tmp = 0;

	mmoutd(I8254X_REG_EERD(dev->mmio_address), 1 | (uint32_t)addr << 8);

	while (!((tmp = mmind(I8254X_REG_EERD(dev->mmio_address))) & (1 << 4)))
		pause();

	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}

static void i8254x_tx_init(struct i8254x_device *dev) {
	dev->tx_desc_base =
		(void *)((uintptr_t)pmm_allocz(
					 ALIGN_UP(sizeof(i8254x_tx_desc_t) * NUM_TX_DESCRIPTORS,
							  PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);

	for (int i = 0; i < NUM_TX_DESCRIPTORS; i++) {
		dev->tx_desc[i] = (i8254x_tx_desc_t *)(dev->tx_desc_base +
											   (i * sizeof(i8254x_tx_desc_t)));
		dev->tx_desc[i]->address = 0;
		dev->tx_desc[i]->cmd = 0;
	}

	// setup the TX desc address
	mmoutd(I8254X_REG_TDBAH(dev->mmio_address),
		   (uint32_t)(((uint64_t)dev->tx_desc_base - MEM_PHYS_OFFSET) >> 32));
	mmoutd(I8254X_REG_TDBAL(dev->mmio_address),
		   (uint32_t)(((uint64_t)dev->tx_desc_base - MEM_PHYS_OFFSET) &
					  0xFFFFFFFF));

	// TX buffer length
	mmoutd(I8254X_REG_TDLEN(dev->mmio_address),
		   (uint32_t)(NUM_TX_DESCRIPTORS * 16));

	// Head and Tail pointer
	mmoutd(I8254X_REG_TDH(dev->mmio_address), 0);
	mmoutd(I8254X_REG_TDT(dev->mmio_address), NUM_TX_DESCRIPTORS - 1);

	dev->tx_tail = 0;

	mmoutd(I8254X_REG_TCTL(dev->mmio_address), (TCTL_EN | TCTL_PSP));
}

static void i8254x_rx_init(struct i8254x_device *dev) {
	dev->rx_desc_base =
		(void *)((uintptr_t)pmm_allocz(
					 ALIGN_UP(sizeof(i8254x_rx_desc_t) * NUM_RX_DESCRIPTORS,
							  PAGE_SIZE)) +
				 MEM_PHYS_OFFSET);

	for (int i = 0; i < NUM_RX_DESCRIPTORS; i++) {
		dev->rx_desc[i] = (i8254x_rx_desc_t *)(dev->rx_desc_base +
											   (i * sizeof(i8254x_rx_desc_t)));
		dev->rx_desc[i]->address = (uint64_t)pmm_allocz(3);
		dev->rx_desc[i]->status = 0;
	}

	// setup the RX desc address
	mmoutd(I8254X_REG_RDBAH(dev->mmio_address),
		   (uint32_t)(((uint64_t)dev->rx_desc_base - MEM_PHYS_OFFSET) >> 32));
	mmoutd(I8254X_REG_RDBAL(dev->mmio_address),
		   (uint32_t)(((uint64_t)dev->rx_desc_base - MEM_PHYS_OFFSET) &
					  0xFFFFFFFF));

	// RX buffer length
	mmoutd(I8254X_REG_RDLEN(dev->mmio_address),
		   (uint32_t)(NUM_RX_DESCRIPTORS * 16));

	// Head and Tail pointer
	mmoutd(I8254X_REG_RDH(dev->mmio_address), 0);
	mmoutd(I8254X_REG_RDT(dev->mmio_address), NUM_RX_DESCRIPTORS - 1);

	dev->rx_tail = 0;

	mmoutd(I8254X_REG_RCTL(dev->mmio_address),
		   (RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RDMTS_HALF | RCTL_SECRC |
			RCTL_LPE | RCTL_BAM | RCTL_BSIZE_8192));
}

static void i8254x_tx_packet(struct i8254x_device *dev, void *packet,
							 uint16_t length) {
	dev->tx_desc[dev->tx_tail]->address = (uint64_t)packet;
	dev->tx_desc[dev->tx_tail]->length = length;
	dev->tx_desc[dev->tx_tail]->cmd = ((1 << 3) | (3));

	int tail = dev->tx_tail;

	dev->tx_tail = (dev->tx_tail + 1) % NUM_TX_DESCRIPTORS;
	mmoutd(I8254X_REG_TDT(dev->mmio_address), dev->tx_tail);

	while (!(dev->tx_desc[tail]->sta & 0xf)) {
		pause();
	}
}

static void i8254x_rx_packet(struct i8254x_device *dev) {
	while ((dev->rx_desc[dev->rx_tail]->status & (1 << 0))) {
		uint8_t *packet = (void *)(dev->rx_desc[dev->rx_tail]->address);
		uint16_t packet_length = dev->rx_desc[dev->rx_tail]->length;

		bool drop = false;

		if (packet_length < 60) {
			kprintf("I8254X: This packet (%u) is too short!\n", packet_length);
			drop = true;
		}

		if (!(dev->rx_desc[dev->rx_tail]->status & (1 << 1))) {
			kprintf("I8254X: No End of Packet set!\n");
			drop = true;
		}

		if (dev->rx_desc[dev->rx_tail]->errors) {
			kprintf("I8254X: RX Errors: 0x%x\n",
					dev->rx_desc[dev->rx_tail]->errors);
			drop = true;
		}

		if (!drop) {
			uint8_t *packet_pass = kmalloc(packet_length);
			uint64_t *handover = kmalloc(sizeof(uint64_t) * 3);
			memcpy(packet_pass, (void *)((uintptr_t)packet + MEM_PHYS_OFFSET),
				   packet_length);
			handover[0] = (uint64_t)packet_pass;
			handover[1] = packet_length;
			handover[2] = (uint64_t)&nic_i8254x;

			thread_create((uintptr_t)net_handle_packet_thread,
						  (uint64_t)handover, false, process_list);
		}

		dev->rx_desc[dev->rx_tail]->status = 0;
		int tail = dev->tx_tail;
		dev->rx_tail = (dev->rx_tail + 1) % NUM_RX_DESCRIPTORS;

		mmoutd(I8254X_REG_RDT(dev->mmio_address), tail);
	}
}

static void i8254x_handler(registers_t *reg) {
	cli();
	uint32_t icr = mmind((void *)(i8254x_dev.mmio_address + 0xc0));

	// Clear TX success bit
	icr &= ~(3);

	// LINK status change
	if (icr & (1 << 2)) {
		icr &= ~(1 << 2);
		mmoutd(I8254X_REG_CTRL(i8254x_dev.mmio_address),
			   (mmind(I8254X_REG_CTRL(i8254x_dev.mmio_address)) | CTRL_SLU |
				CTRL_ASDE));
	}

	// RX underrun / min threshold
	if (icr & (1 << 6) || icr & (1 << 4)) {
		icr &= ~((1 << 6) | (1 << 4));
		i8254x_rx_packet(&i8254x_dev);
	}

	// packet is pending
	if (icr & (1 << 7)) {
		icr &= ~(1 << 7);
		i8254x_rx_packet(&i8254x_dev);
	}

	if (icr)
		kprintf("I8254X: Unhandled interrupt: 0x%x!\n", icr);

	// clearing the pending interrupts
	mmind((void *)(i8254x_dev.mmio_address + 0xc0));
	sti();

	apic_eoi();
}

uint8_t *i8254x_get_mac_addr(void) {
	return i8254x_dev.mac_addr;
}

void i8254x_send_packet(uint8_t *dest, void *packet, uint32_t packet_length,
						uint16_t protocol) {
	struct network_packet *data =
		(struct network_packet *)((uintptr_t)pmm_allocz(
									  ALIGN_UP(sizeof(struct network_packet) +
												   packet_length,
											   PAGE_SIZE)) +
								  MEM_PHYS_OFFSET);
	uint32_t data_length = sizeof(struct network_packet) + packet_length;

	uint8_t *actual_data = (uint8_t *)((uintptr_t)data->data);

	memcpy((void *)(data->destination_mac), dest, 6);
	memcpy((void *)(data->source_mac), i8254x_dev.mac_addr, 6);
	memcpy(actual_data, packet, packet_length);

	data->type = BSWAP16(protocol);

	i8254x_tx_packet(&i8254x_dev, (void *)((uintptr_t)data - MEM_PHYS_OFFSET),
					 (uint16_t)data_length);

	pmm_free((void *)((uintptr_t)data - MEM_PHYS_OFFSET),
			 ALIGN_UP(data_length, PAGE_SIZE));
}

uint64_t driver_entry(struct module *driver_module) {
	for (int i = 0; i < 25; i++) {
		i8254x_pci_device =
			pci_get_pci_device(I8254X_VENDOR_ID, I8254X_DEVICE_IDS[i]);
		if (i8254x_pci_device) {
			break;
		}
	}

	if (i8254x_pci_device == NULL) {
		kprintf("I8254X: No I8254X card found!\n");
		return 2;
	}

	struct pci_bar bar = {0};
	if (!pci_get_bar_n(i8254x_pci_device, &bar, 0)) {
		kprintf("I8254X: Failed to get PCI Bar 0!\n");
		return 2;
	}

	if (bar.is_mmio == false) {
		kprintf("I8254X: The card is not MMIO??\n");
		return 2;
	}

	// Enable bus mastering, mmio and pio
	PCI_WRITE_W(i8254x_pci_device, 0x04, 0x04 | 0x02 | 0x01);

	i8254x_dev.mmio_address = bar.base;

	// Reset the controller
	mmoutd(I8254X_REG_CTRL(i8254x_dev.mmio_address),
		   (mmind(I8254X_REG_CTRL(i8254x_dev.mmio_address)) | (1 << 26)));
	while (mmind(I8254X_REG_CTRL(i8254x_dev.mmio_address)) & (1 << 26))
		pause();

	uint16_t mac = i8254x_eeprom_read(&i8254x_dev, 0);
	i8254x_dev.mac_addr[0] = mac & 0xff;
	i8254x_dev.mac_addr[1] = (mac >> 8) & 0xff;

	mac = i8254x_eeprom_read(&i8254x_dev, 1);
	i8254x_dev.mac_addr[2] = mac & 0xff;
	i8254x_dev.mac_addr[3] = (mac >> 8) & 0xff;

	mac = i8254x_eeprom_read(&i8254x_dev, 2);
	i8254x_dev.mac_addr[4] = mac & 0xff;
	i8254x_dev.mac_addr[5] = (mac >> 8) & 0xff;

	uint32_t rar_low = ((uint32_t)i8254x_dev.mac_addr[0]) |
					   ((uint32_t)i8254x_dev.mac_addr[1] << 8) |
					   ((uint32_t)i8254x_dev.mac_addr[2] << 16) |
					   ((uint32_t)i8254x_dev.mac_addr[3] << 24);
	uint32_t rar_high = ((uint32_t)i8254x_dev.mac_addr[4]) |
						((uint32_t)i8254x_dev.mac_addr[5] << 8) |
						RAH_AV; // Set the Address Valid bit

	mmoutd(I8254X_REG_RAL(i8254x_dev.mmio_address), rar_low);
	mmoutd(I8254X_REG_RAH(i8254x_dev.mmio_address), rar_high);

	uint8_t irq_number = PCI_READ_B(i8254x_pci_device, 0x3c);
	kprintf("I8254X: Got IRQ number %d\n", irq_number);
	ioapic_redirect_irq(irq_number, 48 + irq_number);
	isr_register_handler(48 + irq_number, i8254x_handler);

	// LINK UP
	mmoutd(I8254X_REG_CTRL(i8254x_dev.mmio_address),
		   (mmind(I8254X_REG_CTRL(i8254x_dev.mmio_address)) | CTRL_SLU |
			CTRL_ASDE));
	// Initialize multicast array
	for (int i = 0; i < 128; i++) {
		mmoutd(I8254X_REG_MTA(i8254x_dev.mmio_address) + i * 4, 0);
	}

	i8254x_rx_init(&i8254x_dev);
	i8254x_tx_init(&i8254x_dev);

	// Enable interrupts
	mmoutd(I8254X_REG_IMS(i8254x_dev.mmio_address), 0x1f6dc);
	mmoutd(I8254X_REG_IMS(i8254x_dev.mmio_address), 0xff & ~4);
	mmind((void *)(i8254x_dev.mmio_address + 0xc0));

	kprintf("I8254X: Got Mac Addr %x:%x:%x:%x:%x:%x\n", i8254x_dev.mac_addr[0],
			i8254x_dev.mac_addr[1], i8254x_dev.mac_addr[2],
			i8254x_dev.mac_addr[3], i8254x_dev.mac_addr[4],
			i8254x_dev.mac_addr[5]);

	nic_i8254x.get_mac_addr = i8254x_get_mac_addr;
	nic_i8254x.send_packet = i8254x_send_packet;
	return 0;
}
