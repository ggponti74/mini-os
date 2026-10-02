[bits 16]
[org 0x7C00]

KERNEL_OFFSET equ 0x7E00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Read sectors off floppy to 0x7E00
    mov ah, 0x02
    mov al, 15
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov dl, 0
    mov bx, KERNEL_OFFSET
    int 0x13

    ; Protected Mode Switch
    cli
    in al, 0x92
    or al, 0x02
    out 0x92, al            ; Enable A20 Line

    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax            ; Set PE bit

    jmp 0x08:init_pm        ; Far jump to 32-bit code

; GDT Setup
gdt_start:
    dd 0x00000000, 0x00000000               ; Null descriptor
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF       ; Code segment (0x08)
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF       ; Data segment (0x10)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[bits 32]
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp KERNEL_OFFSET       ; Jump to Stage 2 wrapper at 0x7E00

times 510 - ($ - $$) db 0
dw 0xAA55