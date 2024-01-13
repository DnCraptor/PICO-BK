#pragma once
#include "BKFloppyImage_Prototype.h"
#include <memory>

#define IDS_LC_SPECIFIC_HOLOGRAPHY_HDR  739

#pragma pack(push)
#pragma pack(1)
struct HolographyFileRecord
{
	uint8_t     name[16];       // имя файла, как в БК10.
	uint16_t    start_block;    // начальный блок
	uint16_t    length_blk;     // длина в блоках
	uint16_t    load_address;   // стартовый адрес
	uint16_t    start_address;  // стартовый адрес

	HolographyFileRecord()
	{
		memset(this, 0, sizeof(HolographyFileRecord));
	}
	HolographyFileRecord &operator = (const HolographyFileRecord &src)
	{
		memcpy(this, &src, sizeof(HolographyFileRecord));
		return *this;
	}
	HolographyFileRecord &operator = (const HolographyFileRecord *src)
	{
		memcpy(this, src, sizeof(HolographyFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto HOLO_CATALOG_BLK = 037; // начальный блок каталога
constexpr auto HOLO_CATALOG_LENBLK = 2; // размер каталога в блоках
constexpr auto HOLO_CATALOG_DIMENSION = 052; // количество записей, помещающихся в каталог
constexpr auto HOLO_REC_SIZE = sizeof(HolographyFileRecord); // размер записи о файле в байтах
constexpr auto HOLO_CATALOG_SIZE = HOLO_CATALOG_LENBLK * BLOCK_SIZE; // размер каталога в байтах

constexpr auto HOLO_DATA_BLK = 041; // начало данных на диске

class CBKFloppyImage_Holography :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;  // буфер каталога
		HolographyFileRecord *m_pDiskCat;
		int     m_nRecsNum;     // количество записей в каталоге
		int     m_nFreeBlock;   // номер блока, с которого начинается свободное место

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord(HolographyFileRecord *pPec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord2(HolographyFileRecord *pPec, bool bFull = true);

#ifdef _DEBUG
		void DebugOutCatalog(HolographyFileRecord *pRec);
#endif

	protected:
		/* преобразование абстрактной записи в конкретную. Реализация зависит от ОС */
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;

	public:
		CBKFloppyImage_Holography(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_Holography() override;

		// виртуальные функции

		virtual unsigned int HasSpecificData() const override
		{
			return IDS_LC_SPECIFIC_HOLOGRAPHY_HDR;
		}
		virtual const std::string GetSpecificData(BKDirDataItem *fr) const override;

		/* прочитать каталог образа */
		virtual bool ReadCurrentDir() override;
		/* записать каталог образа */
		virtual bool WriteCurrentDir() override;
		/* прочитать файл из образа */
		virtual bool ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
};

/*
Эта недо-ОС умеет очень мало.
Удалять файлы не умеет. Добавляет файлы только в конец каталога, пока каталог не переполнится и не случится
попытка записи в ПЗУ. Никаких проверок ни на что нету, можно добавлять в каталог одинаковые файлы, но поиском
будет находиться только первый из них.

Зато есть фича - адрес запуска, и он может не совпадать с адресом загрузки и может не быть блока автозапуска.
Он теряется при экспорте/импорте файлов, потому что про такую фичу в формате бин файла никто не подумал.

Поэтому, ничего, кроме экспорта из образа реализовывать не буду.

Формат файловой системы:
блок 0: загрузчик, использует EMT БОС БК11М
блоки 1..036 - ядро. Загружается в страницу 6 в окно 1.
        первые 20000 байтов - стандартный дамп ПЗУ монитора БК10,
        затем 16000 байтов - выделено под систему, из которых она занимает чуть-чуть
        с адреса 136000 находится буфер каталога.
блоки 037..040 - каталог 2 блока
начиная с блока 041 - область данных.

Формат записи в каталоге.
Размер записи 030 байтов

Смещение    Размер  Значение
0           20      Имя файла, пустые места заполнены пробелами
20          2       номер начального блока на диске
22          2       размер в блоках
                    0 - признак конца каталога
                    у конечной записи номер начального блока = номер свободного блока
24          2       адрес загрузки
26          2       адрес запуска
*/
