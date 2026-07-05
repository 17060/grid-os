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

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
    } :data
}
