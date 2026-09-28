org 0x7c00
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    ; Set up 16-bit segment registers and stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl

    ; 1. LOAD KERNEL FROM DISK IN REAL MODE FIRST
    call load_kernel

    ; 2. SWITCH TO PROTECTED MODE
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Far jump to flush pipeline and load CS with 0x08 (CODE_SEG)
    jmp CODE_SEG:init_pm

load_kernel:
    ; Reset disk drive
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; Read 15 sectors into 0x0000:0x7E00
    mov ah, 0x02
    mov al, 15          ; Sector count
    mov ch, 0           ; Cylinder 0
    mov dh, 0           ; Head 0
    mov cl, 2           ; Sector 2 (Sector 1 is boot.bin)
    mov dl, [BOOT_DRIVE]
    mov bx, KERNEL_OFFSET
    int 0x13
    jc disk_error
    ret

disk_error:
    ; Print 'E' at top left if disk read failed
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f
    hlt

; --- GDT Setup ---
gdt_start:
    dq 0x0

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
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    ; Jump directly to kernel entry point at 0x7E00
    jmp KERNEL_OFFSET

BOOT_DRIVE: db 0

times 510-($-$$) db 0
dw 0xaa55
