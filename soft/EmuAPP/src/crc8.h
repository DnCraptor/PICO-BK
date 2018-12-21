#ifndef CRC8_H
#define CRC8_H


#include "ovl.h"
#include "ets.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CRC8_INIT	0xFF
#define CRC8_OK		0x00

#define CRC8_ovln OVL_NUM_FS
#define CRC8_ovls OVL_SEC_FS
uint8_t CRC8(uint8_t crc, const uint8_t *data, uint8_t len);


#ifdef __cplusplus
}
#endif


#endif
