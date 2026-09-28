ENTRY(stage2_entry)

SECTIONS
{
    . = 0x7E00;

    .text : ALIGN(4K)
    {
        /* Force kernel_entry.o (.text) to be placed first */
        *kernel_entry.o(.text)
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}