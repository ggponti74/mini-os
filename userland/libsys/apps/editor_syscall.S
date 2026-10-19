.text
.global editor_syscall
.type editor_syscall, @function
editor_syscall:
    push %ebx
    mov 8(%esp), %eax
    mov 12(%esp), %ebx
    mov 16(%esp), %ecx
    mov 20(%esp), %edx
    int $0x80
    pop %ebx
    ret
