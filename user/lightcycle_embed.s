global lightcycle_bin
global lightcycle_bin_end

section .rodata
align 4
lightcycle_bin:
incbin "build/lightcycle.bin"
lightcycle_bin_end:
