[bits 32]
[extern kernel_main]

global _start
_start:
    ; Set segment registers to 32-bit data segment selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack pointer pointing to a safe physical address
    mov esp, 0x90000

    ; Call C entry point
    call kernel_main

    ; Infinite halt if kernel returns
.halt:
    cli
    hlt
    jmp .halt
    