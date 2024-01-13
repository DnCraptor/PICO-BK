#pragma once
#include "BKFloppyImage_Prototype.h"

#pragma pack(push)
#pragma pack(1)
struct OptokFileRecord
{
	uint8_t     name[16];   // имя файла, как в БК10.
	uint16_t    status;     // статус записи: 0 - файл стерт, 177777 - файл существует
	uint16_t    masks;		// маска 1 - 1 - файл защищен от удаления
							// маска 2 - 1 - файл содержит BAD-блоки
							// маска 4 - 1 - файл запускается с отказом
							//               от дисковода
							//      иначе - нулевые биты
	uint16_t    WAAR;		// адрес переноса дисковой области при запуске
	uint16_t    unused;		// не задействовано
	uint16_t    address;    // адрес загрузки файла
	uint16_t    length;     // длина файла в байтах
	uint16_t    start_block;// номер первого сектора файла (начальный блок)
	uint16_t    crc;        // КС

	OptokFileRecord()
	{
		memset(this, 0, sizeof(OptokFileRecord));
	}
	OptokFileRecord &operator = (const OptokFileRecord &src)
	{
		memcpy(this, &src, sizeof(OptokFileRecord));
		return *this;
	}
	OptokFileRecord &operator = (const OptokFileRecord *src)
	{
		memcpy(this, src, sizeof(OptokFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto FMT_OPTOK_FIRST_FILE_BLOCK = 034;
constexpr auto FMT_OPTOK_DISK_SIZE = 036;

constexpr auto OPTOK_CAT_SIZE = BLOCK_SIZE * 7; // размер каталога в байтах
constexpr auto OPTOK_REC_SIZE = sizeof(OptokFileRecord); // размер записи о файле в байтах


class CBKFloppyImage_Optok :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;
		OptokFileRecord *m_pDiskCat;
		int     m_nRecsNum;     // количество записей в каталоге
		int     m_nFreeBlock;   // номер блока, с которого начинается свободное место
	protected:

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord(OptokFileRecord *pPec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord2(OptokFileRecord *pPec, bool bFull = true);

		bool Squeeze();

#ifdef _DEBUG
		void DebugOutCatalog(OptokFileRecord *pRec);
#endif
		/* преобразование абстрактной записи в конкретную. Реализация зависит от ОС */
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
	public:
		CBKFloppyImage_Optok(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_Optok() override;

		// виртуальные функции

		/* прочитать каталог образа */
		virtual bool ReadCurrentDir() override;
		/* записать каталог образа */
		virtual bool WriteCurrentDir() override;
		/* прочитать файл из образа */
		virtual bool ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;

		/* записать файл в образ */
		virtual bool WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze) override;

		/* удалить файл в образе */
		virtual bool DeleteFile(BKDirDataItem *pFR, bool bForce = false) override;

		virtual bool RenameRecord(BKDirDataItem *pFR) override;
};

/*
Формат файловой системы:
блоки 0, 1 - не используются
блоки 2..8 - каталог
первая запись каталога - служебная

формат служебной записи
смещ.   размер  назначение
000     002     "**" - сигнатура
002     032     не используется, все нули
034     002     номер блока, с которого начинаются данные
036     002     количество свободных блоков на диске
                (значение уменьшается по мере добавления файлов в каталог)

формат записи о файле.
смещ.   размер  назначение
000     020     имя файла
020     002     признак действительной записи
030     002     адрес загрузки
032     002     размер файла в байтах
034     002     начальный блок
036     002     контрольная сумма?

если запись удалена, то имя - все нули, и признак записи по смещению 020 тоже 0.

Interleave1:    1,6,2,7,3,8,4,9,5
Interleave2:    1,4,7,2,5,8,3,6,9

*/
