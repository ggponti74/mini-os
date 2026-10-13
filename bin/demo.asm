[bits 32]
global _start

_start:
    ; [esp + 4] points to the first argument (e.g., args pointer)
    mov eax, [esp + 4] 

    mov eax, 0      ; Set return code to 0 (success)
    ret