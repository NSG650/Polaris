#ifndef APIC_H
#define APIC_H

#include <stddef.h>
#include <stdint.h>

void lapic_init(uint8_t cpu_id);
uint8_t lapic_get_id(void);

void apic_send_ipi(uint8_t lapic_id, uint8_t vector);
void apic_init(void);
void apic_eoi(void);
void apic_timer_init(void);

void pic_init(void);

void ioapic_redirect_irq(uint32_t irq, uint8_t vect);

#endif
