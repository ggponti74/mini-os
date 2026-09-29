// hal/timer.h
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>

void hal_timer_init(uint32_t frequency_hz);
uint64_t hal_timer_get_ticks(void);
void hal_sleep(uint32_t milliseconds);

#endif