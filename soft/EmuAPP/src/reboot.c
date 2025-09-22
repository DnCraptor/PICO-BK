#include "ff.h"
#include "reboot.h"
#include "hardware/watchdog.h"

FATFS fatfs = { 0 };

void reboot() {
    f_mount(&fatfs, "", 0);
    watchdog_enable(100, true);
    while(1);
    __unreachable();
}
