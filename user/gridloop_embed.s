global gridloop_bin
global gridloop_bin_end

section .note.GNU-stack noalloc noexec nowrite progbits

section .rodata
align 4
gridloop_bin:
incbin "build/gridloop.bin"
gridloop_bin_end:
