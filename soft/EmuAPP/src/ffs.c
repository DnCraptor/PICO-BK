#include <stdint.h>
#include "ffs.h"

uint8_t  free_sect [128];  // максимум 4мб
uint32_t f_size;
