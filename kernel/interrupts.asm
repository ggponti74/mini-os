[bits 32]

global idt_load
global isr_default_stub
global irq1_keyboard_stub

extern keyboard_handler

; Load IDT Pointer passed from C
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Default fallback ISR handler
isr_default_stub:
    pusha
    popa
    iret

; IRQ1 Keyboard Interrupt Stub
irq1_keyboard_stub:
    pusha               ; Save registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    call keyboard_handler
    popa                ; Restore registers
    iret                ; Return from interrupt