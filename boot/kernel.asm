[bits 32]
[global _stage2_entry]
[extern _kernel_main]

_stage2_entry:
    call _kernel_main
    hlt