[bits 32]
global idt_load
global isr_default_stub
global irq0_timer_stub
global irq1_keyboard_stub
global syscall_stub

extern hal_timer_irq_handler
extern keyboard_handler
extern syscall_dispatch

; Load IDT table into CPU IDTR
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Fallback handler for unhandled interrupts
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

; IRQ0 PIT timer interrupt handler stub
irq0_timer_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call hal_timer_irq_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

; IRQ1 Keyboard interrupt handler stub
irq1_keyboard_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call keyboard_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

syscall_stub:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov esi, esp
    push dword [esi + 20]
    push dword [esi + 24]
    push dword [esi + 16]
    push dword [esi + 28]
    call syscall_dispatch
    add esp, 16
    mov [esi + 28], eax
    popa
    iretd
    