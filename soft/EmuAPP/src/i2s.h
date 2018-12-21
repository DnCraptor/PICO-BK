#ifndef I2S_H
#define I2S_H


#include "ovl.h"
#include "ets.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef const uint32_t* (*i2s_cb_t)(void);


#define i2s_init_ovln  OVL_NUM_INIT
#define i2s_init_ovls  OVL_SEC_INIT
#define i2s_start_ovln OVL_NUM_INIT
#define i2s_start_ovls OVL_SEC_INIT
void i2s_init(i2s_cb_t cb, int size);
void i2s_start(void);


#ifdef __cplusplus
}
#endif


#endif

