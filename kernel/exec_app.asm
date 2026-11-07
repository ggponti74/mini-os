[bits 32]
global exec_app_call
global exec_app_exit

; int exec_app_call(int (*entry)(int, char **), int argc, char **argv)
; Save the kernel caller's callee-saved registers and stack so SYS_EXIT can
; abandon the application stack and return directly to exec_flat_binary().
exec_app_call:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov [app_exit_stack], esp

    mov eax, [ebp + 8]
    mov edx, [ebp + 12]
    mov ecx, [ebp + 16]

    ; Match the i386 ABI's 16-byte stack alignment at the application call.
    and esp, 0xfffffff0
    sub esp, 8
    push ecx
    push edx
    call eax

    mov dword [app_exit_stack], 0
    lea esp, [ebp - 12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; int exec_app_exit(int status)
; With no running application, return the status normally. Otherwise restore
; exec_app_call's saved stack and return from that function with the exit code.
exec_app_exit:
    mov eax, [esp + 4]
    mov edx, [app_exit_stack]
    test edx, edx
    jz .return_status

    mov dword [app_exit_stack], 0
    mov esp, edx
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

.return_status:
    ret

section .bss
align 4
app_exit_stack: resd 1

section .note.GNU-stack noalloc noexec nowrite progbits
