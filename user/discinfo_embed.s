global discinfo_bin
global discinfo_bin_end

section .rodata
align 4
discinfo_bin:
incbin "build/discinfo.bin"
discinfo_bin_end:
