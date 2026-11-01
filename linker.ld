ENTRY(stage2_entry)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
    stack PT_GNU_STACK FLAGS(6);
}

SECTIONS
{
    . = 0x7E00;

    .text : ALIGN(4)
    {
        KEEP(*(.boot_entry))
        *kernel_entry.o(.text*)
        *(.text*)
    } :text

    .rodata : ALIGN(4) { *(.rodata*) } :text

    . = ALIGN(0x1000);
    .data   : ALIGN(4) { *(.data*) } :data
    .bss    : ALIGN(4) { *(COMMON) *(.bss*) } :data

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
        *(.eh_frame*)
    }
    
    kernel_end_symbol = .;
}
