[bits 16]
[org 0x7c00]

start:
    mov [BOOT_DRIVE], dl    ; Save boot drive provided by BIOS (0x00 or 0x80)

load_kernel:
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13                ; Reset disk controller

    mov ah, 0x02
    mov al, 32              ; Read 32 sectors (~16KB)
    mov ch, 0
    mov dh, 0
    mov cl, 2               ; Start at Sector 2 (right after boot sector)
    mov dl, [BOOT_DRIVE]
    mov bx, 0x7E00          ; Load destination address
    int 0x13
    jnc .success

    ; Fallback: retry with explicit hard disk drive 0x80
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov dl, 0x80
    mov bx, 0x7E00
    int 0x13

.success:
    ; Proceed to Protected Mode switch...

BOOT_DRIVE: db 0
