#ifndef ORANGE_BK_INPUT_LINUX_H
#define ORANGE_BK_INPUT_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * evdev keyboards and game controllers (/dev/input/event*), the Linux
 * counterpart of the PS/2 keyboard, USB HID and NES (Dendy) gamepad drivers
 * of the RP2350 build.  Keyboards are grabbed exclusively so key presses do not
 * also reach the text console underneath the KMS output.  Devices are
 * hot-plugged by periodic rescans.
 */

void input_init(void);
void input_shutdown(void);

/* Rescan for new devices (rate-limited) and read pending events.  Keyboard
 * events are passed to on_key (Linux input code; value 1 = press,
 * 2 = autorepeat, 0 = release) in the order they arrived. */
void input_poll(void (*on_key)(int code, int value));

/* Controller buttons (INPUT_PAD_* bits). */
#define INPUT_PAD_A      0x01
#define INPUT_PAD_B      0x02
#define INPUT_PAD_SELECT 0x04
#define INPUT_PAD_START  0x08
#define INPUT_PAD_UP     0x10
#define INPUT_PAD_DOWN   0x20
#define INPUT_PAD_LEFT   0x40
#define INPUT_PAD_RIGHT  0x80

/* State of game controller n (0: first connected, 1: second), as
 * INPUT_PAD_* bits; 0 when there is no such controller. */
uint32_t input_gamepad_state(int n);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_INPUT_LINUX_H */
