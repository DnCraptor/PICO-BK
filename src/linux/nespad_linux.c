/*
 * Linux counterpart of drivers/nespad: the two NES (Dendy) pads are the
 * first two evdev game controllers.  INPUT_PAD_* bits have the DPAD_* layout
 * of nespad.h.
 */
#include "nespad.h"
#include "input_linux.h"

_Static_assert(INPUT_PAD_A == DPAD_A && INPUT_PAD_B == DPAD_B &&
	       INPUT_PAD_SELECT == DPAD_SELECT && INPUT_PAD_START == DPAD_START &&
	       INPUT_PAD_UP == DPAD_UP && INPUT_PAD_DOWN == DPAD_DOWN &&
	       INPUT_PAD_LEFT == DPAD_LEFT && INPUT_PAD_RIGHT == DPAD_RIGHT,
	       "INPUT_PAD_* must match DPAD_*");

uint8_t usbpad_state = 0;
uint8_t nespad_state = 0;
uint8_t nespad_state2 = 0;
bool nespad_available = true;

bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin, uint8_t latPin)
{
	(void)cpu_khz;
	(void)clkPin;
	(void)dataPin;
	(void)latPin;
	return true;
}

void nespad_read(void)
{
	nespad_state = (uint8_t)input_gamepad_state(0);
	nespad_state2 = (uint8_t)input_gamepad_state(1);
}
