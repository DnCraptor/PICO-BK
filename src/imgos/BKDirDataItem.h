#pragma once
#include <stdint.h>
#include <string.h>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
typedef unsigned char char_t;

enum FR_ATTR : uint32_t
{
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

constexpr auto SPECIFIC_DATA_BUFFER_LENGTH = 64; // размер буфера в байтах, где хранится оригинальная запись как есть.

class BKDirDataItem
{
	public:
		enum class RECORD_TYPE : int        // тип записи каталога
		{
			UP = 0,                         // обозначение выхода из каталога
			DIRECTORY,                      // обозначение директории
			LINK,                           // обозначение ссылки (на другой диск)
			LOGDSK,                         // обозначение логического диска
			FILE                            // обозначение файла
		};

		fs::path        strName;            // имя/название
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
		time_t          timeCreation;       // для ФС умеющих хранить дату создания файла
		// здесь будем хранить оригинальную запись о файле.
		uint32_t        nSpecificDataLength; // размер записи о файле. на БК они гарантированно меньше SPECIFIC_DATA_BUFFER_LENGTH байтов
		uint8_t         pSpecificData[SPECIFIC_DATA_BUFFER_LENGTH]; // буфер данных

		BKDirDataItem()
		{
			clear();
		}
		~BKDirDataItem() = default;
		void clear()
		{
			this->strName.clear();
			this->nAttr = 0;
			this->nStartBlock = this->nBlkSize = this->nSize = this->nAddress = this->nDirNum = this->nDirBelong = 0;
			this->nRecType = RECORD_TYPE::UP;
			this->bSelected = false;
			this->nSpecificDataLength = 0;
			this->timeCreation = 0;
			memset(this->pSpecificData, 0, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem(const BKDirDataItem *src)
		{
			this->strName = src->strName;
			this->nAttr = src->nAttr;
			this->nRecType = src->nRecType;
			this->nDirBelong = src->nDirBelong;
			this->nDirNum = src->nDirNum;
			this->nAddress = src->nAddress;
			this->nSize = src->nSize;
			this->nBlkSize = src->nBlkSize;
			this->nStartBlock = src->nStartBlock;
			this->bSelected = src->bSelected;
			this->timeCreation = src->timeCreation;
			this->nSpecificDataLength = src->nSpecificDataLength;
			memcpy(this->pSpecificData, src->pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
		}
		BKDirDataItem &operator = (const BKDirDataItem &src)
		{
			this->strName = src.strName;
			this->nAttr = src.nAttr;
			this->nRecType = src.nRecType;
			this->nDirBelong = src.nDirBelong;
			this->nDirNum = src.nDirNum;
			this->nAddress = src.nAddress;
			this->nSize = src.nSize;
			this->nBlkSize = src.nBlkSize;
			this->nStartBlock = src.nStartBlock;
			this->bSelected = src.bSelected;
			this->timeCreation = src.timeCreation;
			this->nSpecificDataLength = src.nSpecificDataLength;
			memcpy(this->pSpecificData, src.pSpecificData, SPECIFIC_DATA_BUFFER_LENGTH);
			return *this;
		}
};

