#ifndef STR_H
#define STR_H


#include "ovl.h"


#ifdef __cplusplus
extern "C" {
#endif


int to_upper(int c);

#define strnlen_ovln OVL_NUM_UI
#define strnlen_ovls OVL_SEC_UI
int strnlen(const char *s, int maxlen);

#ifdef __cplusplus
}
#endif


#endif
