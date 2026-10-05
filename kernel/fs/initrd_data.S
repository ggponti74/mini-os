.section .data
.global initrd_start
.global initrd_end

initrd_start:
    .incbin "initrd.tar"
initrd_end: