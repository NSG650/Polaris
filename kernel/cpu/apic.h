#ifndef APIC_H
#define APIC_H

#include <stdbool.h>
#include <stdint.h>

void apic_eoi(void);
void apic_init(void);
void apic_timer_init(void);
void ioapic_init(void);
void ioapic_redirect_irq(uint8_t lapic_id, uint8_t irq, uint8_t vect,
						 bool status);

#endif
