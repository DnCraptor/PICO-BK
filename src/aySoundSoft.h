#pragma once
#include "inttypes.h" 
#include "pico/platform.h"

#define FAST_FUNC __time_critical_func

void FAST_FUNC(AY_select_reg)(uint8_t N_reg);
uint8_t AY_get_reg();
void FAST_FUNC(AY_set_reg)(uint8_t val);

uint8_t*  get_AY_Out(uint8_t delta);
void  AY_reset();
void AY_print_state_debug();
void AY_write_address(uint16_t addr);

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
