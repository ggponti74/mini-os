[bits 32]
global stage2_entry
extern kernel_main

section .boot_entry

stage2_entry:
    ; 1. Reload segment registers with 32-bit Data Selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 2. Initialize Stack Pointer (0x90000)
    mov ebp, 0x90000
    mov esp, ebp

    ; 3. Call C Kernel Entry
    call kernel_main

.halt:
    hlt
    jmp .halt
    