ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    note PT_NOTE FLAGS(4);
}

SECTIONS
{
    . = 1M;

    .text : {
        KEEP(*(.multiboot_header))
        *(.text .text.*)
    } :text

    .note.Xen : {
        KEEP(*(.note.Xen))
    } :note

    .rodata : ALIGN(4K) {
        *(.rodata .rodata.*)
    } :text

    .data : ALIGN(4K) {
        *(.data .data.*)
    } :text

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss .bss.*)
    } :text
}
