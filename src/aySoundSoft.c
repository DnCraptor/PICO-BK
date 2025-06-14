// from https://github.com/UmkaSK/speccy-pico-trd/blob/main/src/zx_emu/aySoundSoft.c
#include "aySoundSoft.h"
#include "stdbool.h"
#include "debug.h"

uint8_t N_sel_reg=0;

//регистры состояния AY
uint16_t ay_R1_R0=0;
uint16_t ay_R3_R2=0;
uint16_t ay_R5_R4=0;
uint8_t ay_R6=0;
uint8_t ay_R7=0;
uint8_t ay_R8=0;
uint8_t ay_R9=0;
uint8_t ay_R10=0;
uint16_t ay_R12_R11=0;
uint32_t ay_R12_R11_sh=0;
uint8_t ay_R13=0;
uint8_t ay_R14=0;
uint8_t ay_R15=0;

static bool is_envelope_begin=false;

void FAST_FUNC(AY_select_reg)(uint8_t N_reg)
{
//    DBGM_PRINT(("AY_select_reg(%d)", N_reg));
    N_sel_reg = N_reg;
};

void AY_reset()
{
    DBGM_PRINT(("AY_reset"));
    ay_R1_R0=0;
    ay_R3_R2=0;
    ay_R5_R4=0;
    ay_R6=0;
    ay_R7=0xFF;
    ay_R8=0;
    ay_R9=0;
    ay_R10=0;
    ay_R12_R11=0;
    ay_R12_R11_sh=0;
    ay_R13=0;
    ay_R14=0xFF;
    ay_R15=0;

};
void AY_print_state_debug()
{
    DBGM_PRINT(("AY STATE"));
    DBGM_PRINT(("%d",ay_R1_R0));
    DBGM_PRINT(("%d",ay_R3_R2));
    DBGM_PRINT(("%d",ay_R5_R4));
    DBGM_PRINT(("%d",ay_R6));
    DBGM_PRINT(("%d",ay_R7));
    DBGM_PRINT(("%d",ay_R8));
    DBGM_PRINT(("%d",ay_R9));
    DBGM_PRINT(("%d",ay_R10));
    DBGM_PRINT(("%d",ay_R12_R11));
    DBGM_PRINT(("%d",ay_R13));
    DBGM_PRINT(("%d",ay_R14));
    DBGM_PRINT(("%d",ay_R15));
};
uint8_t FAST_FUNC(AY_get_reg)()
    {
        switch (N_sel_reg)
        {
        case 0: return (ay_R1_R0)&0xff;
        case 1: return (ay_R1_R0>>8)&0xff;
        case 2: return (ay_R3_R2)&0xff;
        case 3: return (ay_R3_R2>>8)&0xff;
        case 4: return (ay_R5_R4)&0xff;
        case 5: return (ay_R5_R4>>8)&0xff;
        case 6: return ay_R6;
        case 7: return ay_R7;
        case 8: return ay_R8;
        case 9: return ay_R9;
        case 10: return ay_R10;
        
        
        case 11: return (ay_R12_R11)&0xff;
        case 12: return (ay_R12_R11>>8)&0xff;
        case 13: return ay_R13;
        case 14: return ay_R14;
        case 15: return ay_R15;
   
        default: return 0;
        }
    };


void FAST_FUNC(AY_set_reg)(uint8_t val)
    {
        DBGM_PRINT(("AY_set_reg(%02Xh)", val));
        switch (N_sel_reg)
        {
        case 0:
            ay_R1_R0=(ay_R1_R0&0xff00)|val;
            break;
        case 1:
//            ay_R1_R0=(ay_R1_R0&0xff)|((val)<<8);
            ay_R1_R0=(ay_R1_R0&0xff)|((val&0xf)<<8);
            break;
        case 2:
            ay_R3_R2=(ay_R3_R2&0xff00)|val;
            break;
        case 3:
//            ay_R3_R2=(ay_R3_R2&0xff)|((val)<<8);
            ay_R3_R2=(ay_R3_R2&0xff)|((val&0xf)<<8);
            break;
        case 4:
            ay_R5_R4=(ay_R5_R4&0xff00)|val;
            break;
        case 5:
//            ay_R5_R4=(ay_R5_R4&0xff)|((val)<<8);
            ay_R5_R4=(ay_R5_R4&0xff)|((val&0xf)<<8);

            break;
        case 6:
            ay_R6=val&0x1f;
            break;
        case 7:
            ay_R7=val;
            break;
        case 8:
            ay_R8=val&0x1f;            
            break;
        case 9:
            ay_R9=val&0x1f;
            break;
        case 10:
            ay_R10=val&0x1f;
            break;
        case 11:
            ay_R12_R11=(ay_R12_R11&0xff00)|val;
            ay_R12_R11_sh=ay_R12_R11<<1;
            break;
        case 12:
            ay_R12_R11=(ay_R12_R11&0xff)|(val<<8);
            ay_R12_R11_sh=ay_R12_R11<<1;

            break;
        case 13:
            ay_R13=val&0xf;
            is_envelope_begin=true;
            break;
        case 14:
            ay_R14=val;
            break;
        case 15:
            ay_R15=val;
            break;
        default:
            break;
        }
    };

//------------------------
 bool FAST_FUNC(get_random)(){


    static uint32_t S = 0x00000001;
       if (S & 0x00000001) {
           S = ((S ^ 0xea000001) >> 1) | 0x80000000;
           return true;}
       else {
           S >>= 1;
           return false;};

    
    // static uint16_t S = 0x0001;
    //    if (S & 0x0001) {
    //        S = ((S ^ 0xea01) >> 1) | 0x8000;
    //        return true;}
    //    else {
    //        S >>= 1;
    //        return false;};

}

//uint8_t ampls[]={0,1,2,3,4,5,6,8,10,12,16,19,23,28,34,40};//снятя с китайского чипа
//uint8_t ampls[]={0,3,5,8,11,13,16,19,21,24,27,29,32,35,37,40};//линейная
//uint8_t ampls[]={0,10,15,20,23,27,29,31,32,33,35,36,38,39,40,40};//выгнутая
//uint8_t ampls[]={0,1,2,2,3,3,4,6,7,9,12,15,19,25,32,41};//степенная зависимость


uint8_t ampls[]={0,2,4,5,7,8,9,11,12,14,16,19,23,28,34,40};//гибрид линейной и китайского чипа
uint8_t* ampls0=ampls;
uint8_t*  FAST_FUNC(get_AY_Out)(uint8_t delta)
{   
    
    //static uint32_t counts[7];

    
    
    static bool bools[4];


    #define chA_bit (bools[0])
    #define chB_bit (bools[1])
    #define chC_bit (bools[2])
    #define noise_bit (bools[3])
  
    static uint32_t main_ay_count_env=0;

    //отдельные счётчики
    static uint32_t chA_count=0;
    static uint32_t chB_count=0;
    static uint32_t chC_count=0;
    static uint32_t noise_ay_count=0;
    // static uint16_t ampl_chA=0;
    // static uint16_t ampl_chB=0;
    // static uint16_t ampl_chC=0;
    // const uint8_t shAmpl=0;

/*
N регистра	Назначение или содержание	Значение	
0, 2, 4 	Нижние 8 бит частоты голосов А, В, С 	0 - 255
1, 3, 5 	Верхние 4 бита частоты голосов А, В, С 	0 - 15
6 	        Управление частотой генератора шума 	0 - 31
7 	        Управление смесителем и вводом/выводом 	0 - 255
8, 9, 10 	Управление амплитудой каналов А, В, С 	0 - 15
11 	        Нижние 8 бит управления периодом пакета 	0 - 255
12 	        Верхние 8 бит управления периодом пакета 	0 - 255
13 	        Выбор формы волнового пакета 	0 - 15
14, 15 	    Регистры портов ввода/вывода 	0 - 255

R7

  7 	  6 	  5 	  4 	  3 	  2 	  1 	  0
порт В	порт А	шум С	шум В	шум А	тон С	тон В	тон А
управление
вводом/выводом	выбор канала для шума	выбор канала для тона
*/

    //копирование прошлого значения каналов для более быстрой работы


    register bool chA_bitOut=chA_bit;
    register bool chB_bitOut=chB_bit;
    register bool chC_bitOut=chC_bit;
    
    #define nR7 (~ay_R7)
    // if (nR7&0x1) {if (ay_R1_R0==0||((main_ay_count%ay_R1_R0)==0)) {chA_bit^=1;}} else chA_bitOut=1;
    // if (nR7&0x2) {if (ay_R3_R2==0||((main_ay_count%ay_R3_R2)==0)) {chB_bit^=1;}} else chB_bitOut=1;
    // if (nR7&0x4) {if (ay_R5_R4==0||((main_ay_count%ay_R5_R4)==0)) {chC_bit^=1;}} else chC_bitOut=1;

    // if (is_ay_chA_en) {if (ay_R1_R0==0||(main_ay_count%ay_R1_R0==0)) {chA_bit^=1;}} else chA_bitOut=1;
    // if (is_ay_chB_en) {if (ay_R3_R2==0||(main_ay_count%ay_R3_R2==0)) {chB_bit^=1;}} else chB_bitOut=1;
    // if (is_ay_chC_en) {if (ay_R5_R4==0||(main_ay_count%ay_R5_R4==0)) {chC_bit^=1;}} else chC_bitOut=1;

    //личные счетчики каналов
      //отдельные счётчики - без операции деления
    
    
    
    //
    // if (nR7&0x1) {chA_count+=delta;if (chA_count>=ay_R1_R0 && (ay_R1_R0>=delta) ) {chA_bit^=1;chA_count-=ay_R1_R0;}} else {chA_bitOut=1;chA_count=0;};
    // if (nR7&0x2) {chB_count+=delta;if (chB_count>=ay_R3_R2 && (ay_R3_R2>=delta) ) {chB_bit^=1;chB_count-=ay_R3_R2;}} else {chB_bitOut=1;chB_count=0;};
    // if (nR7&0x4) {chC_count+=delta;if (chC_count>=ay_R5_R4 && (ay_R5_R4>=delta) ) {chC_bit^=1;chC_count-=ay_R5_R4;}} else {chC_bitOut=1;chC_count=0;};


/*
    Если установлен "ТОН" в канале X, то 
            {
                увеличиваем chX_count на 5(delta);
                Если ( (chX_count >= значений в регистрах R1_R0(делитель частоты)) И (R1_R0(делитель частоты >=5(delta))
                    {

                    }
            }
*/

    //nR7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл

    if (nR7&0x1) {chA_count+=delta;if (chA_count>=ay_R1_R0 && (ay_R1_R0>=delta) ) {chA_bit^=1;chA_count=0;}} else {chA_bitOut=1;chA_count=0;}; /*Тон A*/
    if (nR7&0x2) {chB_count+=delta;if (chB_count>=ay_R3_R2 && (ay_R3_R2>=delta) ) {chB_bit^=1;chB_count=0;}} else {chB_bitOut=1;chB_count=0;}; /*Тон B*/
    if (nR7&0x4) {chC_count+=delta;if (chC_count>=ay_R5_R4 && (ay_R5_R4>=delta) ) {chC_bit^=1;chC_count=0;}} else {chC_bitOut=1;chC_count=0;}; /*Тон C*/


    //проверка запрещения тона в каналах
    if (ay_R7&0x1) chA_bitOut=1; 
    if (ay_R7&0x2) chB_bitOut=1;
    if (ay_R7&0x4) chC_bitOut=1;

   
    

   

    //добавление шума, если разрешён шумовой канал

    if (nR7&0x38)//есть шум хоть в одном канале
        {
          
            noise_ay_count+=delta;
            if (noise_ay_count>=(ay_R6<<1)) {noise_bit=get_random();noise_ay_count=0;}//отдельный счётчик для шумового
                                // R6 - частота шума
            
            if(!noise_bit)//если бит шума ==1, то он не меняет состояние каналов
                {            
                    if ((chA_bitOut)&&(nR7&0x08)) chA_bitOut=0;//шум в канале A
                    if ((chB_bitOut)&&(nR7&0x10)) chB_bitOut=0;//шум в канале B
                    if ((chC_bitOut)&&(nR7&0x20)) chC_bitOut=0;//шум в канале C
            
                    // if ((chA_bitOut)&&(is_ay_noise_chA_en)) chA_bitOut=0;//шум в канале A
                    // if ((chB_bitOut)&&(is_ay_noise_chB_en)) chB_bitOut=0;//шум в канале B
                    // if ((chC_bitOut)&&(is_ay_noise_chC_en)) chC_bitOut=0;//шум в канале C
                };
           
        }

    //вычисление амплитуды огибающей

    main_ay_count_env+=delta;

    static uint8_t ampl_ENV=0;

    static bool is_env_inv_enum=true; //инверсия последовательности огибающей



   // static uint32_t env_main_count=0;



    static uint32_t envelope_ay_count=0;
    #define env_count_16 (envelope_ay_count&0x0f)

    if (is_envelope_begin) {envelope_ay_count=0;main_ay_count_env=0;is_envelope_begin=false;is_env_inv_enum=true;};

   // uint32_t ay_R12_R11_sh=ay_R12_R11<<1;

    //if (!(main_ay_count&1)&&(ay_R12_R11==0||((main_ay_count>>1)%ay_R12_R11==0)))
    //if ((env_main_count++>>1)>=ay_R12_R11)
    if (((main_ay_count_env)>=(ay_R12_R11<<1)))//без операции деления
            {
                 
                 envelope_ay_count++;
                 main_ay_count_env-=ay_R12_R11<<1;
                 if (env_count_16==0) is_env_inv_enum=!is_env_inv_enum;
                 //main_ay_count=0;
                 //envelope_ay_count&=15;//ограничитель
                 //env_main_count=0;

                 switch (ay_R13)
                    //switch (ay_R13)  
                    {
                    case (0b0000):
                    case (0b0001):
                    case (0b0010):
                    case (0b0011):
                    case (0b1001):
                        if (envelope_ay_count<16) ampl_ENV=ampls[15-env_count_16]; else {ampl_ENV=ampls[0];};
                        break;
                    case (0b0100):
                    case (0b0101):
                    case (0b0110):
                    case (0b0111):
                    case (0b1111):
                        if (envelope_ay_count<16) ampl_ENV=ampls[env_count_16]; else {ampl_ENV=ampls[0];}
                        break;
                    case (0b1000):
                        ampl_ENV=ampls[15-env_count_16]; 
                        break;
                    case (0b1100):
                        ampl_ENV=ampls[env_count_16]; 
                        break;
                    case (0b1010):
                        if (is_env_inv_enum) ampl_ENV=ampls[15-env_count_16]; else ampl_ENV=ampls[env_count_16];
                        break;
                    case (0b1110):
                        if (!is_env_inv_enum) ampl_ENV=ampls[15-env_count_16]; else ampl_ENV=ampls[env_count_16];
                        break;
                    case (0b1011):
                        if (envelope_ay_count<16) ampl_ENV=ampls[15-env_count_16]; else {ampl_ENV=ampls[15];}
                        break;
                    case (0b1101):
                        if (envelope_ay_count<16) ampl_ENV=ampls[env_count_16]; else {ampl_ENV=ampls[15];}
                        break;
                    
                    
                    
                    default:
                        break;
                    }
            }
   



    static uint8_t outs[3];
   
    #define outA (outs[0])
    #define outB (outs[1])
    #define outC (outs[2])

    outA=chA_bitOut?(( ay_R8&0xf0)?ampl_ENV:ampls0[ay_R8]):0;
    outB=chB_bitOut?(( ay_R9&0xf0)?ampl_ENV:ampls0[ay_R9]):0;
    outC=chC_bitOut?((ay_R10&0xf0)?ampl_ENV:ampls0[ay_R10]):0;



    return outs;
    

};

/*
Структура PSG - формата
Offset Number of byte Description
+ 0 3 Identifier 'PSG'
+ 3 1 Marker “End of Text”(1Ah)
+ 4 1 Version number
+ 5 1 Player frequency(for versions 10 + )
+ 6 10 Data

Data — последовательности пар байтов записи в регистр.
Первый байт — номер регистра(от 0 до 0x0F), второй — значение.
Вместо номера регистра могут быть специальные маркеры : 0xFF, 0xFE или 0xFD
0xFD — конец композиции.
0xFF — ожидание 20 мс.
0xFE — следующий байт показывает сколько раз выждать по 80 мс.
*/
void AY_write_address(uint16_t word) {
//    DBGM_PRINT(("AY_write_address(%04Xh) ~word = %04X", word, ~word));
    uint8_t addr = (~word) & 0xff;
    if (addr >= 0xFD && addr <= 0xFE) return;
    AY_select_reg(addr & 0x0F);
}

void beep(bool v) {
    #ifdef HWAY
        AY_to595Beep(v);
    #else
        pwm_set_gpio_level(BEEPER_PIN, v);
    #endif
}
