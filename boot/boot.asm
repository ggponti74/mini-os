org 0x7c00
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    ; 1. Preserve BIOS boot drive passed in DL (0x00=Floppy, 0x80=HDD, 0xE0=CD-ROM)
    mov [BOOT_DRIVE], dl

    ; Debug marker 1: Real mode started
    mov al, '1'
    out 0xe9, al

    call load_kernel

    ; Debug marker 3: Kernel loaded, transitioning to 32-bit mode
    mov al, '3'
    out 0xe9, al

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp dword CODE_SEG:init_pm

load_kernel:
    ; Debug marker 2: Entered load_kernel
    mov al, '2'
    out 0xe9, al

    ; Try LBA Extended Read (AH = 0x42)
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .fallback_chs
    cmp bx, 0xaa55
    jne .fallback_chs

    ; Execute LBA Read using DAP
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    jnc .read_success

.fallback_chs:
    ; Reset Drive
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; Read 64 sectors in 16-sector track-safe chunks
    mov bx, KERNEL_OFFSET
    mov cl, 2                   ; Start at Sector 2
    mov ch, 0                   ; Cylinder 0
    mov dh, 0                   ; Head 0
    mov di, 4                   ; 4 chunks * 16 sectors = 64 sectors

.read_loop:
    mov ah, 0x02
    mov al, 16                  ; 16 sectors at a time (prevents track overrun)
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .disk_error              ; <--- Prints 'E' if this fails!

    add bx, 512 * 16
    inc dh                      ; Next head
    dec di
    jnz .read_loop

.read_success:
    ret

.disk_error:
    ; Print red 'E' to top-left of VGA screen
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f

    ; Output 'E' to debug log
    mov al, 'E'
    out 0xe9, al

.halt_loop:
    hlt
    jmp .halt_loop

; ------------------------------------------------------------------------------
; LBA Disk Address Packet (DAP)
; ------------------------------------------------------------------------------
align 4
dap:
    db 0x10                     ; Packet size (16 bytes)
    db 0x00                     ; Reserved
    dw 64                       ; Sectors to read (32 KB)
    dw KERNEL_OFFSET            ; Offset (0x7E00)
    dw 0x0000                   ; Segment (0x0000)
    dd 1                        ; Starting LBA Sector (Sector 1)
    dd 0                        ; Upper 32-bits

; ------------------------------------------------------------------------------
; Global Descriptor Table & 32-bit Init
; ------------------------------------------------------------------------------
align 4
gdt_start:
    dd 0x0, 0x0
    ; Code Segment (0x08)
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
    ; Data Segment (0x10)
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_start + 8
DATA_SEG equ gdt_end - 8
BOOT_DRIVE: db 0

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

    ; Debug marker 4: Entering kernel
    mov al, '4'
    out 0xe9, al

    jmp KERNEL_OFFSET

times 510-($-$$) db 0
dw 0xaa55
