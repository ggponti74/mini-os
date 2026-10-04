org 0x7c00
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    xor ax, ax                  ; Set segment registers to 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl        ; Save boot drive number passed by BIOS

    ; Enable A20 Line via System Control Port A (0x92)
    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; Load kernel sectors from disk
    call load_kernel

    ; Switch to 32-bit Protected Mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:init_pm

load_kernel:
    ; Reset disk controller
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; Read 128 sectors (64 KB) to 0x0000:0x7E00 to fit initrd archive
    mov ah, 0x02
    mov al, 128                 ; Number of sectors
    mov ch, 0                   ; Cylinder 0
    mov dh, 0                   ; Head 0
    mov cl, 2                   ; Sector 2 (immediately after boot sector)
    mov dl, [BOOT_DRIVE]
    mov bx, KERNEL_OFFSET
    int 0x13
    jc disk_error
    ret

disk_error:
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f
    hlt

; --- Global Descriptor Table (GDT) ---
align 4
gdt_start:
    ; Null Descriptor
    dd 0x0
    dd 0x0

gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[bits 32]
init_pm:
    mov ax, DATA_SEG            ; Point all data segment registers to 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000            ; Set stack safely above 0x7E00
    mov esp, ebp

    jmp KERNEL_OFFSET           ; Jump to kernel entry point in kernel.asm

BOOT_DRIVE: db 0

times 510-($-$$) db 0
dw 0xaa55