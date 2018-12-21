#ifndef TV_H
#define TV_H


#include "ovl.h"
#include "ets.h"


#ifdef __cplusplus
extern "C" {
#endif

#define tv_init_ovln  OVL_NUM_INIT
#define tv_init_ovls  OVL_SEC_INIT
#define tv_start_ovln OVL_NUM_INIT
#define tv_start_ovls OVL_SEC_INIT
void tv_init(void);
void tv_start(void);

#ifdef __cplusplus
}
#endif


#endif
