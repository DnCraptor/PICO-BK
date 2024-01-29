#pragma once

#define DPAD_LEFT 0x40
#define DPAD_RIGHT 0x80
#define DPAD_DOWN 0x20
#define DPAD_UP 0x10
#define DPAD_START 0x08
#define DPAD_SELECT 0x04
#define DPAD_B 0x02
#define DPAD_A 0x01
#include <stdint.h>
#include <stdbool.h>

extern uint8_t nespad_state;    // NES Joystick1
extern uint8_t nespad_state2;    // NES Joystick1
extern bool nespad_available;
extern volatile bool is_dendy_joystick;
extern volatile bool is_kbd_joystick;
#define DPAD_STATE_DELAY 100
extern int nespad_state_delay;

bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin, uint8_t latPin);
void nespad_read();


