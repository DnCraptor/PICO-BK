/**
 * Partly migration from https://gid.pdp-11.ru/bkemuc.html?custom=3.13.2401.10604
*/
#pragma once
#ifndef DISK_H
#define DISK_H

extern "C" {
#include "string.h"
#include "stdint.h"
int m_add_file_ext(const char* fname, bool dir, int dir_num);
}

enum FR_ATTR : uint32_t {
	READONLY    = 0x001,
	HIDDEN      = 0x002,
	PROTECTED   = 0x004,    // везде, имеет смысл как RO, но в RT-11 есть оба атрибута, и RO, и P
	SYSTEM      = 0x008,
	ARCHIVE     = 0x010,
	DIRECTORY   = 0x020,
	VOLUMEID    = 0x040,
	TEMPORARY   = 0x080,
	LOGDISK     = 0x100,
	LINK        = 0x200,
	BAD         = 0x400,
	DELETED     = 0x800
};

enum class IMAGE_TYPE : int
{
	ERROR_NOIMAGE = -1,
	UNKNOWN = 0,
	ANDOS,
	MKDOS,
	AODOS,
	NORD,
	MIKRODOS,
	CSIDOS3,
	RT11,
	NCDOS,
	DXDOS,
	OPTOK,
	HOLOGRAPHY,
	DALE,
	MSDOS
};

struct PARSE_RESULT_C
{
	unsigned long   nImageSize;     // размер файла в байтах
	unsigned long   nBaseOffset;    // смещение до логического диска (раздела) в образе файла
	int             nPartLen;       // размер раздела в блоках (если возможно получить)
	IMAGE_TYPE      imageOSType;    // результат парсинга
	bool            bImageBootable; // флаг загрузочного образа
	char            strName[256];        // имя файла образа
};

// размер блока по умолчанию. == размер сектора
constexpr auto BLOCK_SIZE = 512;

extern "C" void detect_os_type(const char* path, char* os_type, size_t sz);
#if EXT_DRIVES_MOUNT
extern "C" bool mount_img(const char* path, int curr_dir_num);
#endif

constexpr auto SPECIFIC_DATA_BUFFER_LENGTH = 64; // размер буфера в байтах, где хранится оригинальная запись как есть.

class BKDirDataItem {
	public:
		enum class RECORD_TYPE : int        // тип записи каталога
		{
			UP = 0,                         // обозначение выхода из каталога
			DIRECTORY,                      // обозначение директории
			LINK,                           // обозначение ссылки (на другой диск)
			LOGDSK,                         // обозначение логического диска
			FILE                            // обозначение файла
		};
		char            strName[16];        // имя/название
		unsigned int    nAttr;              // атрибуты, ну там директория, защищённый, скрытый, удалённый, плохой
		RECORD_TYPE     nRecType;           // тип записи: 0="..", 1=директория, 2=ссылка, 3=логический диск, 4=простой файл, вводится специально для сортировки
		// поэтому частично дублирует атрибуты
		int             nDirBelong;         // номер каталога, к которому принадлежит файл для файла и директории
		int             nDirNum;            // номер директории для директории, 0 - для файла
		unsigned int    nAddress;           // адрес файла, для директории 0, если для директории не 0, то это ссылка на другой диск.
		unsigned int    nSize;              // размер файла
		int             nBlkSize;           // размер в блоках, или кластерах (для андос)
		unsigned int    nStartBlock;        // начальный сектор/блок/кластер
		bool            bSelected;          // вводим флаг, что запись в списке выделена.
	//	time_t          timeCreation;       // для ФС умеющих хранить дату создания файла
		// здесь будем хранить оригинальную запись о файле.
		uint32_t        nSpecificDataLength; // размер записи о файле. на БК они гарантированно меньше SPECIFIC_DATA_BUFFER_LENGTH байтов
		uint8_t         pSpecificData[SPECIFIC_DATA_BUFFER_LENGTH]; // буфер данных
		BKDirDataItem()	{
			clear();
		}
		~BKDirDataItem() = default;
		void clear() {
			memset(this->strName, 0, 16);
			this->nAttr = 0;
			this->nStartBlock = this->nBlkSize = this->nSize = this->nAddress = this->nDirNum = this->nDirBelong = 0;
			this->nRecType = RECORD_TYPE::UP;
			this->bSelected = false;
			this->nSpecificDataLength = 0;
		//	this->timeCreation = 0;
			memset(this->pSpecificData, 0, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem(const BKDirDataItem *src)	{
			strncpy(this->strName, src->strName, sizeof this->strName);
			this->nAttr = src->nAttr;
			this->nRecType = src->nRecType;
			this->nDirBelong = src->nDirBelong;
			this->nDirNum = src->nDirNum;
			this->nAddress = src->nAddress;
			this->nSize = src->nSize;
			this->nBlkSize = src->nBlkSize;
			this->nStartBlock = src->nStartBlock;
			this->bSelected = src->bSelected;
			//this->timeCreation = src->timeCreation;
			this->nSpecificDataLength = src->nSpecificDataLength;
			memcpy(this->pSpecificData, src->pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem &operator = (const BKDirDataItem &src) {
			strncpy(this->strName, src.strName, sizeof this->strName);
			this->nAttr = src.nAttr;
			this->nRecType = src.nRecType;
			this->nDirBelong = src.nDirBelong;
			this->nDirNum = src.nDirNum;
			this->nAddress = src.nAddress;
			this->nSize = src.nSize;
			this->nBlkSize = src.nBlkSize;
			this->nStartBlock = src.nStartBlock;
			this->bSelected = src.bSelected;
		//	this->timeCreation = src.timeCreation;
			this->nSpecificDataLength = src.nSpecificDataLength;
			memcpy(this->pSpecificData, src.pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
			return *this;
		}
};

#endif
