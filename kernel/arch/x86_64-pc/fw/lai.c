#include <debug/debug.h>
#include <fw/acpi.h>
#include <io/pci.h>
#include <io/ports.h>
#include <lai/core.h>
#include <lai/drivers/ec.h>
#include <lai/helpers/sci.h>
#include <lai/host.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <sys/timer.h>

extern uint8_t revision;

void laihost_log(int level, const char *msg) {
	switch (level) {
		case LAI_DEBUG_LOG:
			kprintf("LAI: Debug: %s\n", msg);
			break;
		case LAI_WARN_LOG:
			kprintf("LAI: Warning: %s\n", msg);
			break;
		default:
			break;
	}
}

void laihost_panic(const char *msg) {
	panic("LAI: %s\n", msg);
	__builtin_unreachable();
}

void *laihost_malloc(size_t size) {
	return kmalloc(size);
}

void *laihost_realloc(void *ptr, size_t newsize, size_t oldsize) {
	(void)oldsize;
	return krealloc(ptr, newsize);
}

void laihost_free(void *ptr, size_t oldsize) {
	(void)oldsize;
	kfree(ptr);
}

void *laihost_map(size_t address, size_t size) {
	(void)size;
	return (void *)address + MEM_PHYS_OFFSET;
}

void laihost_unmap(void *pointer, size_t count) {
	// We don't want to unmap kernel mapped memory
	(void)pointer;
	(void)count;
}

__attribute__((always_inline)) inline bool is_canonical(uint64_t addr) {
	return ((addr <= 0x00007FFFFFFFFFFF) ||
			((addr >= 0xFFFF800000000000) && (addr <= 0xFFFFFFFFFFFFFFFF)));
}

void *laihost_scan(const char *signature, size_t index) {
	// The DSDT must be found using a pointer in the FADT
	if (!memcmp(signature, "DSDT", 4)) {
		acpi_fadt_t *facp = acpi_find_sdt("FACP", 0);
		uint64_t dsdt_addr = 0;

		if (is_canonical(facp->x_dsdt) && revision >= 2) {
			dsdt_addr = facp->x_dsdt;
		} else {
			dsdt_addr = facp->dsdt;
		}

		return (void *)(dsdt_addr + MEM_PHYS_OFFSET);
	} else {
		return acpi_find_sdt(signature, index);
	}
}

void laihost_outb(uint16_t port, uint8_t val) {
	outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val) {
	outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val) {
	outd(port, val);
}

uint8_t laihost_inb(uint16_t port) {
	return inb(port);
}

uint16_t laihost_inw(uint16_t port) {
	return inw(port);
}

uint32_t laihost_ind(uint16_t port) {
	return ind(port);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint8_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 1);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint16_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 2);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						uint16_t offset, uint32_t val) {
	pci_write(seg, bus, slot, fun, offset, val, 4);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						  uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 1);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						   uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 2);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun,
						   uint16_t offset) {
	return pci_read(seg, bus, slot, fun, offset, 4);
}

void laihost_sleep(uint64_t ms) {
	timer_sleep(ms * 1000);
}

uint64_t laihost_timer(void) {
	return timer_count() / 100000;
}
