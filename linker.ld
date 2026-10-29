ENTRY(kmain)

PHDRS
{
    text PT_LOAD FILEHDR PHDRS FLAGS(0x5);
    rodata PT_LOAD FLAGS(0x4);
    data PT_LOAD FLAGS(0x6);
}

SECTIONS
{
    /* Limine requires fixed-address ELF load segments in the x86-64 higher half. */
    . = 0xffffffff80000000 + SIZEOF_HEADERS;

    .text : ALIGN(4K)
    {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
        *(.text*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : ALIGN(4K)
    {
        *(.data*)
    } :data

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss*)
    } :data

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note*)
    }
}