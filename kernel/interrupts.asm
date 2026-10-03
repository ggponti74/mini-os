[bits 32]
global idt_load
global isr_default_stub
global irq1_keyboard_stub    ; Export symbol for C linker

extern keyboard_handler

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Default handler for unmapped interrupts
isr_default_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

; Dedicated handler for IRQ1 (Keyboard)
irq1_keyboard_stub:
    pusha           ; Save general-purpose registers
    push ds         ; Save segment registers
    push es
    push fs
    push gs

    mov ax, 0x10    ; Load Kernel Data Segment (0x10)
    mov ds, ax
    mov es, ax

    call keyboard_handler ; Call C handler in keyboard.c

    pop gs          ; Restore segment registers
    pop fs
    pop es
    pop ds
    popa            ; Restore general-purpose registers
    iret            ; Return from interrupt