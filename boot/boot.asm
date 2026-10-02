[bits 16]
[org 0x7C00]

KERNEL_OFFSET equ 0x7E00    ; Kernel load target address in memory

start:
    ; 1. Clear interrupt flag during critical register setup
    cli

    ; 2. Normalize segment registers (0x0000) and stack pointer
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows downward from bootloader origin

    sti                     ; Re-enable interrupts for BIOS disk reads

    ; 3. Output boot sequence debug marker '1' to port 0xE9
    mov al, '1'
    out 0xE9, al

    ; 4. Load kernel sectors from floppy disk
    call load_kernel

    ; 5. Output boot sequence debug marker '2' to port 0xE9
    mov al, '2'
    out 0xE9, al

    ; 6. Disable interrupts before entering Protected Mode
    cli

    ; 7. Enable A20 Line via System Control Port A
    in al, 0x92
    or al, 0x02
    out 0xE9, al            ; Safety fallback or debug write
    out 0x92, al            ; Bit 1 enables physical address line 20

    ; 8. Load Global Descriptor Table (GDT)
    lgdt [gdt_descriptor]

    ; 9. Set Protection Enable (PE) bit in Control Register CR0
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    ; 10. Far jump to clear the instruction prefetch queue and set CS selector (0x08)
    jmp 0x08:init_pm

; ------------------------------------------------------------------------------
; DISK READ ROUTINE
; ------------------------------------------------------------------------------
load_kernel:
    ; Reset disk system (Drive 0x00 = Floppy Drive A:)
    mov ah, 0x00
    mov dl, 0x00
    int 0x13
    jc load_kernel          ; Retry on error

    ; Read Kernel Sectors starting at Sector 2 (CHS: C=0, H=0, S=2)
    mov ah, 0x02            ; BIOS Read Sectors function
    mov al, 15              ; Number of sectors to read (adjust as kernel grows)
    mov ch, 0               ; Cylinder 0
    mov dh, 0               ; Head 0
    mov cl, 2               ; Sector 2 (Sector 1 is boot.asm)
    mov dl, 0               ; Drive 0 (Floppy A:)
    mov bx, KERNEL_OFFSET   ; ES:BX buffer pointer (0x0000:0x7E00)
    int 0x13
    jc load_kernel          ; Retry if read failed

    ret

; ------------------------------------------------------------------------------
; GLOBAL DESCRIPTOR TABLE (GDT)
; ------------------------------------------------------------------------------
gdt_start:

gdt_null:                   ; Mandatory null descriptor (8 bytes)
    dd 0x00000000
    dd 0x00000000

gdt_code:                   ; Kernel Code Segment: Base 0x0, Limit 4GB, Ring 0
    dw 0xFFFF               ; Limit (bits 0-15)
    dw 0x0000               ; Base (bits 0-15)
    db 0x00                 ; Base (bits 16-23)
    db 10011010b            ; Access Byte: Present, Ring 0, Code, Executable, Readable
    db 11001111b            ; Flags (Granularity 4KB, 32-bit) + Limit (bits 16-19)
    db 0x00                 ; Base (bits 24-31)

gdt_data:                   ; Kernel Data Segment: Base 0x0, Limit 4GB, Ring 0
    dw 0xFFFF               ; Limit (bits 0-15)
    dw 0x0000               ; Base (bits 0-15)
    db 0x00                 ; Base (bits 16-23)
    db 10010010b            ; Access Byte: Present, Ring 0, Data, Writable
    db 11001111b            ; Flags (Granularity 4KB, 32-bit) + Limit (bits 16-19)
    db 0x00                 ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; GDT Size (Limit)
    dd gdt_start                 ; GDT Address

; ------------------------------------------------------------------------------
; 32-BIT PROTECTED MODE ENTRY POINT
; ------------------------------------------------------------------------------
[bits 32]
init_pm:
    ; Output debug marker '3' to port 0xE9
    mov al, '3'
    out 0xE9, al

    ; Point 32-bit segment registers to Data Segment Selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Initialize 32-bit stack pointer safely above loaded kernel
    mov esp, 0x90000

    ; Output debug marker '4' to port 0xE9
    mov al, '4'
    out 0xE9, al

    ; Jump directly to kernel entry point
    jmp KERNEL_OFFSET

; ------------------------------------------------------------------------------
; BOOT SECTOR PADDING & SIGNATURE
; ------------------------------------------------------------------------------
times 510 - ($ - $$) db 0   ; Pad up to byte 510 with zeros
dw 0xAA55                   ; Boot sector magic signature (2 bytes)
