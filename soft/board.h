#ifndef BOARD_H
#define BOARD_H


//#define DO_DEBUG

#ifdef DO_DEBUG
    // NodeMCU debug
    #define TV_SYNC         15
    #define PS2_DATA        13
    #define PS2_CLK         12

    #define TV_SYNC_FUNC    FUNC_GPIO15
    #define PS2_DATA_FUNC   FUNC_GPIO13
    #define PS2_CLK_FUNC    FUNC_GPIO12

    #define TV_SYNC_REG     PERIPHS_IO_MUX_GPIO15_U
    #define PS2_DATA_REG    PERIPHS_IO_MUX_GPIO13_U
    #define PS2_CLK_REG     PERIPHS_IO_MUX_GPIO12_U

#else
    // ESP-01 LUT
    #define TV_SYNC     1
    #define PS2_DATA    2
    #define PS2_CLK     0

    #define TV_SYNC_FUNC    FUNC_GPIO1
    #define PS2_DATA_FUNC   FUNC_GPIO2
    #define PS2_CLK_FUNC    FUNC_GPIO0

    #define TV_SYNC_REG     PERIPHS_IO_MUX_GPIO1_U
    #define PS2_DATA_REG    PERIPHS_IO_MUX_GPIO2_U
    #define PS2_CLK_REG     PERIPHS_IO_MUX_GPIO0_U

#endif

#define BEEPER      14
#define BEEPER_FUNC FUNC_GPIO14
#define BEEPER_REG  PERIPHS_IO_MUX_GPIO14_U


#endif
