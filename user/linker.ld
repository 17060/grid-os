ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);   /* PF_R | PF_X */
    rodata PT_LOAD FLAGS(4); /* PF_R */
    data PT_LOAD FLAGS(6);   /* PF_R | PF_W */
}

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text .text.*)
    } :text

    /* Page-align each loadable segment so no two PT_LOADs share a 4 KiB page.
     * The kernel ELF loader maps one page per segment with that segment's
     * permissions; when .text (R+X) and .rodata (R, NX) shared a page, the
     * rodata mapping clobbered the text page's execute bit and the entry-point
     * fetch faulted (W^X). Separate pages keep each segment's flags intact. */
    . = ALIGN(0x1000);
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(0x1000);
    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
    } :data
}
