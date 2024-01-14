#pragma once
#include "inttypes.h" 
#include "pico/platform.h"
#include "config_em.h"

#define FAST_FUNC __time_critical_func

void FAST_FUNC(AY_select_reg)(uint8_t N_reg);
uint8_t AY_get_reg();
void FAST_FUNC(AY_set_reg)(uint8_t val);

uint8_t*  get_AY_Out(uint8_t delta);
void  AY_reset();
void AY_print_state_debug();
void AY_write_address(uint16_t addr);

extern volatile uint16_t true_covox;
extern volatile uint16_t az_covox_R;
extern volatile uint16_t az_covox_L;
