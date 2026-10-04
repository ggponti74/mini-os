ENTRY(stage2_entry)

SECTIONS
{
    . = 0x7E00;

    .text : ALIGN(4)
    {
        KEEP(*(.boot_entry))
        *kernel_entry.o(.text*)
        *(.text*)
    }

    .rodata : ALIGN(4) { *(.rodata*) }
    .data   : ALIGN(4) { *(.data*) }
    .bss    : ALIGN(4) { *(COMMON) *(.bss*) }

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
        *(.eh_frame*)
    }
}
