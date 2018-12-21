#ifndef TAPE_H
#define TAPE_H

#include "ovl.h"

#define tape_ReadOp_ovln  OVL_NUM_FS
#define tape_ReadOp_ovls  OVL_SEC_FS
#define tape_WriteOp_ovln OVL_NUM_FS
#define tape_WriteOp_ovls OVL_SEC_FS

void tape_ReadOp  (void);
void tape_WriteOp (void);

#endif
