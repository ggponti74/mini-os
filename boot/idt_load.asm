[bits 32]
global idt_load

idt_load:
    mov eax, [esp + 4]    ; C argument: address of struct idt_ptr
    lidt [eax]            ; Load the 6-byte {limit, base} descriptor
    ret
