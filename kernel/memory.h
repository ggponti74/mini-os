#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

/* --- Physical Page Frame Allocator --- */

/**
 * @brief Initializes the physical memory manager with a given RAM size.
 * @param ram_size Total physical memory size in bytes.
 * @param kernel_end The first physical address after the kernel image.
 */
void pmm_init(uintptr_t ram_size, uintptr_t kernel_end);

/**
 * @brief Allocates a single physical page frame.
 * @return Physical address of the allocated page, or 0 if out of memory.
 */
uintptr_t pmm_alloc_frame(void);

/**
 * @brief Frees a previously allocated physical page frame.
 * @param frame_addr Physical address of the frame to free.
 */
void pmm_free_frame(uintptr_t frame_addr);


/* --- Kernel Heap Allocator (Simple Bump Allocator) --- */

/**
 * @brief Initializes the kernel heap space.
 * @param heap_start Starting virtual/physical address of the heap.
 * @param heap_size Total size allocated for the kernel heap.
 */
void kheap_init(uintptr_t heap_start, size_t heap_size);

/**
 * @brief Allocates a block of memory from the kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory, or NULL if out of memory.
 */
void* kmalloc(size_t size);

/**
 * @brief Frees a block of memory. (Stubbed out for simple bump allocation)
 * @param ptr Pointer to the memory block to free.
 */
void kfree(void* ptr);

#endif /* MEMORY_H */
