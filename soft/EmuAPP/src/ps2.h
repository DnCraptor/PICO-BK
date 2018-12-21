#ifndef PS2_DRV_H
#define PS2_DRV_H


#include "ovl.h"
#include "ets.h"
#include "ps2_codes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ps2_init_ovln       OVL_NUM_INIT
#define ps2_init_ovls       OVL_SEC_INIT
#define ps2_periodic_ovln 	OVL_NUM_EMU
#define ps2_periodic_ovls 	OVL_SEC_EMU
#define ps2_read_ovln 		OVL_NUM_EMU
#define ps2_read_ovls 		OVL_SEC_EMU
#define ps2_leds_ovln 		OVL_NUM_EMU
#define ps2_leds_ovls 		OVL_SEC_EMU

void ps2_init(void);
void ps2_periodic(void);

uint16_t ps2_read(void);
void ps2_leds(bool caps, bool num, bool scroll);

#ifdef __cplusplus
};
#endif


#endif
