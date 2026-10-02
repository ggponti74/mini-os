ENTRY(stage2_entry)

SECTIONS
{
    . = 0x7E00;    /* MUST BE 0x7E00, NOT 0x7C00 */

    .text : ALIGN(4)
    {
        *kernel_entry.o(.text)
        *(.text*)
    }

    .rodata : ALIGN(4)
    {
        *(.rodata*)
    }

    .data : ALIGN(4)
    {
        *(.data*)
    }

    .bss : ALIGN(4)
    {
        *(COMMON)
        *(.bss*)
    }
}