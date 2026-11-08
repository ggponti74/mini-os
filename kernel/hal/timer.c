#include "timer.h"

#include "hal/io.h"
#include "pic.h"
#include "sys/metrics.h"

#define PIT_INPUT_HZ 1193180

static volatile uint64_t timer_ticks;

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
    sys_timer_tick();
    pic_send_eoi(0);
}

uint64_t hal_timer_get_ticks(void) {
    uint32_t eflags;
    uint64_t ticks;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(eflags) : : "memory");
    ticks = timer_ticks;
    if (eflags & (1u << 9)) __asm__ volatile("sti" ::: "memory");
    return ticks;
}

void hal_sleep(uint32_t milliseconds) {
    uint64_t start = hal_timer_get_ticks();
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));
    if (eflags & (1u << 9)) {
        while (hal_timer_get_ticks() - start < milliseconds)
            __asm__ volatile("hlt" ::: "memory");
    } else {
        while (hal_timer_get_ticks() - start < milliseconds)
            __asm__ volatile("sti; hlt; cli" ::: "memory");
    }
}
