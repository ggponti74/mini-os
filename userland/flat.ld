ENTRY(_start)

SECTIONS
{
    . = 0;

    .text : {
        KEEP(*(.text.start))
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .got : {
        *(.got*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }

    __end = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}
