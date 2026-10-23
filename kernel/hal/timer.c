#include "timer.h"

#include "hal/io.h"
#include "pic.h"

#define PIT_INPUT_HZ 1193180

static volatile uint32_t timer_ticks;

void hal_timer_init(uint32_t frequency_hz) {
    if (frequency_hz == 0 || frequency_hz > PIT_INPUT_HZ) {
        return;
    }

    uint32_t divisor = PIT_INPUT_HZ / frequency_hz;
    hal_outb(0x43, 0x36);
    hal_outb(0x40, (uint8_t)(divisor & 0xFF));
    hal_outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    timer_ticks = 0;
    irq_unmask(0);
}

void hal_timer_irq_handler(void) {
    timer_ticks++;
    pic_send_eoi(0);
}

uint64_t hal_timer_get_ticks(void) {
    return timer_ticks;
}

void hal_sleep(uint32_t milliseconds) {
    uint32_t start = timer_ticks;
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));

    if (eflags & (1 << 9)) {
        while ((uint32_t)(timer_ticks - start) < milliseconds) {
            __asm__ volatile("hlt" ::: "memory");
        }
    } else {
        while ((uint32_t)(timer_ticks - start) < milliseconds) {
            __asm__ volatile("sti; hlt; cli" ::: "memory");
        }
    }
}
