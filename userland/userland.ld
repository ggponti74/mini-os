/* userland/userland.ld */
ENTRY(_start)

SECTIONS
{
    /* Load userland programs at virtual address 16MB (0x01000000)[cite: 2] */
    . = 0x01000000;

    .text : ALIGN(4K) {
        *(.text._start)
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
