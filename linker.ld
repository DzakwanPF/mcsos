OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

PHDRS
{
    requests PT_LOAD FLAGS(5);
    text     PT_LOAD FLAGS(5);
    rodata   PT_LOAD FLAGS(4);
    data     PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0xffffffff80000000;
    __kernel_start = .;

    .limine_requests_start : ALIGN(8)
    {
        KEEP(*(.limine_requests_start))
    } :requests

    .limine_requests : ALIGN(8)
    {
        KEEP(*(.limine_requests))
    } :requests

    .limine_requests_end : ALIGN(8)
    {
        KEEP(*(.limine_requests_end))
    } :requests

    .text : ALIGN(4096)
    {
        *(.text .text.*)
    } :text

    .rodata : ALIGN(4096)
    {
        *(.rodata .rodata.*)
    } :rodata

    .data : ALIGN(4096)
    {
        *(.data .data.*)
    } :data

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    __kernel_end = .;
}
