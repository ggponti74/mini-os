[bits 32]

; Handle leading underscore difference between Windows (win32) and Linux (elf32)
%ifidn __OUTPUT_FORMAT__, win32
    %define IDT_LOAD _idt_load
    %define ISR_DEFAULT_STUB _isr_default_stub
%else
    %define IDT_LOAD idt_load
    %define ISR_DEFAULT_STUB isr_default_stub
%endif

global IDT_LOAD
global ISR_DEFAULT_STUB

IDT_LOAD:
    mov eax, [esp + 4]  ; Pointer to idt_ptr passed from C
    lidt [eax]          ; Load Interrupt Descriptor Table
    ret

ISR_DEFAULT_STUB:
    pusha               ; Save CPU registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    
    ; Optional interrupt handling logic can go here
    
    popa                ; Restore CPU registers
    iret                ; Return from interrupt