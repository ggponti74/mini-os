[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x7e00

start:
    mov [BOOT_DRIVE], dl

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    call load_kernel

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_pm

load_kernel:
    mov bx, KERNEL_OFFSET
    mov cx, 0x0002
    mov dh, 0
    mov di, 64

.read_loop:
    push cx
    push dx
    push di

    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13

    mov ax, 0x0201
    mov dl, [BOOT_DRIVE]
    int 0x13
    jnc .success

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

.disk_error:
    mov ax, 0xb800
    mov es, ax
    mov byte [es:0], 'E'
    mov byte [es:1], 0x4f
.halt:
    hlt
    jmp .halt

align 4
gdt_start:
    dq 0

gdt_code:
    dw 0xffff
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

gdt_data:
    dw 0xffff
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BOOT_DRIVE: db 0

[bits 32]
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp
    jmp KERNEL_OFFSET

times 510-($-$$) db 0
dw 0xaa55
