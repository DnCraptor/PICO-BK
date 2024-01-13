#pragma once
#include "BKFloppyImage_Prototype.h"
#include <deque>

#define IDS_LC_SPECIFIC_CSIDOS_HDR      723

#pragma pack(push)
#pragma pack(1)
struct CsidosFileRecord
{
	uint8_t     type;           // тип записи, номер директории для записи-директории
	uint8_t     protection;     // если <0 - защита от удаления
	uint8_t     name[11];       // имя файла с расширением, но без точки
	uint8_t     status;         // статус файла.
	uint16_t    start_block;    // начальный блок
	uint16_t    address;        // адрес загрузки
	uint16_t    length;         // длина файла, в байтах или блоках, зависит от статуса
	CsidosFileRecord()
	{
		memset(this, 0, sizeof(CsidosFileRecord));
	}
	CsidosFileRecord &operator = (const CsidosFileRecord &src)
	{
		memcpy(this, &src, sizeof(CsidosFileRecord));
		return *this;
	}
	CsidosFileRecord &operator = (const CsidosFileRecord *src)
	{
		memcpy(this, src, sizeof(CsidosFileRecord));
		return *this;
	}
};

struct CsidosCatHeader
{
	uint16_t block_number;  // физический номер блока
	uint16_t block_counts;  // общее число блоков на диске
	uint16_t marker1;       // 0123123 для csidos - 3
	uint16_t marker2;       // 0123123 для csidos - 3
	uint16_t marker3;       // 0123123 принадлежность диска системе csidos
	uint16_t reserved;      // не используется == 0
	CsidosCatHeader()
	{
		memset(this, 0, sizeof(CsidosCatHeader));
	}
	CsidosCatHeader &operator = (const CsidosCatHeader &src)
	{
		memcpy(this, &src, sizeof(CsidosCatHeader));
		return *this;
	}
	CsidosCatHeader &operator = (const CsidosCatHeader *src)
	{
		memcpy(this, src, sizeof(CsidosCatHeader));
		return *this;
	}
};
#pragma pack(pop)


class CBKFloppyImage_Csidos3 :
	public CBKFloppyImage_Prototype
{
		CsidosCatHeader m_FirstBlockHeader;
		std::deque<CsidosFileRecord> m_CSICatalog; // здесь будем хранить каталог

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord(CsidosFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord2(CsidosFileRecord *pRec, bool bFull = true);

		bool    ReadCSICatalog();
		bool    WriteCSICatalog();
		bool    Squeeze();
		int     CompareSize(CsidosFileRecord *pRec, unsigned int fileLen);
		unsigned int GetLength(CsidosFileRecord *pRec);

		void    SetLength(CsidosFileRecord *pRec, unsigned int fileLen);
#ifdef _DEBUG
		void DebugOutCatalog();
#endif

	protected:
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
		virtual void OnReopenFloppyImage() override;

	public:
		CBKFloppyImage_Csidos3(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_Csidos3() override;

		// виртуальные функции
		virtual unsigned int HasSpecificData() const override
		{
			return IDS_LC_SPECIFIC_CSIDOS_HDR;
		}
		virtual const std::string GetSpecificData(BKDirDataItem *fr) const override;

		/* прочитать каталог образа.
		 на выходе: заполненная структура m_sDiskCat */
		virtual bool ReadCurrentDir() override;
		/* записать каталог образа */
		virtual bool WriteCurrentDir() override;
		/* прочитать файл из образа
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сохранена
		*       и реальная запись о файле, где сохранены все необходимые данные,
		*       даже с дублированием.
		* pBuffer - указатель на заранее выделенный массив памяти, куда будем сохранять файл.
		*   что мы будем делать потом с данными буфера, функцию не волнует. */
		virtual bool ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
		virtual void OnExtract(BKDirDataItem *pFR, std::string &strName) override;
		/* записать файл в образ
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сформирована
		*       реальная запись о файле, где по возможности сохранены все необходимые данные для записи,
		*       даже с дублированием.
		* pBuffer - указатель на заранее выделенный массив памяти, где заранее подготовлен массив сохраняемого файла. */
		virtual bool WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze) override;
		/* удалить файл в образе
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сохранена
		*       и реальная запись о файле, где сохранены все необходимые данные,
		*       даже с дублированием. */
		virtual bool DeleteFile(BKDirDataItem *pFR, bool bForce = false) override;
		/* создать директорию в образе */
		virtual bool CreateDir(BKDirDataItem *pFR) override;
		/* проверить, существует ли такая директория в образе */
		virtual bool VerifyDir(BKDirDataItem *pFR) override;
		/* удалить директорию в образе */
		virtual bool DeleteDir(BKDirDataItem *pFR) override;

		virtual bool GetNextFileName(BKDirDataItem *pFR) override;

		virtual bool RenameRecord(BKDirDataItem *pFR) override;
};


/*
Расположение информации на диске

00 блок - загрузчик системы
02-011 блок - каталог
012-... блоки под файлы

Расположение записей в каталоге

первый блок
смещение длина значение
------------------------------
0       2       физический номер блока
2       2       общее число блоков на диске
4       2       0123123 для csidos-3
6       2       0123123 для csidos-3
010     2       0123123 принадлежность диска системе csidos
012     2       не используется == 0
014     0764    записи о файлах

остальные блоки
смещение длина значение
------------------------------
0       2       физический номер блока
2       012     не используется == 0
014     0764    записи о файлах


Формат записи о файле (024 байта)

смещение длина значение
------------------------------
0       1       тип:
                1-0310 номер директории, к которой принадлежит запись. 1 - корень.
                0311 плохое место - сбойный блок
                0312 запись не содержит информации (это когда удалили директорию)
                0376 дырка
                0377 удалённый файл, который можно восстановить
                0 конец каталога
                номер директории 0307 используется для сокрытия файлов.
1       1       <0 - защита от удаления
2       013     имя файла с расширением (без точки)
                8 символов на имя файла и 3 символа на расширение
                Оказывается, директории имеют только 8 символов имени,
                и опознаются по нулями в поле расширения
                Неочевидная фича - имя файла должно состоять только из строчных символов.
015     1       статус файла:
                7 бит - признак длины в блоках
                3 бит - признак файла для бк0010: 1 == БК11 0 == БК10
                остальные биты - копия регистра 0177717 (страницы)
                либо номер директории для директории
016     2       начальный блок файла, 0 для директории
020     2       адрес загрузки, 0 для директории
022     2       длина файла, 0 для директории

оставшееся свободное место в конце диска помечается дыркой.

*/