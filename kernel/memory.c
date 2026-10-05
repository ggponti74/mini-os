#include "memory.h"

/* --- Internal PMM State --- */
static uint32_t *pmm_bitmap = NULL;
static size_t pmm_total_frames = 0;
static size_t pmm_bitmap_size = 0;

/* Global metrics counters */
uint32_t total_memory_pages = 0;
uint32_t free_memory_pages = 0;

/* Helper macros for bitmap manipulation */
#define BITMAP_INDEX(frame) ((frame) / 32)
#define BITMAP_OFFSET(frame) ((frame) % 32)

void pmm_init(uintptr_t ram_size, uintptr_t kernel_end)
{
    pmm_total_frames = ram_size / PAGE_SIZE;
    pmm_bitmap_size = pmm_total_frames / 32;
    total_memory_pages = (uint32_t)pmm_total_frames;
    free_memory_pages = 0;

    /* Place the bitmap immediately after the kernel image */
    pmm_bitmap = (uint32_t *)kernel_end;

    /* Initially mark all frames as used/reserved */
    for (size_t i = 0; i < pmm_bitmap_size; i++)
    {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    /* Calculate where usable memory starts (after kernel + bitmap) */
    uintptr_t usable_start = kernel_end + (pmm_bitmap_size * sizeof(uint32_t));
    /* Align to next page boundary */
    usable_start = (usable_start + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
    size_t start_frame = usable_start / PAGE_SIZE;

    /* Free up the usable memory frames for the allocator */
    for (size_t frame = start_frame; frame < pmm_total_frames; frame++)
    {
        pmm_free_frame(frame * PAGE_SIZE);
    }
}

uintptr_t pmm_alloc_frame(void)
{
    for (size_t i = 0; i < pmm_bitmap_size; i++)
    {
        if (pmm_bitmap[i] != 0xFFFFFFFF)
        { /* Has at least one free bit (0) */
            for (int bit = 0; bit < 32; bit++)
            {
                if (!(pmm_bitmap[i] & (1 << bit)))
                {
                    size_t frame = (i * 32) + bit;
                    pmm_bitmap[i] |= (1 << bit); /* Mark as used */
                    if (free_memory_pages > 0)
                    {
                        free_memory_pages--;
                    }
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    return 0; /* Out of memory */
}

void pmm_free_frame(uintptr_t frame_addr)
{
    size_t frame = frame_addr / PAGE_SIZE;
    size_t idx = BITMAP_INDEX(frame);
    size_t off = BITMAP_OFFSET(frame);

    if (idx < pmm_bitmap_size)
    {
        if (pmm_bitmap[idx] & (1 << off))
        {
            pmm_bitmap[idx] &= ~(1 << off); /* Mark as free (0) */
            free_memory_pages++;
        }
    }
}

/* --- Internal Heap State --- */
static uintptr_t heap_curr = 0;
static uintptr_t heap_end = 0;

void kheap_init(uintptr_t heap_start, size_t heap_size)
{
    /* Align heap start address to 8 bytes for data alignment safety */
    heap_curr = (heap_start + 7) & ~7;
    heap_end = heap_curr + heap_size;
}

void *kmalloc(size_t size)
{
    /* Align allocation size to 8-byte boundaries */
    size = (size + 7) & ~7;

    if (heap_curr + size > heap_end)
    {
        return NULL; /* Out of heap space */
    }

    void *allocated_ptr = (void *)heap_curr;
    heap_curr += size; /* "Bump" the pointer forward */

    return allocated_ptr;
}

void kfree(void *ptr)
{
    /* A basic bump allocator cannot free individual blocks.
     * In a minimal system, this is often left as a no-op until
     * a Linked List or Slab allocator is implemented. */
    (void)ptr;
}
