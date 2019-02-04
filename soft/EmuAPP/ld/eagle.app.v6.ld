/* This linker script generated from xt-genldscripts.tpp for LSP . */
/* Linker Script for ld -N */
MEMORY
{
    dram_seg    : org = 0x3FFE8000, len = 0x4000
    dram_bk_seg : org = 0x3FFEC000, len = 0x14000
    iram_seg    : org = 0x40100000, len = 0x4000
    iram_bk_seg : org = 0x40104000, len = 0xC000
    irom_seg    : org = 0x40201010, len = 0xEFF0
}

PHDRS
{
    dram_phdr           PT_LOAD;
    dram_bss_phdr       PT_LOAD;
    dram_bk_phdr        PT_LOAD;
    iram_phdr           PT_LOAD;
    iram_bk_phdr        PT_LOAD;
    irom_phdr           PT_LOAD;
}


/*  Default entry point:  */
ENTRY(call_user_start)

PROVIDE(_memmap_vecbase_reset = 0x40000000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00000110;
_memmap_cacheattr_wt_base = 0x00000110;
_memmap_cacheattr_bp_base = 0x00000220;
_memmap_cacheattr_unused_mask = 0xFFFFF00F;
_memmap_cacheattr_wb_trapnull = 0x2222211F;
_memmap_cacheattr_wba_trapnull = 0x2222211F;
_memmap_cacheattr_wbna_trapnull = 0x2222211F;
_memmap_cacheattr_wt_trapnull = 0x2222211F;
_memmap_cacheattr_bp_trapnull = 0x2222222F;
_memmap_cacheattr_wb_strict = 0xFFFFF11F;
_memmap_cacheattr_wt_strict = 0xFFFFF11F;
_memmap_cacheattr_bp_strict = 0xFFFFF22F;
_memmap_cacheattr_wb_allvalid = 0x22222112;
_memmap_cacheattr_wt_allvalid = 0x22222112;
_memmap_cacheattr_bp_allvalid = 0x22222222;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{
    .data : ALIGN(4)
    {
        _data_start = ABSOLUTE(.);

        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.sdata2)
        *(.sdata2.*)
        *(.gnu.linkonce.s2.*)
        *(.jcr)

        _data_end = ABSOLUTE(.);

    } >dram_seg :dram_phdr

    .rodata : ALIGN(4)
    {
        _rodata_start = ABSOLUTE(.);

        *(.sdk.version)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        *(.rodata1)
        __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)
        *(.eh_frame)
        /*  C++ constructor and destructor tables, properly ordered:  */
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        /*  C++ exception handlers table:  */
        __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
        *(.xt_except_desc)
        *(.gnu.linkonce.h.*)
        __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
        *(.xt_except_desc_end)
        *(.dynamic)
        *(.gnu.version_d)

        _rodata_end = ABSOLUTE(.);

    } >dram_seg :dram_phdr

    .bss ALIGN(8) (NOLOAD) : ALIGN(4)
    {
        . = ALIGN (8);
        _bss_start = ABSOLUTE(.);
        *(.dynsbss)
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
        *(.sbss2)
        *(.sbss2.*)
        *(.gnu.linkonce.sb2.*)
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN (8);
        _bss_end = ABSOLUTE(.);
        _heap_start = ABSOLUTE(.);
        _stack_limit = ALIGN(0x8);

    } >dram_seg :dram_bss_phdr

    _stack_init = 0x3FFEC000;

    .dram_bk.bss (NOLOAD) : ALIGN(4)
    {
        *(.dram_bk.bss)

    } >dram_bk_seg :dram_bk_phdr

    .text : ALIGN(4)
    {
        _stext = .;
        _text_start = ABSOLUTE(.);
        *(.vecbase.text)
        . = ALIGN (16);
        *(.entry.text)
        *(.init.literal)
        *(.init)
        *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
        *(.iram.literal .iram.text)
        *(.fini.literal)
        *(.fini)
        *(.gnu.version)
        _text_end = ABSOLUTE(.);
        _etext = .;

    } >iram_seg :iram_phdr

    .lit4 : ALIGN(4)
    {
        _lit4_start = ABSOLUTE(.);
        *(*.lit4)
        *(.lit4.*)
        *(.gnu.linkonce.lit4.*)
        _lit4_end = ABSOLUTE(.);
    } >iram_seg :iram_phdr

/*------------------- Overlay 0 ----------------------------------------- */

    OVERLAY : NOCROSSREFS
    {
        .ovl0_ef.text {*(.ovl0_ef.literal .ovl0_ef.text .ovl0_ef.rodata)}
        .ovl0_u.text  {*(.ovl0_u.literal  .ovl0_u.text  .ovl0_u.rodata )}
        .ovl0_i.text  {*(.ovl0_i.literal  .ovl0_i.text  .ovl0_i.rodata )}

    } >iram_seg AT>irom_seg :irom_phdr

    .ovl0_end :
    {
        _ovl0_start = ADDR(.ovl0_ef.text);
        _ovl0_end   = ABSOLUTE(.);
    }

    __load_size_ovl0_eftext = __load_stop_ovl0_eftext - __load_start_ovl0_eftext;
    __load_size_ovl0_utext  = __load_stop_ovl0_utext  - __load_start_ovl0_utext;
    __load_size_ovl0_itext  = __load_stop_ovl0_itext  - __load_start_ovl0_itext;

/*------------------- Overlay 1 ----------------------------------------- */

    OVERLAY : NOCROSSREFS
    {
        .ovl1_eu.text {*(.ovl1_eu.literal .ovl1_eu.text .ovl1_eu.rodata)}
        .ovl1_f.text  {*(.ovl1_f.literal  .ovl1_f.text  .ovl1_f.rodata )}
        .ovl1_i.text  {*(.ovl1_i.literal  .ovl1_i.text  .ovl1_i.rodata )}

    } >iram_seg AT>irom_seg :irom_phdr


    .ovl1_end :
    {
        _ovl1_start = ADDR(.ovl1_eu.text);
        _ovl1_end   = ABSOLUTE(.);
    }

    __load_size_ovl1_eutext = __load_stop_ovl1_eutext - __load_start_ovl1_eutext;
    __load_size_ovl1_ftext  = __load_stop_ovl1_ftext  - __load_start_ovl1_ftext;
    __load_size_ovl1_itext  = __load_stop_ovl1_itext  - __load_start_ovl1_itext;

/*------------------- Overlay 2 ----------------------------------------- */

    OVERLAY : NOCROSSREFS
    {
        .ovl2_fu.text {*(.ovl2_fu.literal .ovl2_fu.text .ovl2_fu.rodata)}
        .ovl2_e.text  {*(.ovl2_e.literal  .ovl2_e.text  .ovl2_e.rodata )}
        .ovl2_i.text  {*(.ovl2_i.literal  .ovl2_i.text  .ovl2_i.rodata )}

    } >iram_seg AT>irom_seg :irom_phdr

    .ovl2_end :
    {
        _ovl2_start = ADDR(.ovl2_fu.text);
        _ovl2_end   = ABSOLUTE(.);
    }

    __load_size_ovl2_futext = __load_stop_ovl2_futext - __load_start_ovl2_futext;
    __load_size_ovl2_etext  = __load_stop_ovl2_etext  - __load_start_ovl2_etext;
    __load_size_ovl2_itext  = __load_stop_ovl2_itext  - __load_start_ovl2_itext;

/*------------------- Overlay 3 ----------------------------------------- */

    OVERLAY : NOCROSSREFS
    {
        .ovl3_e.text  {*(.ovl3_e.literal  .ovl3_e.text  .ovl3_e.rodata)}
        .ovl3_f.text  {*(.ovl3_f.literal  .ovl3_f.text  .ovl3_f.rodata)}
        .ovl3_u.text  {*(.ovl3_u.literal  .ovl3_u.text  .ovl3_u.rodata)}
        .ovl3_i.text  {*(.ovl3_i.literal  .ovl3_i.text  .ovl3_i.rodata)}

    } >iram_seg AT>irom_seg :irom_phdr

    .ovl3_end :
    {
        _ovl3_start = ADDR(.ovl3_e.text);
        _ovl3_end   = ABSOLUTE(.);
    }

    __load_size_ovl3_etext  = __load_stop_ovl3_etext  - __load_start_ovl3_etext;
    __load_size_ovl3_ftext  = __load_stop_ovl3_ftext  - __load_start_ovl3_ftext;
    __load_size_ovl3_utext  = __load_stop_ovl3_utext  - __load_start_ovl3_utext;
    __load_size_ovl3_itext  = __load_stop_ovl3_itext  - __load_start_ovl3_itext;

/*----------------------------------------------------------------------- */

    .iram_startup.text : ALIGN(4)
    {
        *(.startup.literal .startup.text)

    } >iram_bk_seg : iram_bk_phdr

/*  .iram_bk.bss (NOLOAD) : ALIGN(4) {*(.iram_bk.bss)} >iram_bk_seg : iram_bk_phdr */

    .irom.text : ALIGN(4)
    {
        _irom_text_start = ABSOLUTE(.);

        *(.irom0.literal .irom.literal .irom.text.literal .irom0.text .irom.text)

        _irom_text_end   = ABSOLUTE(.);

    } >irom_seg :irom_phdr
}

/* get ROM code address */
INCLUDE "ld/eagle.rom.addr.v6.ld"
