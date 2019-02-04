OVL_FUNC_DESC (ffs_remove,       OVL_MODE_FFS)
OVL_FUNC_DESC (ffs_rename,       OVL_MODE_FFS)
OVL_FUNC_DESC (ffs_find,         OVL_MODE_FFS)
OVL_FUNC_DESC (tape_ReadOp,      OVL_MODE_FFS)
OVL_FUNC_DESC (tape_WriteOp,     OVL_MODE_FFS)
OVL_FUNC_DESC (menu,             OVL_MODE_UI)
OVL_FUNC_DESC (menu_fileman,     OVL_MODE_UI)


/*
    Init
    Emu
    Menu
    FileSystem

    Emu         Menu        FileSystem
    +                       +               (CPU_ReadMemW, CPU_ReadMemB, CPU_WriteMemW, CPU_WriteMemB, EmuFs_OvlData, emu_tv, emu_NewTvFrame, emu_OnTv)
    +           +                           ui_GetKey
                +           +               (ffs_name, ffs_get_fat)

Emu:            Emu        + ui_GetKey      + some emu funcs          + (CPU_ReadMemW, CPU_ReadMemB, CPU_WriteMemW, CPU_WriteMemB, EmuFs_OvlData, emu_tv, emu_NewTvFrame, emu_OnTv)
FileSystem:     FileSystem + some ffs funcs + (ffs_name, ffs_get_fat) + (CPU_ReadMemW, CPU_ReadMemB, CPU_WriteMemW, CPU_WriteMemB, EmuFs_OvlData, emu_tv, emu_NewTvFrame, emu_OnTv)
Menu:           Menu       + ui_GetKey      + (ffs_name, ffs_get_fat) + some menu funcs

OVL0_SEG_EMU_FFS
OVL0_SEG_UI
OVL0_SEG_INIT

OVL1_SEG_EMU_UI
OVL1_SEG_FFS
OVL1_SEG_INIT

OVL2_SEG_FFS_UI
OVL2_SEG_EMU
OVL2_SEG_INIT

OVL3_SEG_EMU
OVL3_SEG_FFS
OVL3_SEG_UI
OVL3_SEG_INIT

*/
