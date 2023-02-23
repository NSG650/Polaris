#include <asm/asm.h>
#include <debug/debug.h>
#include <devices/rtl8139.h>
#include <io/pci.h>
#include <io/ports.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <net/net.h>
#include <sys/apic.h>
#include <sys/isr.h>
#include <sys/prcb.h>

struct rtl8139_device {
	uint16_t io_base;
	uint8_t mac_addr[6];
	uint8_t *rx_buffer;
	uint32_t packet_ptr_off;
	uint32_t tx_ptr_off;
};

struct rtl8139_device rtl8139_dev = {0};
struct pci_device *rtl8139_pci_device = NULL;

static void rtl8139_rx_handler(registers_t *reg) {
	if (inb(rtl8139_dev.io_base + RTL8139_REG_CMD) & 1) // the buffer is empty
		return;

	vmm_switch_pagemap(kernel_pagemap);

	uint16_t *packet =
		(uint16_t *)((rtl8139_dev.rx_buffer + rtl8139_dev.packet_ptr_off));

	uint16_t packet_length = *(packet + 1);
	packet += 2; // skip the header

	uint8_t *packet_pass = kmalloc(packet_length);
	memcpy(packet_pass, packet, packet_length);

	net_handle_packet(packet_pass, packet_length);

	kfree(packet_pass);

	rtl8139_dev.packet_ptr_off =
		(rtl8139_dev.packet_ptr_off + packet_length + 4 + 3) & (~3);

	if (rtl8139_dev.packet_ptr_off > RX_BUFFER_SIZE)
		rtl8139_dev.packet_ptr_off -= RX_BUFFER_SIZE;

	outw(rtl8139_dev.io_base + 0x38, rtl8139_dev.packet_ptr_off - 0x10);

	vmm_switch_pagemap(prcb_return_current_cpu()
						   ->running_thread->mother_proc->process_pagemap);
}

void rtl8139_handler(registers_t *reg) {
	uint16_t status = inw(rtl8139_dev.io_base + RTL8139_REG_ISR);

	if (status & (1 << 2)) { // TX
		kprintf("RTL8139: Packet sent\n");
	}

	if (status & (1 << 0)) { // RX
		rtl8139_rx_handler(reg);
	}

	// Tell we got the packet
	outw(rtl8139_dev.io_base + 0x3E, 0x5);

	apic_eoi();
}

static void rtl8139_get_mac_addr(struct rtl8139_device *rtl8139_dev) {
	uint32_t mac_part1 = ind(rtl8139_dev->io_base + RTL8139_REG_MAC05);
	uint16_t mac_part2 = inw(rtl8139_dev->io_base + RTL8139_REG_MAC07);

	rtl8139_dev->mac_addr[0] = mac_part1 >> 0;
	rtl8139_dev->mac_addr[1] = mac_part1 >> 8;
	rtl8139_dev->mac_addr[2] = mac_part1 >> 16;
	rtl8139_dev->mac_addr[3] = mac_part1 >> 24;

	rtl8139_dev->mac_addr[4] = mac_part2 >> 0;
	rtl8139_dev->mac_addr[5] = mac_part2 >> 8;

	kprintf("RTL8139: Got Mac Addr %x:%x:%x:%x:%x:%x\n",
			rtl8139_dev->mac_addr[0], rtl8139_dev->mac_addr[1],
			rtl8139_dev->mac_addr[2], rtl8139_dev->mac_addr[3],
			rtl8139_dev->mac_addr[4], rtl8139_dev->mac_addr[5]);
}

void rtl8139_send_data(uint8_t *data, uint32_t length) {
	uint8_t *t_data = pmm_allocz(length / PAGE_SIZE);
	memcpy(t_data + MEM_PHYS_OFFSET, data, length);

	outd(rtl8139_dev.io_base + 0x20 + (rtl8139_dev.tx_ptr_off * 4),
		 (uint32_t)t_data);
	outd(rtl8139_dev.io_base + 0x10 + (rtl8139_dev.tx_ptr_off * 4),
		 (uint32_t)length);

	rtl8139_dev.tx_ptr_off++;
	if (rtl8139_dev.tx_ptr_off > 3)
		rtl8139_dev.tx_ptr_off = 0;

	pmm_free(t_data, length / PAGE_SIZE);
}

uint8_t *net_get_mac_addr(void) {
	return rtl8139_dev.mac_addr;
}

void net_send_packet(uint8_t *dest, uint8_t *packet, uint32_t packet_length,
					 uint16_t protocol) {
	struct network_packet *data =
		kmalloc(sizeof(struct network_packet) + packet_length);
	uint32_t data_length = sizeof(struct network_packet) + packet_length;

	uint8_t *actual_data = (uint8_t *)((uintptr_t)data->data);

	memcpy(data->destination_mac, dest, 6);
	memcpy(data->source_mac, net_get_mac_addr(), 6);
	memcpy(actual_data, packet, packet_length);

	data->type = BSWAP16(protocol);

	rtl8139_send_data((uint8_t *)data, data_length);

	kfree(data);
}

bool rtl8139_init(void) {
	rtl8139_pci_device =
		pci_get_pci_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
	if (rtl8139_pci_device == NULL)
		return false;

	uint32_t pci_read_ret = pci_read(0, rtl8139_pci_device->bus,
									 rtl8139_pci_device->device, 0, 0x10, 4);

	rtl8139_dev.io_base = pci_read_ret & (~0x3);

    // Enable PCI bus mastering otherwise it won't be able to access the allocated buffer for RX
	uint16_t pci_cmd_ret = pci_read(0, rtl8139_pci_device->bus,
									rtl8139_pci_device->device, 0, 0x04, 2);
	pci_write(0, rtl8139_pci_device->bus, rtl8139_pci_device->device, 0, 0x04,
			  (pci_cmd_ret | (1 << 2)) & (~(1 << 10)),
			  2); // This enables interrupts as well

	if (rtl8139_dev.io_base == 0)
		return false;

	kprintf("RTL8139: Got IO Base at 0x%p\n", rtl8139_dev.io_base);

	// Turning on the RTL8139
	// Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to
	// active high. this should essentially *power on* the device.

	outb(rtl8139_dev.io_base + 0x52, 0x0);

	// Soft reset
	outb(rtl8139_dev.io_base + 0x37, 0x10);
	while ((inb(rtl8139_dev.io_base + 0x37) & 0x10) != 0)
		pause();

	rtl8139_dev.rx_buffer =
		pmm_allocz((RX_BUFFER_SIZE + 1500 + 16) / PAGE_SIZE);

	// set the rx buffer to be used
	// we need a physical address which is why we use pmm_allocz
	outd(rtl8139_dev.io_base + RTL8139_REG_RBSTART,
		 (uint32_t)rtl8139_dev.rx_buffer);

	// Sets the TOK and ROK bits high
	outw(rtl8139_dev.io_base + 0x3C, 0x0005);

	// (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
	outd(rtl8139_dev.io_base + 0x44, 0xf);

	// Sets the RE and TE bits high
	outb(rtl8139_dev.io_base + 0x37, 0x0C);

	uint8_t irq_number = pci_read(0, rtl8139_pci_device->bus,
								  rtl8139_pci_device->device, 0, 0x3C, 1);
	kprintf("RTL8139: Got IRQ number %d\n", irq_number);

	isr_register_handler(48 + irq_number, rtl8139_handler);
	ioapic_redirect_irq(irq_number, irq_number + 48);

	rtl8139_get_mac_addr(&rtl8139_dev);

	return true;
}