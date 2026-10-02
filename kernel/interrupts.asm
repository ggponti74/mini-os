[bits 32]

; Handle leading underscore symbol naming differences (Windows vs Linux)
%ifidn __OUTPUT_FORMAT__, win32
    %define IDT_LOAD _idt_load
    %define ISR_DEFAULT_STUB _isr_default_stub
    %define IRQ1_KEYBOARD_STUB _irq1_keyboard_stub
    %define KEYBOARD_HANDLER _keyboard_handler
%else
    %define IDT_LOAD idt_load
    %define ISR_DEFAULT_STUB isr_default_stub
    %define IRQ1_KEYBOARD_STUB irq1_keyboard_stub
    %define KEYBOARD_HANDLER keyboard_handler
%endif

global IDT_LOAD
global ISR_DEFAULT_STUB
global IRQ1_KEYBOARD_STUB

extern KEYBOARD_HANDLER

; Load IDT Pointer passed from C
IDT_LOAD:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Default fallback ISR handler
isr_default_stub:
    pusha
    
    ; If you had E9 debug prints in here, this is where '>' / '<' came from!
    mov al, '>'
    out 0xe9, al
    mov al, '<'
    out 0xe9, al

    popa
    iretd    ; Must be iretd for 32-bit protected mode!

; IRQ1 Keyboard Interrupt Assembly Stub
IRQ1_KEYBOARD_STUB:
    pusha               ; Save registers
    call KEYBOARD_HANDLER
    popa                ; Restore registers
    iret                ; Return from interrupt

; Non-executable stack section declaration for Linux ELF targets
%ifidn __OUTPUT_FORMAT__, elf32
section .note.GNU-stack noexec alloc progbits
%endif