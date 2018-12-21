/* This linker script generated from xt-genldscripts.tpp for LSP . */
/* Linker Script for ld -N */
MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  bkram_seg :                           org = 0x3FFE8000, len = 0x8000
  bkrom_seg :                           org = 0x3FFF0000, len = 0x8000
  dram0_0_seg :                         org = 0x3FFF8000, len = 0x4000
  iram1_0_seg :                         org = 0x40100000, len = 0xF000
  iram1_1_seg :                         org = 0x4010F000, len = 0x0C00  /* last 256b for loader */
  irom0_0_seg :                         org = 0x40201010, len = 0xEFF0
}

PHDRS
{
  dport0_0_phdr     PT_LOAD;
  bkram_phdr        PT_LOAD;
  bkrom_phdr        PT_LOAD;
  dram0_0_phdr      PT_LOAD;
  dram0_0_bss_phdr  PT_LOAD;
  iram1_0_phdr      PT_LOAD;
  iram1_1_phdr      PT_LOAD;
  irom0_0_phdr      PT_LOAD;
}


/*  Default entry point:  */
ENTRY(call_user_start)
EXTERN(_DebugExceptionVector)
EXTERN(_DoubleExceptionVector)
EXTERN(_KernelExceptionVector)
EXTERN(_NMIExceptionVector)
EXTERN(_UserExceptionVector)
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
  .dport0.rodata : ALIGN(4)
  {
    _dport0_rodata_start = ABSOLUTE(.);
    *(.dport0.rodata)
    *(.dport.rodata)
    _dport0_rodata_end = ABSOLUTE(.);
  } >dport0_0_seg :dport0_0_phdr

  .dport0.literal : ALIGN(4)
  {
    _dport0_literal_start = ABSOLUTE(.);
    *(.dport0.literal)
    *(.dport.literal)
    _dport0_literal_end = ABSOLUTE(.);
  } >dport0_0_seg :dport0_0_phdr

  .dport0.data : ALIGN(4)
  {
    _dport0_data_start = ABSOLUTE(.);
    *(.dport0.data)
    *(.dport.data)
    _dport0_data_end = ABSOLUTE(.);
  } >dport0_0_seg :dport0_0_phdr

  .bkram.bss (NOLOAD) : ALIGN(4)
  {
    _bkram_data_start = ABSOLUTE(.);
    *(.bkram.bss)
    _bkram_data_end   = ABSOLUTE(.);
  } >bkram_seg :bkram_phdr

  .bkrom.data : ALIGN(4)
  {
    _bkrom_data_start = ABSOLUTE(.);
    *(.bkrom.data)
    _bkrom_data_end   = ABSOLUTE(.);
  } >bkrom_seg :bkrom_phdr

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
  } >dram0_0_seg :dram0_0_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.sdk.version)
    *(.rodata)
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
    . = ALIGN(4);       /* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

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
/*    _stack_sentry = ALIGN(0x8); */
  } >dram0_0_seg :dram0_0_bss_phdr
/* __stack = 0x3ffc8000; */

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
  } >iram1_0_seg :iram1_0_phdr

  .lit4 : ALIGN(4)
  {
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
  } >iram1_0_seg :iram1_0_phdr

  OVERLAY : NOCROSSREFS
  {
    .ovl0.text {*(.ovl0.literal .ovl0.text)}
    .ovl1.text {*(.ovl1.literal .ovl1.text)}
    .ovl2.text {*(.ovl2.literal .ovl2.text)}
    .ovl3.text {*(.ovl3.literal .ovl3.text)}
    .ovl4.text {*(.ovl4.literal .ovl4.text)}
    .ovl5.text {*(.ovl5.literal .ovl5.text)}
    .ovl6.text {*(.ovl6.literal .ovl6.text)}
    .ovl7.text {*(.ovl7.literal .ovl7.text)}
    .ovl8.text {*(.ovl8.literal .ovl8.text)}
    .ovl9.text {*(.ovl9.literal .ovl9.text)}
  } >iram1_0_seg AT>irom0_0_seg :irom0_0_phdr
  _ovl_start = ADDR(.ovl0.text);
  _ovl_end = ABSOLUTE(.);
  __load_size_ovl0text = __load_stop_ovl0text - __load_start_ovl0text;
  __load_size_ovl1text = __load_stop_ovl1text - __load_start_ovl1text;
  __load_size_ovl2text = __load_stop_ovl2text - __load_start_ovl2text;
  __load_size_ovl3text = __load_stop_ovl3text - __load_start_ovl3text;
  __load_size_ovl4text = __load_stop_ovl4text - __load_start_ovl4text;
  __load_size_ovl5text = __load_stop_ovl5text - __load_start_ovl5text;
  __load_size_ovl6text = __load_stop_ovl6text - __load_start_ovl6text;
  __load_size_ovl7text = __load_stop_ovl7text - __load_start_ovl7text;
  __load_size_ovl8text = __load_stop_ovl8text - __load_start_ovl8text;
  __load_size_ovl9text = __load_stop_ovl9text - __load_start_ovl9text;

  .irom0.text : ALIGN(4)
  {
    _irom0_text_start = ABSOLUTE(.);
    *(.irom0.literal .irom.literal .irom.text.literal .irom0.text .irom.text)
    _irom0_text_end = ABSOLUTE(.);
  } >irom0_0_seg :irom0_0_phdr

  .startup.text : ALIGN(4)
  {
    *(.startup.literal .startup.text)

  } >iram1_1_seg :iram1_1_phdr
}

/* get ROM code address */
INCLUDE "ld/eagle.rom.addr.v6.ld"
