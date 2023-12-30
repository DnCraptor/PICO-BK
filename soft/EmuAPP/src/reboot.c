#include "reboot.h"
#include "hardware/watchdog.h"

void reboot(uint32_t value) {
    watchdog_enable(100, true);
    while(1);
}
