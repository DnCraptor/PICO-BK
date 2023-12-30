#ifndef PS2_DRV_H
#define PS2_DRV_H

#include <stdint.h>

#define PS2_RX_BUF_SIZE    16

typedef struct
{
    uint32_t  IntPrevT;
    uint32_t  PrevT;
    uint16_t  RxBuf [PS2_RX_BUF_SIZE];
    uint16_t  PrevCode;

    uint16_t  TxData;
    uint16_t  RxData;
    uint16_t  IntState;

    uint8_t   iRxBufRd;
    uint8_t   iRxBufWr;

    uint8_t   State;
    uint8_t   TxByte;
    uint8_t   LedsNew;
    uint8_t   LedsLast;

} Tps2_Data;

extern Tps2_Data ps2_Data;

#define PS2_INT_STATE_FLAG_E0      0x0001
#define PS2_INT_STATE_FLAG_E1      0x0002
#define PS2_INT_STATE_FLAG_TX      0x0004
#define PS2_INT_STATE_FLAG_BAT     0x0008
#define PS2_INT_STATE_FLAG_ACK     0x0010
#define PS2_INT_STATE_FLAG_TXACK   0x0020
#define PS2_INT_STATE_FLAG_RESEND  0x0040
#define PS2_INT_STATE_FLAG_F0      0x0080
#define PS2_INT_STATE_BITN_POS     9
#define PS2_INT_STATE_BITN_MASK    0xFE00

#endif
