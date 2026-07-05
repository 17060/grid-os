global gridloop_bin
global gridloop_bin_end

section .rodata
align 4
gridloop_bin:
incbin "build/gridloop.bin"
gridloop_bin_end:
