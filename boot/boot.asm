org 0x7c00
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    cli                         ; 1. Disable interrupts immediately
    cld                         ; Clear direction flag

    ; 2. Initialize segment registers FIRST before accessing memory
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti                         ; Re-enable interrupts

    ; 3. Store the boot drive passed in DL by BIOS (0x80 for hard disk)
    mov [BOOT_DRIVE], dl

    ; 4. Read kernel sectors into RAM at 0x7E00
    call load_kernel

    ; 5. Switch to 32-bit Protected Mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 6. CRITICAL: Explicit 32-bit far jump from 16-bit real mode
    jmp dword CODE_SEG:init_pm

load_kernel:
    ; 1. Reset disk drive controller
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; 2. Read sectors into memory at 0x7E00
    mov ah, 0x02
    mov al, 64                  ; <-- BUMP FROM 15 TO 64 SECTORS (32 KB)
    mov ch, 0                   ; Cylinder 0
    mov dh, 0                   ; Head 0
    mov cl, 2                   ; Start at Sector 2 (Sector 1 is bootloader)
    mov dl, [BOOT_DRIVE]        ; Boot drive passed by BIOS
    mov bx, KERNEL_OFFSET       ; Destination 0x7E00
    int 0x13
    jnc .read_success           ; Jump if successful

    ; Fallback retry forcing Hard Drive 0x80
    mov ah, 0x02
    mov al, 64                  ; <-- BUMP RETRY COUNT TO 64 SECTORS
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov dl, 0x80
    mov bx, KERNEL_OFFSET
    int 0x13
    jc disk_error

.read_success:
    ret

disk_error:
    ; Output "ERR\n" to serial COM1 for terminal debugging
    mov dx, 0x3f8
    mov al, 'E'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 0x0a
    out dx, al
.halt_loop:
    hlt
    jmp .halt_loop

; --- Global Descriptor Table (GDT) ---
gdt_start:
    dq 0x0                      ; Null descriptor

gdt_code:
    dw 0xffff                   ; Limit 0-15
    dw 0x0                      ; Base 0-15
    db 0x0                      ; Base 16-23
    db 10011010b                ; Access byte (Code, Executable, Readable)
    db 11001111b                ; Granularity (4KB blocks, 32-bit PM)
    db 0x0                      ; Base 24-31

gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b                ; Access byte (Data, Read/Write)
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

    jmp KERNEL_OFFSET           ; Transfer control to kernel_entry (0x7E00)

BOOT_DRIVE: db 0

times 510-($-$$) db 0
dw 0xaa55