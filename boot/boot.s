%define MULTIBOOT2_MAGIC 0xE85250D6
%define MULTIBOOT2_ARCH 0
%define XEN_ELFNOTE_PVH_ENTRY 18

section .note.Xen
align 4
    dd 4
    dd 8
    dd XEN_ELFNOTE_PVH_ENTRY
    db "Xen", 0
    align 4
    dq _start

section .multiboot_header
align 8
multiboot_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd multiboot_header_end - multiboot_header
    dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + (multiboot_header_end - multiboot_header))
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .bss
align 16
global stack_top
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
_start:
    mov rsp, stack_top
    xor ebp, ebp
    extern kernel_main
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
