#include <stdint.h>
#include "CPU_i.h"
#include "main_i.h"
#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_i.text")))

extern uint32_t VecBase;

void AT_OVL main_init (void) {
    // Инитим процессор
    CPU_Init ();
}
