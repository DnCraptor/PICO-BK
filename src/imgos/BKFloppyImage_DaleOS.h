#pragma once
#include "BKFloppyImage_Prototype.h"
#include <deque>
#include <memory>

#pragma pack(push)
#pragma pack(1)
struct DaleOSFileRecord
{
	uint8_t     name[16];       // имя файла, как в БК10.
	uint16_t    address;        // стартовый адрес
	uint16_t    length;         // длина в байтах
	uint16_t    start_block;    // начальный блок
	uint16_t    attr;           // признаки
	uint32_t    reserved;       // зарезервировано

	DaleOSFileRecord()
	{
		memset(this, 0, sizeof(DaleOSFileRecord));
	}
	DaleOSFileRecord &operator = (const DaleOSFileRecord &src)
	{
		memcpy(this, &src, sizeof(DaleOSFileRecord));
		return *this;
	}
	DaleOSFileRecord &operator = (const DaleOSFileRecord *src)
	{
		memcpy(this, src, sizeof(DaleOSFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto DALE_BITMAP_BLK = 1; // блок битовой карты, и некоторых других параметров.
constexpr auto DALE_CAT_BLK = 2;    // начало каталога на диске
constexpr auto DALE_CAT_SIZEBLK = 8;// размер каталога в блоках

constexpr auto DALE_CAT_SIGNATURE = 041125; // сигнатура каталога
constexpr auto DALE_CAT_SIGNATURE_OFFSET = 0776; // смещение в блоке каталога до сигнатуры каталога
constexpr auto DALE_CAT_REC_SIZE = 022; // количество записей в одном блоке каталога
constexpr auto DALE_CAT_SIZE = DALE_CAT_SIZEBLK * BLOCK_SIZE; // размер каталога в байтах
constexpr auto DALE_REC_SIZE = sizeof(DaleOSFileRecord); // размер одной записи каталога в байтах

constexpr auto DALE_BITMAP_SIGNATURE = 0x6144; // 'Da' сигнатура ОС
constexpr auto DALE_BITMAP_SIGNATURE_OFFSET = 0; // смещение в блоке битмата до сигнатуры
constexpr auto DALE_BITMAP_DATABLOCK_OFFSET = 6; // смещение в блоке битмата до номера блока, с которого начинаются данные
constexpr auto DALE_BITMAP_DISKDIMENSION_OFFSET = 010; // смещение в блоке битмапа до номера блока, где находится размер диска в блоках
constexpr auto DALE_BITMAP_DISKLABEL_OFFSET = 012; // метка диска
constexpr auto DALE_BITMAP_DISKOWNER_OFFSET = 032; // имя владельца диска
constexpr auto DALE_BITMAP_CRC_OFFSET = 054; // контрольная сумма
constexpr auto DALE_BITMAP_ARRAY_OFFSET = 056; // собственно битовая карта, размер 01000-056 байтов


class СBKFloppyImage_DaleOS :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;  // буфер каталога

		int     m_nDataBlock;   // номер блока, с которого начинаются данные на диске,
		int     m_nDiskSizeBlk; // размер диска в блоках

		std::deque<DaleOSFileRecord> m_DaleCatalog; // здесь будем хранить каталог

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		//int FindRecord(DaleOSFileRecord *pPec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		//int FindRecord2(DaleOSFileRecord *pPec, bool bFull = true);

		bool    ReadDaleCatalog();
		//bool    WriteDaleCatalog();

#ifdef _DEBUG
		void DebugOutCatalog();
#endif
	protected:
		/* преобразование абстрактной записи в конкретную. Реализация зависит от ОС */
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;

	public:
		СBKFloppyImage_DaleOS(const PARSE_RESULT &image);
		virtual ~СBKFloppyImage_DaleOS() override;
		/* прочитать каталог образа.
		 на выходе: заполненная структура m_sDiskCat */
		virtual bool ReadCurrentDir() override;
		/* записать каталог образа */
		//virtual bool WriteCurrentDir() override;
		/* прочитать файл из образа
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сохранена
		*       и реальная запись о файле, где сохранены все необходимые данные,
		*       даже с дублированием.
		* pBuffer - указатель на заранее выделенный массив памяти, куда будем сохранять файл.
		*   что мы будем делать потом с данными буфера, функцию не волнует. */
		virtual bool ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
};

/*
В общем, пока не знаю, стоит ли смысл возиться с остальным фукнционалом.
В коде ядра всё равно ошбики, доделывать ОС смысла нету,
*/
