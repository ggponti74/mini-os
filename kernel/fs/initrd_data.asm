[bits 32]

[global _initrd_start]
[global _initrd_end]

section .rodata

_initrd_start:
    incbin "initrd.tar"    ; Or a simple raw payload / text file
_initrd_end: