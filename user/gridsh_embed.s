global gridsh_bin
global gridsh_bin_end

section .rodata
align 4
gridsh_bin:
incbin "build/gridsh.bin"
gridsh_bin_end:
