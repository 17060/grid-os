global gridprog_bin
global gridprog_bin_end

section .rodata
align 4
gridprog_bin:
incbin "build/gridprog.bin"
gridprog_bin_end:
