[bits 32]

%ifidn __OUTPUT_FORMAT__, win32
    %define KERNEL_MAIN _kernel_main
    %define STAGE2_ENTRY _stage2_entry
%else
    %define KERNEL_MAIN kernel_main
    %define STAGE2_ENTRY stage2_entry
%endif

[global STAGE2_ENTRY]
[extern KERNEL_MAIN]

STAGE2_ENTRY:
    call KERNEL_MAIN
    hlt