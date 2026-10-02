ENTRY(_start)

SECTIONS
{
    . = 0x7E00;

    .text : {
        *(.entry)         /* Kernel entry assembly stub MUST be first */
        *(.text*)
        *(.rodata*)       /* Read-only data (string literals) mapped here */
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}