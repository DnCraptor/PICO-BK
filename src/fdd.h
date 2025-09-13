#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

char* fdd0_rom();
size_t fdd0_sz();
char* fdd1_rom();
size_t fdd1_sz();

typedef struct __attribute__ ((__packed__)) TABLE_EMFDD {
			uint16_t    CSRW;               // 00 копия по записи регистра состояния КНГМД
			uint16_t    CURTRK;             // 02 адрес текущей дорожки (адрес одного из последующих байтов)
			uint8_t     TRKTAB[4];          // 04 таблица текущих дорожек
			uint16_t    TDOWN;              // 10 задержка опускания головки
			uint16_t    TSTEP;              // 12 задержка перехода с дорожки на дорожку
			uint8_t     TRKCOR;             // 14 дорожка начала предкомпенсации
			uint8_t     BRETRY;             // 15 число попыток повтора при ошибке
			uint8_t     FLAGS;              // 16 рабочая ячейка драйвера
			uint8_t     FILLB;              // 17 код заполнения при форматировании
			uint16_t    FLGPTR;             // 20 указатель на байт признаков (один из следующих байтов)
			uint8_t     FLGTAB[4];          // 22 байты признаков
			uint16_t    ADDR;               // 26 адрес начала массива данных в ОЗУ (обязательно чётный)
			int16_t     WCNT;               // 30 количество слов для пересылки
			uint8_t     SIDE;               // 32 номер стороны диска
			uint8_t     TRK;                // 33 номер дорожки
			uint8_t     UNIT;               // 34 номер привода
			uint8_t     SECTOR;             // 35 номер сектора
			uint16_t    WRTVAR;             // 36 значение, записываемое при форматировании
			uint16_t    MARKER;             // 40 буфер маркера при записи
			uint16_t    FREE;               // 42 длина пустого остатка сектора
			uint16_t    INTIME;             // 44 счётчик длительности индекса
			uint16_t    BUF4;               // 46 буфер для сохранения вектора 4
			uint16_t    BUFSP;              // 50 буфер для сохранения SP
			uint16_t    BUFPSW;             // 52 буфер для сохранения PSW
			uint8_t     CRETRY;             // 54 счётчик повторов при ошибке
			uint8_t     TURNS;              // 55 число оборотов диска при поиске сектора
			uint8_t     SECRET;             // 56 число повторных попыток поиска сектора
			uint8_t     ERRNUM;             // 57 буфер для номера ошибки
			uint16_t    MAXSEC;             // 60 число секторов на дорожке
			uint16_t    HOLTIN;             // 62 время задержки после индекса до первого сектора
			uint16_t    SECLEN;             // 64 длина сектора в словах (следить за значением при записи, иначе портится разметка диска)
} TABLE_EMFDD;

/*формат регистра PSW: PPPTNZVC
PPP - приоритет
если приоритет установленный на данный момент для ЦП ниже, чем установленный приоритет
ЗАПРОСА НА ПРЕРЫВАНИЕ, то ЦП прервёт работу и обработает прерывание.
Если же приоритет текущей задачи равен или выше приоритета запроса, то ЦП сперва закончит вычисления,
а затем обработает прерывание
У приоритета реально используется только бит 7. остальные биты ни на что не влияют.
T - флаг трассировки
N - отрицательность
Z - ноль
V - арифметический перенос, т.е. при операции со знаковыми числами, происходит перенос в знаковый разряд,
который самый старший
C - перенос, который происходит за пределы разрядности операндов
*/
// PSW bits
enum PSW_BIT {
	C_FLAG = 0,
	V_FLAG = 1,
	Z_FLAG = 2,
	N_FLAG = 3,
	T_FLAG = 4,
	P5_FLAG = 5,
	P6_FLAG = 6,
	P_FLAG = 7,
	MASKI_FLAG = 10, // 10й бит, маскирует IRQ1, IRQ2, IRQ3, VIRQ
	HALT_FLAG = 11   // 11й бит - маскирует только IRQ1
};

enum {
			FDD_NOERROR = 0,
			FDD_DISK_PROTECTED,         // 1
			FDD_SECT_HEADER_ERROR,      // 2
			FDD_0_TRACK_ERROR,          // 3
			FDD_POSITIONING_ERROR,      // 4
			FDD_SECTOR_NOT_FOUND,       // 5
			FDD_NO_DISK,                // 6
			FDD_STOP,                   // 7
			FDD_ADDR_MARKER_NOT_FOUND,  // 010
			FDD_DATA_MARKER_NOT_FOUND,  // 011
			FDD_BAD_FORMAT              // 012
};

void EmulateFDD();
void SetCommand(uint16_t cmd);
uint16_t GetState();
uint16_t GetData();
void WriteData(uint16_t data);
bool IsEngineOn();
void Periodic();

size_t size_of_drive(uint8_t drive);
uint16_t word_of_drive(uint8_t drive, size_t pos);
bool word_to_drive(uint8_t drive, size_t pos, uint16_t word);
char* fdd0_rom();
char* fdd1_rom();
size_t fdd0_sz();
size_t fdd1_sz();
