global escape_bin
global escape_bin_end

section .note.GNU-stack noalloc noexec nowrite progbits

section .rodata
align 4
escape_bin:
incbin "build/escape.bin"
escape_bin_end:
