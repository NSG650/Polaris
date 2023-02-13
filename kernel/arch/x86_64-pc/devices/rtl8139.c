#include <asm/asm.h>
#include <debug/debug.h>
#include <devices/rtl8139.h>
#include <io/pci.h>
#include <io/ports.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/apic.h>
#include <sys/isr.h>

struct rtl8139_device {
	uint16_t io_base;
	uint8_t mac_addr[6];
	uint8_t *rx_buffer;
};

struct rtl8139_device rtl8139_dev = {0};
struct pci_device *rtl8139_pci_device = NULL;

void rtl8139_handler(registers_t *reg) {
	// kprintf("RTL8139: WE GOT SOMETHING!!!\n");
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

bool rtl8139_init(void) {
	rtl8139_pci_device =
		pci_get_pci_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
	if (rtl8139_pci_device == NULL)
		return false;

	uint32_t pci_read_ret = pci_read(0, rtl8139_pci_device->bus,
									 rtl8139_pci_device->device, 0, 0x10, 4);

	rtl8139_dev.io_base = pci_read_ret & (~0x3);

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

	rtl8139_get_mac_addr(&rtl8139_dev);
	rtl8139_dev.rx_buffer = pmm_allocz(RX_BUFFER_SIZE / PAGE_SIZE);

	// set the rx buffer to be used
	// we need a physical address which is why we use pmm_allocz
	outd(rtl8139_dev.io_base + RTL8139_REG_RBSTART,
		 (uint32_t)rtl8139_dev.rx_buffer);

	// Sets the TOK and ROK bits high
	outw(rtl8139_dev.io_base + 0x3C, 0x0005);

	// (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
	outd(rtl8139_dev.io_base + 0x44, 0xf | (1 << 7));

	// Sets the RE and TE bits high
	outb(rtl8139_dev.io_base + 0x37, 0x0C);

	uint8_t irq_number = pci_read(0, rtl8139_pci_device->bus,
								  rtl8139_pci_device->device, 0, 0x3C, 1);
	kprintf("RTL8139: Got IRQ number %d\n", irq_number);

	isr_register_handler(48 + irq_number, rtl8139_handler);
	ioapic_redirect_irq(irq_number, irq_number + 48);

	return true;
}