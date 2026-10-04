// src/sys/metrics.c
#include "metrics.h"

// Example memory tracker assumptions (hook into your dynamic allocator/pmm)
extern uint32_t total_memory_pages;
extern uint32_t free_memory_pages;

mem_stats_t sys_get_mem_stats(void) {
    mem_stats_t stats;
    // Assuming 4KB pages
    stats.total_kb = total_memory_pages * 4;
    stats.free_kb = free_memory_pages * 4;
    stats.used_kb = stats.total_kb - stats.free_kb;
    return stats;
}

// CPU Usage Tracker
static uint32_t total_ticks = 0;
static uint32_t idle_ticks = 0;
static uint32_t current_cpu_usage = 0;

// Call this function inside your Kernel Idle Loop whenever no tasks are active
void cpu_idle_hook(void) {
    idle_ticks++;
    __asm__ volatile("hlt");
}

// Hook this into PIT IRQ0 handler (e.g. running at 100 Hz)
void sys_timer_tick(void) {
    total_ticks++;

    // Calculate CPU % every 100 ticks (approx 1 second)
    if (total_ticks >= 100) {
        if (idle_ticks > total_ticks) idle_ticks = total_ticks;
        
        // % Usage = 100 - % Idle
        uint32_t idle_pct = (idle_ticks * 100) / total_ticks;
        current_cpu_usage = (idle_pct > 100) ? 0 : (100 - idle_pct);

        // Reset counters for the next sample window
        total_ticks = 0;
        idle_ticks = 0;
    }
}

uint32_t sys_get_cpu_usage(void) {
    return current_cpu_usage;
}