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
align 4096
boot_pml4:
    resb 4096
boot_pdpt:
    resb 4096
boot_pd:
    resb 4096
align 16
stack_bottom:
    resb 16384
global stack_top
stack_top:

section .rodata
align 16
boot_gdt:
    dq 0
    dq 0x00AF9A000000FFFF          ; 0x08: 64-bit ring-0 code
    dq 0x00CF92000000FFFF          ; 0x10: ring-0 data
boot_gdt_end:
boot_gdt_ptr:
    dw boot_gdt_end - boot_gdt - 1
    dd boot_gdt                    ; 32-bit base — GDT lives below 4 GB

section .text
global _start

; QEMU PVH (and multiboot) hand control to _start in 32-bit protected mode
; with paging disabled. Build identity-mapped page tables for the first 1 GB
; (2 MB pages), enable long mode, then jump into the 64-bit kernel.
bits 32
_start:
    cli
    cld

    ; zero the three boot page tables
    mov edi, boot_pml4
    xor eax, eax
    mov ecx, (4096 * 3) / 4
    rep stosd

    ; pml4[0] -> pdpt, pdpt[0] -> pd (present | writable)
    mov eax, boot_pdpt
    or eax, 0x3
    mov [boot_pml4], eax
    mov eax, boot_pd
    or eax, 0x3
    mov [boot_pdpt], eax

    ; pd[0..511] = i * 2MB (present | writable | page-size)
    mov edi, boot_pd
    mov eax, 0x83
    mov ecx, 512
.fill_pd:
    mov [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .fill_pd

    ; CR4.PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, boot_pml4
    mov cr3, eax

    ; EFER.LME
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; CR0.PG | CR0.PE
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    lgdt [boot_gdt_ptr]
    jmp 0x08:long_mode_entry

bits 64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, stack_top
    xor ebp, ebp
    extern kernel_main
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
