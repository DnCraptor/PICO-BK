#pragma once

/*
 * Linux counterpart of drivers/ps2kbd/ps2.h: the keyboard functions that
 * src/linux/kbd_linux.cpp provides (without the driver's unused PS/2 scan
 * code tables).
 */

#include <stdint.h>
#include <stdbool.h>

void keyboard_init(void);
void keyboard_tick(void);

void ps2cleanup();
uint32_t ps2get_raw_code();
void push_script_scan_code(uint16_t sc);
