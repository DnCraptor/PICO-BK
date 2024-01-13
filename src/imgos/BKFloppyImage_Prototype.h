#pragma once

//#include "resource.h"
#include "BKDEDefines.h"
#include "ImgUtil.h"
#include "BKParseImage.h"

class CBKFloppyImage_Prototype
{
	public:
		const char_t   *m_pKoi8tbl;         // используемая в заданной ОС таблица КОИ-8

	protected:
		bool            m_bMakeAdd;         // флаг, реализованы ли функции добавления
		bool            m_bMakeDel;         // флаг, реализованы ли функции удаления
		bool            m_bMakeRename;      // флаг, реализованы ли функции переименования
		bool            m_bChangeAddr;      // флаг, разрешено ли менять адрес записей
		bool            m_bFileROMode;      // режим открытия образа, только для чтения == true. или для чтения/записи == false

		unsigned int    m_nBlockSize;       // Размер блока в байтах, обычно совпадает с сектором

		IMAGE_ERROR     m_nLastErrorNumber;
		CBKImgFile      m_pFoppyImgFile;    // файл образа

		PARSE_RESULT    m_sParseImgResult;  // структура с параметрами

		DiskCatalog     m_sDiskCat;         // структура, куда собрано всё, что касается каталога

		uint8_t         m_nSector[BLOCK_SIZE];
		uint8_t         m_mBlock[COPY_BLOCK_SIZE];
		unsigned int    m_nSeekOffset;

		// переменные для GetStartFileName/GetNextFileName
		std::vector<int> m_vecPC;           // список позиций для поиска в каталоге, для того, чтобы рекурсивные алгоритмы обхода каталога работали
		unsigned int    m_nCatPos;          // глобальная позиция для поиска в каталоге

		void            MakeFakeOutDirRecord();

		/* Добавление номера существующей директории к списку с проверкой на дублирование
		 * Выход: true - всё в порядке
		 *      false - такой номер уже встречался*/
		bool            AppendDirNum(uint8_t nNum);

		/* назначить новый номер директории.
		 * 0 - номеров не осталось, все заняты*/
		uint8_t         AssignNewDirNum();

		/* Перемещение к заданному блоку образа */
		bool            SeekToBlock(size_t nBlockNumber);
		/* чтение данных
		/* Внимание!!! эта функция читает данные всегда поблочно!!!
		/* Поэтому выделенная память ВСЕГДА должна быть выровнена по границе блока!!!
		/* Это из-за того, что читалка совмещена с чтением с реального FDD, а там чтение
		/* только посекторно. там нет возможости читать часть сектора.
		*/
		bool            ReadData(void *ptr, size_t length);
		/* Перемещение к заданному блоку образа и чтение данных */
		bool            SeektoBlkReadData(size_t nBlockNumber, void *ptr, size_t length);
		/* запись данных */
		bool            WriteData(void *ptr, size_t length);
		/* Перемещение к заданному блоку образа и запись данных */
		bool            SeektoBlkWriteData(size_t nBlockNumber, void *ptr, size_t length);

		/* преобразование абстрактной записи в конкретную. Реализация зависит от ОС */
		virtual void    ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) {}
		virtual void    ConvertRealToAbstractRecord(BKDirDataItem *pFR) {}
		virtual void    OnReopenFloppyImage() {}

	public:
		CBKFloppyImage_Prototype(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_Prototype();

		/* открыть образ */
		bool            OpenFloppyImage();
		/* закрыть образ */
		void            CloseFloppyImage();

		inline std::vector<BKDirDataItem> *CurrDirectory()
		{
			return & m_sDiskCat.vecFC;
		}
		inline const int GetCurrDirNum() // специальный костыль. чтобы узнавать о выходе из лог диска
		{
			return m_sDiskCat.nCurrDirNum;
		}
		inline const fs::path &GetCurrImgName()
		{
			return m_sParseImgResult.strName;
		}
		inline bool     GetImageOpenStatus()
		{
			return m_bFileROMode;
		}
		inline unsigned long GetBaseOffset()
		{
			return m_sParseImgResult.nBaseOffset;
		}
		inline unsigned long GetImgSize()
		{
			return m_sParseImgResult.nImageSize;
		}
		inline IMAGE_TYPE GetImgOSType()
		{
			return m_sParseImgResult.imageOSType;
		}
		inline bool isImageBootable()
		{
			return m_sParseImgResult.bImageBootable;
		}
		inline IMAGE_ERROR GetErrorNumber()
		{
			return m_nLastErrorNumber;
		}

		uint32_t        EnableGUIControls() const;

		// размер проги выровняем по границе сектора
		size_t          EvenSizeByBlock(size_t length)
		{
			return length ? (((length - 1) | static_cast<size_t>(BLOCK_SIZE - 1)) + 1) : 0;
		}

		// размер проги в размерах блока
		size_t          ByteSizeToBlockSize(size_t length)
		{
			return EvenSizeByBlock(length) / BLOCK_SIZE;
		}

		// размер проги выровняем по границе блока, сперцифичного для заданной ОС
		int             EvenSizeByBlock_l(int length)
		{
			return length ? (((length - 1) | (m_nBlockSize - 1)) + 1) : 0;
		}

		// размер проги в размерах блока, сперцифичного для заданной ОС
		int             ByteSizeToBlockSize_l(int length)
		{
			return EvenSizeByBlock_l(length) / m_nBlockSize;
		}

		// виртуальные функции

		// получение данных специфических для заданной ОС
		// возвращает 0, если спец данных нету, или ID строки в файле ресурсов, если есть. Предполагается, что ID не может быть 0
		virtual unsigned int HasSpecificData() const
		{
			return 0;
		}
		virtual const std::string GetSpecificData(BKDirDataItem *fr) const
		{
			return "";
		}

		virtual const std::string GetImageInfo() const;

		virtual const size_t GetImageFreeSpace() const;


		/* прочитать каталог образа */
		virtual bool    ReadCurrentDir();
		/* записать каталог образа */
		virtual bool    WriteCurrentDir();
		/* прочитать файл из образа */
		virtual bool    ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
		{
			return false;
		}
		// специально для ксидос вводим перехватчик,
		// который будет модифицировать имя файла
		virtual void    OnExtract(BKDirDataItem *pFR, std::string &strName)
		{
		}
		/* записать файл в образ */
		virtual bool    WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
		{
			bNeedSqueeze = false;
			return false;
		}
		/* удалить файл в образе */
		virtual bool    DeleteFile(BKDirDataItem *pFR, bool bForce = false)
		{
			return false;
		}
		/* сменить текущую директорию в образе */
		virtual bool    ChangeDir(BKDirDataItem *pFR);
		/* проверить, существует ли такая директория в образе */
		virtual bool    VerifyDir(BKDirDataItem *pFR);
		/* создать директорию в образе */
		virtual bool    CreateDir(BKDirDataItem *pFR);
		/* удалить директорию в образе */
		virtual bool    DeleteDir(BKDirDataItem *pFR);

		/*поиск и выдача всех найденных записей о файлах в текущей директории
		Вход: pFR указатель на существующий экземпляр записи
		Выход: true - найдено, при этом pFR - структура заполнена данными
		false - не найдено, pFR обнулена
		*/
		virtual bool    GetStartFileName(BKDirDataItem *pFR);
		virtual bool    GetNextFileName(BKDirDataItem *pFR);

		virtual bool    RenameRecord(BKDirDataItem *pFR)
		{
			return false;
		}
};

