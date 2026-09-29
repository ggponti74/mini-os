// hal/interrupts.h
#ifndef HAL_INTERRUPTS_H
#define HAL_INTERRUPTS_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

// Core Control
void hal_interrupts_enable(void);   // wraps sti
void hal_interrupts_disable(void);  // wraps cli

// Controller & Handler Management
void hal_interrupts_init(void);     // Remaps PIC/APIC and sets up IDT
void hal_register_irq_handler(uint8_t irq, irq_handler_t handler);
void hal_irq_ack(uint8_t irq);       // Sends End of Interrupt (EOI)

#endif