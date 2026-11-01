// src/sys/metrics.c
#include "metrics.h"
#include "memory.h"

mem_stats_t sys_get_mem_stats(void) {
    mem_stats_t stats;
    // The PMM tracks memory in 4 KB pages.
    stats.total_kb = total_memory_pages * 4;
    stats.free_kb = free_memory_pages * 4;
    stats.used_kb = stats.total_kb - stats.free_kb;
    return stats;
}

// Sample over 100 ms at the configured 1 kHz PIT rate.
#define CPU_SAMPLE_TICKS 100

static volatile uint32_t total_ticks = 0;
static volatile uint32_t idle_ticks = 0;
static volatile uint32_t current_cpu_usage = 0;

void cpu_idle_hook(void) {
    idle_ticks++;
    __asm__ volatile("hlt");
}

void sys_timer_tick(void) {
    total_ticks++;

    if (total_ticks >= CPU_SAMPLE_TICKS) {
        if (idle_ticks > total_ticks) idle_ticks = total_ticks;

        uint32_t idle_pct = (idle_ticks * 100) / total_ticks;
        current_cpu_usage = (idle_pct > 100) ? 0 : (100 - idle_pct);

        total_ticks = 0;
        idle_ticks = 0;
    }
}

uint32_t sys_get_cpu_usage(void) {
    return current_cpu_usage;
}