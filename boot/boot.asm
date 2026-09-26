[BITS 16]
[ORG 0x7C00]        ; BIOS loads MBRs into physical address 0x7C00

start:
    ; Set up segment registers safely
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00  ; Stack grows downward from load address

    ; Print 'H' via BIOS teletype function
    mov ah, 0x0E    ; INT 10h AH=0Eh: Teletype Output
    mov al, 'H'     ; Character to print
    int 0x10

hang:
    cli             ; Disable hardware interrupts
    hlt             ; Halt CPU until next interrupt
    jmp hang        ; Loop if an NMI wakes the CPU

; Pad remaining bytes up to 510 with zeros
times 510 - ($ - $$) db 0

; Boot Signature required by BIOS (Magic bytes)
dw 0xAA55