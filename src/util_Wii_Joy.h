/**
 * Wii-joystick driver
 * Author: @tecnocat
 * bool Init_Wii_Joystick(); - вызвать на старте
 * void Deinit_Wii_Joystick();
 * bool Wii_decode_joy(); - вызывать при опросе
 * void Wii_clear_old();
 * void Wii_debug(struct WIIController *tempData);
 * uint8_t map_to_nes(struct WIIController *tempData); - готовый маппинг на NES джойстик, остальное уже как пожелают
*/
#pragma once
#include "inttypes.h"

#define WII_PORT (i2c1)
#define WII_CLOCK (100000) //4000
#define WII_ADDRESS 0x52
#define WII_BYTE_COUNT (21)
#define WII_CONFIG_COUNT (21)

#define WII_LEFT_HALF 64
#define WII_RIGHT_HALF 64


struct WIIController {
  int LeftX;
  int LeftY;
  int RightX;
  int RightY;
  int LeftT;
  int RightT;
  int AccX;
  int AccY;
  int AccZ;
  bool ButtonDown;
  bool ButtonLeft;
  bool ButtonUp;
  bool ButtonRight;
  bool ButtonSelect;
  bool ButtonHome;
  bool ButtonStart;
  bool ButtonY;
  bool ButtonX;
  bool ButtonA;
  bool ButtonB;
  bool ButtonL;
  bool ButtonR;
  bool ButtonZL;
  bool ButtonZR;
};

extern struct WIIController Wii_joy;

bool Init_Wii_Joystick();
void Deinit_Wii_Joystick();
bool Wii_decode_joy();
void Wii_clear_old();
void Wii_debug(struct WIIController *tempData);
uint8_t map_to_nes(struct WIIController *tempData);