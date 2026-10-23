.section .data

#if defined(_WIN32) || defined(__WIN32__)
.global _initrd_start
.global _initrd_end

_initrd_start:
#else
.global initrd_start
.global initrd_end

initrd_start:
#endif
    .incbin "initrd.tar"

#if defined(_WIN32) || defined(__WIN32__)
_initrd_end:
#else
initrd_end:
#endif