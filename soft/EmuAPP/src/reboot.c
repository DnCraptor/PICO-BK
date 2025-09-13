#include "reboot.h"
#include "hardware/watchdog.h"

void reboot() {
    watchdog_enable(100, true);
    while(1);
}
