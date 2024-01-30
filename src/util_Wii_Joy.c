#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include <pico/stdlib.h>
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "util_Wii_Joy.h"

struct WIIController Wii_joy = { 0 };

bool WII_Init = false;
bool is_WII_Init() {
	return WII_Init;
}
uint8_t WII_Data[WII_BYTE_COUNT];
uint8_t WII_Data_Old[WII_BYTE_COUNT];
uint8_t WII_Calibrate[WII_CONFIG_COUNT];

uint8_t WII_Nes;
//uint8_t WII_Nes_OLD;
uint8_t WII_Data_Format = 0xFF;
uint8_t WII_Device_Type = 0xFF;

const uint8_t init_cmd[2]={0x40,0x00};
const uint8_t init_cmd0[2]={0xF0,0x55};
const uint8_t init_cmd1[2]={0xFB,0x00};
const uint8_t init_cmd3[1]={0xFA};
const uint8_t set_hires_cmd[2]={0xFE,0x03};
const uint8_t set_lores_cmd[2]={0xFE,0x01};

//const uint8_t init_data[] = { 0xF0, 0x55, 0xFB, 0x00, 0xFE, 0x03 };

#ifdef MNGR_DEBUG
#define printf(...) { char tmp[80]; snprintf(tmp, 80, __VA_ARGS__); logMsg(tmp); }
#else
#define printf(...) {}
#endif

bool controller_decode_bytes_uncompressed(uint8_t *buf, size_t len, struct WIIController *tempData){ //mode 3
	
	if ((buf == NULL) || (len < WII_BYTE_COUNT) || (tempData == NULL)) {
		return false;
	}
	
	tempData->LeftX 		= (int)(buf[0]-WII_Calibrate[0]);// - LX_center
	tempData->LeftY 		= (int)(buf[2]-WII_Calibrate[2]);// - LY_center
	tempData->RightX 		= (int)(buf[1]-WII_Calibrate[1]);// - RX_center
	tempData->RightY		= (int)(buf[3]-WII_Calibrate[3]);// - RY_center
	tempData->LeftT 		= (WII_Calibrate[4] > buf[4]) ? 0 : (int)(buf[4] - -WII_Calibrate[4]);
	tempData->RightT 		= (-WII_Calibrate[5] > buf[5]) ? 0 : (int)(buf[5] - -WII_Calibrate[5]);
	
	tempData->ButtonR 		= (~(buf[6] >> 1) & 1);
	tempData->ButtonStart 	= (~(buf[6] >> 2) & 1);
	tempData->ButtonHome 	= (~(buf[6] >> 3) & 1);
	tempData->ButtonSelect 	= (~(buf[6] >> 4) & 1);
	tempData->ButtonL 		= (~(buf[6] >> 5) & 1);
	tempData->ButtonDown 	= (~(buf[6] >> 6) & 1);
	tempData->ButtonRight 	= (~(buf[6] >> 7) & 1);
	
	tempData->ButtonLeft 	= (~(buf[7] >> 1) & 1);
	tempData->ButtonZR 		= (~(buf[7] >> 2) & 1);
	tempData->ButtonX 		= (~(buf[7] >> 3) & 1);
	tempData->ButtonA 		= (~(buf[7] >> 4) & 1);
	tempData->ButtonY 		= (~(buf[7] >> 5) & 1);
	tempData->ButtonB 		= (~(buf[7] >> 6) & 1);
	tempData->ButtonZL 		= (~(buf[7] >> 7) & 1);
	tempData->ButtonUp 		= (~buf[7] & 1);
	return true;
}

bool controller_decode_bytes_uncompressed_alt(uint8_t *buf, size_t len, struct WIIController *tempData){ //mode 2
	
	if ((buf == NULL) || (len < WII_BYTE_COUNT) || (tempData == NULL)) {
		return false;
	}
	
	tempData->LeftX 		= (int)(buf[0]-WII_Calibrate[0]);// - LX_center
	tempData->LeftY 		= (int)(buf[2]-WII_Calibrate[2]);// - LY_center
	tempData->RightX 		= (int)(buf[1]-WII_Calibrate[1]);// - RX_center
	tempData->RightY		= (int)(buf[3]-WII_Calibrate[3]);// - RY_center
	tempData->LeftT 		= (WII_Calibrate[4] > buf[5]) ? 0 : (int)(buf[5] - -WII_Calibrate[4]);
	tempData->RightT 		= (-WII_Calibrate[5] > buf[6]) ? 0 : (int)(buf[6] - -WII_Calibrate[5]);
	
	tempData->ButtonR 		= (~(buf[7] >> 1) & 1);
	tempData->ButtonStart 	= (~(buf[7] >> 2) & 1);
	tempData->ButtonHome 	= (~(buf[7] >> 3) & 1);
	tempData->ButtonSelect 	= (~(buf[7] >> 4) & 1);
	tempData->ButtonL 		= (~(buf[7] >> 5) & 1);
	tempData->ButtonDown 	= (~(buf[7] >> 6) & 1);
	tempData->ButtonRight 	= (~(buf[7] >> 7) & 1);
	
	tempData->ButtonLeft 	= (~(buf[8] >> 1) & 1);
	tempData->ButtonZR 		= (~(buf[8] >> 2) & 1);
	tempData->ButtonX 		= (~(buf[8] >> 3) & 1);
	tempData->ButtonA 		= (~(buf[8] >> 4) & 1);
	tempData->ButtonY 		= (~(buf[8] >> 5) & 1);
	tempData->ButtonB 		= (~(buf[8] >> 6) & 1);
	tempData->ButtonZL 		= (~(buf[8] >> 7) & 1);
	tempData->ButtonUp 		= (~buf[8] & 1);
	return true;
}

bool controller_decode_bytes_compressed(uint8_t *buf, size_t len, struct WIIController *tempData){ //mode 1
	
	if ((buf == NULL) || (len < WII_BYTE_COUNT) || (tempData == NULL)) {
		return false;
	}
	
	tempData->LeftX = (buf[0] & (64 - 1)) - 32; // 0 to 63
	tempData->LeftY = (buf[1] & (64 - 1)) - 32; // 0 to 63 -> -32 to 31
	
	tempData->RightX = (((buf[2] >> 7) & 1) + ((buf[1] >> 6) & 3) * 2 + ((buf[0] >> 6) & 3) * 8) - 16; // 0 to 31 -> -16 to 15
	tempData->RightY = (buf[2] & (32 - 1)) - 16; // 0 to 31 -> -16 to 15
	
	tempData->LeftT = (((buf[2] >> 5) & 3) * 8 + ((buf[3] >> 5) & 7));
	tempData->RightT = (buf[3] & (32 - 1));
	
	tempData->ButtonDown = ((buf[4] & (1 << 6)) == 0);
	tempData->ButtonLeft = ((buf[5] & (1 << 1)) == 0);
	tempData->ButtonUp = ((buf[5] & (1 << 0)) == 0);
	tempData->ButtonRight = ((buf[4] & (1 << 7)) == 0);
	tempData->ButtonSelect = ((buf[4] & (1 << 4)) == 0);
	tempData->ButtonHome = ((buf[4] & (1 << 3)) == 0);
	tempData->ButtonStart = ((buf[4] & (1 << 2)) == 0);
	tempData->ButtonY = ((buf[5] & (1 << 5)) == 0);
	tempData->ButtonX = ((buf[5] & (1 << 3)) == 0);
	tempData->ButtonA = ((buf[5] & (1 << 4)) == 0);
	tempData->ButtonB = ((buf[5] & (1 << 6)) == 0);
	tempData->ButtonL = ((buf[4] & (1 << 5)) == 0);
	tempData->ButtonR = ((buf[4] & (1 << 1)) == 0);
	tempData->ButtonZL = ((buf[5] & (1 << 7)) == 0);
	tempData->ButtonZR = ((buf[5] & (1 << 2)) == 0);
	return true;
}

bool controller_decode_bytes_nunchuck(uint8_t *buf, size_t len, struct WIIController *tempData){ // mode 0
	if ((buf == NULL) || (len < WII_BYTE_COUNT) || (tempData == NULL)) {
		return false;
	}
	
	tempData->LeftX 		= (int)(buf[0]);
	tempData->LeftY 		= (int)(buf[1]);
	
	tempData->AccX 			= (int)(buf[2]);
	tempData->AccY			= (int)(buf[3]);
	tempData->AccZ 			= (int)(buf[4]);
	
	tempData->ButtonA	 	= (~(buf[7] >> 1) & 1);
	tempData->ButtonB 		= (~buf[7] & 1);
	
	return true;
}

bool Init_Wii_Joystick() {
	uint8_t result;
	printf("Init_Wii_Joystick0");
	i2c_init(WII_PORT, WII_CLOCK);
	printf("Init_Wii_Joystick1");
	gpio_set_function(WII_SDA_PIN, GPIO_FUNC_I2C);
	printf("Init_Wii_Joystick2");
	gpio_set_function(WII_SCL_PIN, GPIO_FUNC_I2C);
	printf("Init_Wii_Joystick3");
	gpio_pull_up(WII_SDA_PIN);
	printf("Init_Wii_Joystick4");
	gpio_pull_up(WII_SCL_PIN);
	
	printf("Begin Wii Init\n");
	
	//PICO_ERROR_GENERIC
	result=i2c_write_blocking(WII_PORT, WII_ADDRESS, &init_cmd0[0], 2, false); //Init 0
	if(result!=2){
		printf("Wii>Error first init\n");
		WII_Init = false;
		return false;
	}
	sleep_us(50);
	result=i2c_write_blocking(WII_PORT, WII_ADDRESS, &init_cmd1[0], 2, false); //Disable encode
	if(result==2){
		printf("Wii>No Code\n");
		} else {
		printf("Wii>Error second init\n");
		WII_Init = false;
		return false;
	}
	sleep_us(50);
	result=i2c_write_blocking(WII_PORT, WII_ADDRESS, &init_cmd3[0], 1, false); //Read joy type
	if(result==1){
		sleep_us(50);
		result=i2c_read_blocking(WII_PORT, WII_ADDRESS, WII_Calibrate, WII_CONFIG_COUNT, false);
		if (result>WII_CONFIG_COUNT) {
			printf("Wii>Failed to get controller type:%d\n",result);
		} 
		WII_Data_Format = WII_Calibrate[4];
		WII_Device_Type = WII_Calibrate[5];
		printf("Joy type>");
		for(uint8_t i = 0; i < result; i++){
			printf(" %02X",WII_Calibrate[i]);
		}
		printf("\n");
		printf("WII_Data_Format> %02X \t WII_Device_Type> %02X \n",WII_Data_Format,WII_Device_Type);
		memset(&WII_Calibrate,0,6);
		if (result!= 6) {
			WII_Data_Format = 3;
		} 
		
		} else {
		printf("Wii>Error get controller type data\n");
		WII_Init = false;
		return false;		
	}
	sleep_us(50);
	result=i2c_write_blocking(WII_PORT, WII_ADDRESS, &set_hires_cmd[0], 2, false); //Set Hi res
	if(result==2){
		printf("Wii>HiRes mode\n");
		} else {
		printf("Wii>Error set hires mode\n");
		WII_Init = false;
		return false;
	}	
	sleep_ms(1);
	result=i2c_write_blocking(WII_PORT, WII_ADDRESS, 0x00, 1, false);
	sleep_ms(1);
	if(result==1){
		result=i2c_read_blocking(WII_PORT, WII_ADDRESS, WII_Calibrate, 6, false);
		if (result!= 6) {
			printf("Wii>Failed to calibrate controller\n");
			WII_Init = false;
			//return false;
		}
		
		printf("Wii>calibrate>");
		for(uint8_t i = 0; i < 6; i++){
			printf(" %02X",WII_Calibrate[i]);
		}
		printf("\n");
		
		} else {
		printf("Wii>Error get calibration data\n");
		WII_Init = false;
		return false;
	}
	WII_Init = true;
	return true;
}

void Deinit_Wii_Joystick(){
	i2c_deinit(WII_PORT);
	gpio_set_function(WII_SDA_PIN, GPIO_FUNC_NULL);
	gpio_set_function(WII_SCL_PIN, GPIO_FUNC_NULL);
	gpio_disable_pulls(WII_SDA_PIN);
	gpio_disable_pulls(WII_SCL_PIN);  	
}

bool Wii_decode_joy(){
	uint8_t result;
	bool decode = false;
	busy_wait_us(200);
	i2c_write_blocking(WII_PORT, WII_ADDRESS, 0x00, 1, false);	
	busy_wait_us(200);
	result = i2c_read_blocking(WII_PORT, WII_ADDRESS, &WII_Data[0], WII_BYTE_COUNT, false);
	if(result>=WII_BYTE_COUNT){
		if((WII_Data[0]==0x00)&&(WII_Data[1]==0x00)&&(WII_Data[2]==0x00)&&(WII_Data[3]==0x00)) return false;
		if((WII_Data[0]==0xFF)&&(WII_Data[1]==0xFF)&&(WII_Data[2]==0xFF)&&(WII_Data[3]==0xFF)) return false;
		if(memcmp(&WII_Data[0],&WII_Data_Old[0],WII_BYTE_COUNT)!=0){
			/*
				printf("data>");
				for (uint8_t i = 0; i < 8; i++) {
				printf(" %02X",WII_Data[i]);
				}
			*/
			printf("\n");
			switch (WII_Data_Format) {
				case 0:
				decode = controller_decode_bytes_nunchuck(WII_Data, result, &Wii_joy);
				break;
				case 1:
				decode = controller_decode_bytes_compressed(WII_Data, result, &Wii_joy);
				break;
				case 2:
				decode = controller_decode_bytes_uncompressed_alt(WII_Data, result, &Wii_joy);
				break;
				case 3:
				decode = controller_decode_bytes_uncompressed(WII_Data, result, &Wii_joy);
				break;
				default:
				return false;
			}		
			
		} else return false;
		memcpy(&WII_Data_Old[0],&WII_Data[0],WII_BYTE_COUNT);
		return decode;
	}
	return false;
}

void Wii_clear_old(){
	memset(&WII_Data_Old[0],0,WII_BYTE_COUNT);
}

void Wii_debug(struct WIIController *tempData){
	
	const char fillCharacter = '_';
	
	const char dpadLPrint = tempData->ButtonLeft ? '<' : fillCharacter;
	const char dpadUPrint = tempData->ButtonUp ? '^' : fillCharacter;
	const char dpadDPrint = tempData->ButtonDown ? 'v' : fillCharacter;
	const char dpadRPrint = tempData->ButtonRight ? '>' : fillCharacter;
	
	const char aButtonPrint = tempData->ButtonA ? 'A' : fillCharacter;
	const char bButtonPrint = tempData->ButtonB ? 'B' : fillCharacter;
	const char xButtonPrint = tempData->ButtonX ? 'X' : fillCharacter;
	const char yButtonPrint = tempData->ButtonY ? 'Y' : fillCharacter;
	
	const char plusPrint = tempData->ButtonStart ? '+' : fillCharacter;
	const char minusPrint = tempData->ButtonSelect ? '-' : fillCharacter;
	const char homePrint = tempData->ButtonHome ? 'H' : fillCharacter;
	
	const char ltButtonPrint = tempData->ButtonL ? 'X' : fillCharacter;
	const char rtButtonPrint = tempData->ButtonR ? 'X' : fillCharacter;
	
	const char zlButtonPrint = tempData->ButtonZL ? 'L' : fillCharacter;
	const char zrButtonPrint = tempData->ButtonZR ? 'R' : fillCharacter;
	
	printf("***");
	
	printf("%c%c%c%c | %c%c%c | %c%c%c%c L:(%3d, %3d) R:(%3d, %3d) | LT:%3d%c RT:%3d%c Z:%c%c\n",
		dpadLPrint, dpadUPrint, dpadDPrint, dpadRPrint,
		minusPrint, homePrint, plusPrint,
		aButtonPrint, bButtonPrint, xButtonPrint, yButtonPrint,
		tempData->LeftX, tempData->LeftY, tempData->RightX, tempData->RightY,
		tempData->LeftT, ltButtonPrint, tempData->RightT, rtButtonPrint,
	zlButtonPrint, zrButtonPrint);
}

uint8_t map_to_nes(struct WIIController *tempData){
	memcpy(&WII_Data_Old[0],&WII_Data[0],6);
	uint8_t result = 0;
	/*
		if (tempData->ButtonHome)	{ printf("\tButtonHome");}
		if (tempData->ButtonL)		{ printf("\tButtonL");}
		if (tempData->ButtonR)		{ printf("\tButtonR");}
		if (tempData->ButtonZL)		{ printf("\tButtonZL");}
		if (tempData->ButtonZR)		{ printf("\tButtonZR");}
	*/
	if (tempData->ButtonStart)	{ result|=0x80;}
	if (tempData->ButtonSelect)	{ result|=0x40;}
	
	//if (tempData->ButtonX)	{ result|=0x20;}
	//if (tempData->ButtonY)	{ result|=0x10;}
	
	if (tempData->ButtonA)		{ result|=0x20;}
	if (tempData->ButtonB)		{ result|=0x10;}
	
	if(tempData->LeftX>WII_LEFT_HALF)	   { result|=0x01;} //right
	if(tempData->LeftX<(0-WII_LEFT_HALF))   { result|=0x02;} //left
	if(tempData->LeftY>WII_LEFT_HALF)	   { result|=0x08;} //up
	if(tempData->LeftY<(0-WII_LEFT_HALF))   { result|=0x04;} //down
	
	//if(tempData->RightX>WII_RIGHT_HALF)	   { result|=0x20;} //A
	//if(tempData->RightX<(0-WII_RIGHT_HALF))   { result|=0x02;} //Y
	//if(tempData->RightY>WII_RIGHT_HALF)	   { result|=0x08;} //X
	//if(tempData->RightY<(0-WII_RIGHT_HALF))   { result|=0x10;} //B
	
	if (tempData->ButtonRight)	{ result|=0x01;}
	if (tempData->ButtonLeft)	{ result|=0x02;}
	if (tempData->ButtonDown)	{ result|=0x04;}
	if (tempData->ButtonUp)		{ result|=0x08;}
	return result;
}


