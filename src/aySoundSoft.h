#pragma once
#include "inttypes.h" 

void AY_select_reg(uint8_t N_reg);
uint8_t AY_get_reg();
void AY_set_reg(uint8_t val);

uint8_t*  get_AY_Out(uint8_t delta);
void  AY_reset();
void AY_print_state_debug();
void AY_write_address(uint16_t addr);
