.section .text.start,"ax"
.global _start
.type _start, @function

_start:
    pushal
    mov 36(%esp), %esi
    mov 40(%esp), %edx
    cld
    mov $__bss_start, %edi
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    mov %esp, %ebp
    and $-16, %esp
    sub $8, %esp
    push %edx
    push %esi
    call main
    mov %ebp, %esp
    mov %eax, 28(%esp)
    popal
    ret

.size _start, .-_start
.section .note.GNU-stack,"",@progbits
