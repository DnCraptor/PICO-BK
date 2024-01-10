#pragma once
#include "inttypes.h" 
#include "pico/platform.h"

#define FAST_FUNC __time_critical_func

void AY_select_reg(uint8_t g_chip, uint8_t N_reg);
void AY_set_reg(uint8_t g_chip, uint8_t val);

uint8_t* get_AY_Out(uint8_t g_chip, uint8_t delta);
/*"-=[Sound OFF]=-", - 0
  "  Only Beeper  ", - 1
  " Beeper+SoftAY ", - 2
  " Beeper+SoftTS ", - 3
  " HW TurboSound " - 4
*/
void AY_reset(uint8_t s_mode);
void AY_print_state_debug();
void AY_to_beep(bool Beep);
void AY_write_address(uint16_t addr);
void AY_write_data(uint8_t data);

#ifdef MNGR_DEBUG
extern void logMsg(char* msg);
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#define DBGM_PRINT( X) printf X
#else
#define DBGM_PRINT( X)
#endif

extern volatile bool is_covox_on;
extern volatile bool is_ay_on;
extern volatile bool is_sound_on;
extern volatile uint8_t snd_divider;
extern volatile int8_t covox_multiplier;
extern volatile uint16_t true_covox;
