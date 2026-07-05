global gridsh_bin
global gridsh_bin_end

section .note.GNU-stack noalloc noexec nowrite progbits

section .rodata
align 4
gridsh_bin:
incbin "build/gridsh.bin"
gridsh_bin_end:
