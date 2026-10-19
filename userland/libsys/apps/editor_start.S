.section .text.start,"ax"
.global _start
.type _start, @function
_start:
    jmp editor_entry
