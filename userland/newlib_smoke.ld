OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x00400000;
    .text : {
        KEEP(*(.text.start))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : {
        *(.data*)
        *(.got*)
        *(.init_array*)
        *(.fini_array*)
    }
    __app_file_end = .;
    __bss_start = .;
    .bss (NOLOAD) : {
        *(COMMON)
        *(.bss*)
        *(.sbss*)
    }
    __bss_end = .;
    . = ALIGN(16);
    _end = .;
    end = .;
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}

ASSERT(__app_file_end <= 0x00440000, "Newlib executable exceeds initrd limit")
ASSERT(_end <= 0x00500000, "Newlib app exceeds reserved load memory")
