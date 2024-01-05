// MSF.h: defines for the CMSFManager class.
//


#pragma once

enum class MSF_CONF : int
{
	BK10 = 0,
	BK1001,
	BK1001_MSTD,
	BK1001_EXT32,
	BK1001_FDD,
	BK11M,
	BK11M_FDD,
	BK11,
	BK11_FDD
};

enum MSF_BLOCKTYPE : DWORD
{
	MSF_BLOCKTYPE_UNKNOWN = 0xffffffff,
	MSF_BLOCKTYPE_BASEMEMORY = 0,
	MSF_BLOCKTYPE_CPU_REGISTERS,
	MSF_BLOCKTYPE_PREVIEW,
	MSF_BLOCKTYPE_10EXT16,
	MSF_BLOCKTYPE_10EXT32,
	MSF_BLOCKTYPE_11EXT64,
	MSF_BLOCKTYPE_PORT_REGS,
	MSF_BLOCKTYPE_MEMMAP,
	MSF_BLOCKTYPE_BASEMEMORY11M,
	MSF_BLOCKTYPE_MEMORY_SMK512,
	MSF_BLOCKTYPE_CONFIG,
	MSF_BLOCKTYPE_FRAMEDATA,
	MSF_BLOCKTYPE_WAVE = 200
};

#pragma pack(push)
#pragma pack(4)
struct MSF_FILE_HEADER
{
	DWORD   type;
	DWORD   version;
	DWORD   configuration;
};

struct MSF_BLOCK_HEADER
{
	DWORD   type;
	DWORD   length;
};

struct MSF_CPU_REGISTERS
{
	uint16_t    r0;
	uint16_t    r1;
	uint16_t    r2;
	uint16_t    r3;
	uint16_t    r4;
	uint16_t    r5;
	uint16_t    sp;
	uint16_t    pc;
	uint16_t    psw;
};

struct MSF_PORT_REGS
{
	uint16_t    p0177660;
	uint16_t    p0177662_in;
	uint16_t    p0177662_out;
	uint16_t    p0177664;
	uint16_t    p0177700;
	uint16_t    p0177702;
	uint16_t    p0177704;
	uint16_t    p0177706;
	uint16_t    p0177710;
	uint16_t    p0177712;
	uint16_t    p0177714_in;
	uint16_t    p0177714_out;
	uint16_t    p0177716_in;
	uint16_t    p0177716_out_tap;
	uint16_t    p0177716_out_mem;
};

struct MSF_FRAMEDATA
{
	// состояние ВЕ таймера
	int     nTimerSpeed;
	int     nTimerDiv;

	// состояние экрана
	int     nVideoAddress; // видео адрес, младшие 6 бит - счётчик строк внутри строки
	int     bHgate; // флаг отсчёта служебных видеоциклов в строке
	int     bVgate; // флаг отсчёта служебных строк
	int     nVGateCounter; // дополнительный счётчик служебных строк
	int     nLineCounter; // счётчик видео строк

	// состояние фрейма
	int     nCPUTicks;
	double  fMediaTicks;
	double  fMemoryTicks;
	double  fFDDTicks;
};

struct MSF_BLOCK_INFO
{
	DWORD               offset;
	MSF_BLOCK_HEADER    header;
};

#pragma pack(pop)
