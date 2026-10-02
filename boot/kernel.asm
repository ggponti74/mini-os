[bits 32]
[global stage2_entry]
[extern kernel_main]

stage2_entry:
    call kernel_main
.halt:
    hlt
    jmp .halt