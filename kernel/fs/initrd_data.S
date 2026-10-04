.section .rodata
.global initrd_start
.global initrd_end
.global _initrd_start
.global _initrd_end

initrd_start:
_initrd_start:
    .incbin "initrd.tar"
initrd_end:
_initrd_end: