// include/sys/metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

// Memory Metrics (provided by physical/virtual memory manager)
typedef struct {
    uint32_t total_kb;
    uint32_t used_kb;
    uint32_t free_kb;
} mem_stats_t;

// Functions to query stats
mem_stats_t sys_get_mem_stats(void);
uint32_t sys_get_cpu_usage(void);

// Call once for each PIT tick and from the kernel's idle loop, respectively.
void sys_timer_tick(void);
void cpu_idle_hook(void);

#endif