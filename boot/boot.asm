[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [BOOT_DRIVE], dl    ; Save boot drive index passed by BIOS

    call load_kernel_lba

    ; --- Protected Mode Transition ---
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load GDT structure

    mov eax, cr0
    or eax, 0x1             ; Set Bit 0 in CR0 (Enable Protected Mode)
    mov cr0, eax

    ; Far jump using Code Segment selector (0x08) to flush 16-bit pipeline
    jmp CODE_SEG:init_pm

; ------------------------------------------------------------------------------
; LBA / CHS Disk Loader Routine
; ------------------------------------------------------------------------------
load_kernel_lba:
    ; Check for BIOS LBA Extensions (AH = 0x41)
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .fallback_chs

    ; LBA Extended Read (AH = 0x42) using DAP
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    jnc .read_success

.fallback_chs:
    ; CHS Read Fallback
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    mov ah, 0x02
    mov al, 64              ; Read 64 sectors
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov dl, [BOOT_DRIVE]
    mov bx, KERNEL_OFFSET
    int 0x13
    jc .disk_error

.read_success:
    ret

.disk_error:
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f
    hlt

; ------------------------------------------------------------------------------
; 32-bit Protected Mode Initialization
; ------------------------------------------------------------------------------
[bits 32]
init_pm:
    mov ax, DATA_SEG        ; Point all data segments to Data Descriptor (0x10)
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; Set stack location safely above kernel
    mov esp, ebp

    jmp KERNEL_OFFSET       ; Jump directly into kernel_entry in kernel.asm

; ------------------------------------------------------------------------------
; Global Descriptor Table (GDT) & DAP Data
; ------------------------------------------------------------------------------
align 4
dap:
    db 0x10                 ; Packet size (16 bytes)
    db 0x00                 ; Reserved
    dw 64                   ; Number of sectors to read
    dw KERNEL_OFFSET        ; Destination offset (0x7E00)
    dw 0x0000               ; Destination segment (0x0000)
    dd 1                    ; Starting LBA (Sector 1)
    dd 0                    ; Upper 32-bits of LBA

gdt_start:
    ; Null Descriptor (Mandatory 8 null bytes)
    dd 0x0
    dd 0x0

    ; Code Segment Descriptor (Base: 0x0, Limit: 4GB)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

    ; Data Segment Descriptor (Base: 0x0, Limit: 4GB)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; GDT Size - 1
    dd gdt_start                ; GDT Offset Address

CODE_SEG equ gdt_start + 8      ; Code Segment Selector (0x08)
DATA_SEG equ gdt_end - 8        ; Data Segment Selector (0x10)

BOOT_DRIVE: db 0

times 510-($-$$) db 0
dw 0xaa55
