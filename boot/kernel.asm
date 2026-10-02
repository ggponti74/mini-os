[bits 32]
[extern kernel_main]

global _start
_start:
    ; Load Protected Mode Data Selectors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Stack grows down from 0x90000
    mov ebp, 0x90000
    mov esp, ebp

    call kernel_main

.hang:
    hlt
    jmp .hang
    