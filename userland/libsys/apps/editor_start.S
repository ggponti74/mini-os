.section .text.start,"ax"
.global _start
#ifdef __ELF__
.type _start, @function
#endif
_start:
    jmp editor_entry
