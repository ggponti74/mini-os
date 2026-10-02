[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    ; 1. Save boot drive passed by BIOS in DL
    mov [BOOT_DRIVE], dl

    ; 2. Reset segment registers and stack pointer
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; 3. Load kernel sectors into memory
    call load_kernel

    ; 4. Prepare transition to 32-bit Protected Mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 5. Explicit 16-bit far jump to clear pipeline into 32-bit segment
    jmp 0x08:init_pm

; ------------------------------------------------------------------------------
; Sector Loader Routine (Track-Safe CHS Read)
; ------------------------------------------------------------------------------
load_kernel:
    mov bx, KERNEL_OFFSET
    mov cx, 0x0002          ; Sector 2, Cylinder 0
    mov dh, 0               ; Head 0
    mov di, 64              ; Read 64 sectors (32 KB)

.read_loop:
    push cx
    push dx
    push di

    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13                ; Reset drive controller

    mov ax, 0x0201          ; Read 1 sector
    mov dl, [BOOT_DRIVE]
    int 0x13
    jnc .success

    ; Retry using hard disk drive index 0x80 if floppy 0x00 fails
    mov ax, 0x0201
    mov dl, 0x80
    int 0x13
    jc .disk_error

.success:
    add bx, 512
    pop di
    pop dx
    pop cx

    inc cl
    cmp cl, 19
    jne .next

    mov cl, 1
    inc dh
    cmp dh, 2
    jne .next

    mov dh, 0
    inc ch

.next:
    dec di
    jnz .read_loop
    ret

; ------------------------------------------------------------------------------
; Disk Error Handler (Displays Red 'E' on VGA Screen if INT 13h Fails)
; ------------------------------------------------------------------------------
.disk_error:
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f
.halt:
    hlt
    jmp .halt

; ------------------------------------------------------------------------------
; Global Descriptor Table (GDT)
; ------------------------------------------------------------------------------
align 4
gdt_start:
    dd 0x0, 0x0             ; Null Descriptor

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

BOOT_DRIVE: db 0

; ------------------------------------------------------------------------------
; 32-bit Protected Mode Initialization
; ------------------------------------------------------------------------------
[bits 32]
init_pm:
    mov ax, 0x10            ; Data segment selector (gdt_data offset)
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    jmp KERNEL_OFFSET       ; Jump directly into stage2_entry

times 510-($-$$) db 0
dw 0xaa55