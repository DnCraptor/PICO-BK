#pragma once
#include "BKFloppyImage_Prototype.h"
#include <memory>

// !!! Не работает, только начата работа.
#pragma pack(push)
#pragma pack(1)
struct MSDOSFileRecord
{
	struct FileName
	{
		uint8_t name[8];
		uint8_t ext[3];
	};
	union
	{
		FileName file;
		uint8_t filename[11];
	};

	uint8_t     attr;               // Байт по адресу 0x0B, отвечающий за атрибуты файла.
	uint8_t     ntres;              // Байт по адресу 0x0C, используется в Windows NT.
	uint8_t     create_time_tenth;  // Байт по адресу 0x0D. Счётчик десятков миллисекунд времени создания файла, допустимы значения 0-199. Поле часто неоправданно игнорируется.
	uint16_t    create_time;        // 2 байта по адресу 0x0E. Время создания файла с точностью до 2 секунд.
	uint16_t    create_date;        // 2 байта по адресу 0x10. Дата создания файла.
	uint16_t    last_access_date;   // 2 байта по адресу 0x12. Дата последнего доступа к файлу (то есть последнего чтения или записи — в последнем случае приравнивается DIR_WrtDate). Аналогичное поле для времени не предусмотрено.
	uint16_t    first_cluster_hi;   // 2 байта по адресу 0x14. Номер первого кластера файла (старшее слово, на томе FAT12/FAT16 равен нулю).
	uint16_t    wrt_time;           // 2 байта по адресу 0x16. Время последней записи (модификации) файла, например, его создания.
	uint16_t    wrt_date;           // 2 байта по адресу 0x18. Дата последней записи (модификации) файла, в том числе создания.
	uint16_t    first_cluster_lo;   // 2 байта по адресу 0x1A. Номер первого кластера файла (младшее слово).
	uint32_t    length;             // DWORD, содержащий значение размера файла в байтах. Фундаментальное ограничение FAT32 — максимально допустимое значение размера файла составляет 0xFFFFFFFF (то есть 4 Гб минус 1 байт).
	MSDOSFileRecord()
	{
		memset(this, 0, sizeof(MSDOSFileRecord));
	}
	MSDOSFileRecord &operator = (const MSDOSFileRecord &src)
	{
		memcpy(this, &src, sizeof(MSDOSFileRecord));
		return *this;
	}
	MSDOSFileRecord &operator = (const MSDOSFileRecord *src)
	{
		memcpy(this, src, sizeof(MSDOSFileRecord));
		return *this;
	}
};
#pragma pack(pop)

class CBKFloppyImage_MSDOS :
	public CBKFloppyImage_Prototype
{
		unsigned int        m_nClusterSectors;  // Количество секторов в кластере
		unsigned int        m_nClusterSize;     // размер кластера в байтах
		unsigned int        m_nBootSectors;     // Число секторов в загрузчике
		unsigned int        m_nRootFilesNum;    // Максимальное число файлов в корневом каталоге
		unsigned int        m_nRootSize;        // размер корневого каталога в байтах
		unsigned int        m_nFatSectors;      // Число секторов в одной фат
		unsigned int        m_nFatSize;         // размер одной фат в байтах
		std::unique_ptr<uint8_t[]> m_pFatTbl;
		// текущий каталог
		unsigned int        m_nCatCluster;      // номер кластера в буфере каталога (для подкаталогов, для корня - 0)
		unsigned int        m_nCatRootBlock;    // номер сектора в буфере для корневого каталога
		std::unique_ptr<uint8_t[]> m_pCatBuffer;       // буфер кластера каталога
		MSDOSFileRecord *m_pDiskCat;
		// вспомогательные переменные
		unsigned int        m_nRootSectorOffset;    // начало корневого каталога
		unsigned int        m_nDataSectorOffset;    // начало области данных
		// переменные для поиска записей в текущем каталоге
		unsigned int        m_nFindCatCluster;      // номер кластера в буфере каталога (для подкаталогов, для корня - 0)
		unsigned int        m_nFindCatRootBlock;    // номер сектора в буфере для корневого каталога
		std::unique_ptr<uint8_t[]> m_pFindCatBuffer;       // буфер кластера каталога
		unsigned int        m_nFindRecNum;          // счётчик записей в буфере

		int GetNextFat(int fat);
		int FindFreeFat(int fat);
		uint16_t SetFat(int fat, uint16_t val);

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord(MSDOSFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int FindRecord2(MSDOSFileRecord *pRec, bool bFull = true);

		bool SeekToCluster(int nCluster);

		/*
		выход:  не nullptr - очередная запись каталога
		        nullptr - каталог физически закончился или ошибка.
		*/
		MSDOSFileRecord *GetFirstRecord();
		MSDOSFileRecord *GetNextRecord();

	protected:
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
		virtual void OnReopenFloppyImage() override;

	public:
		CBKFloppyImage_MSDOS(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_MSDOS() override;

		// виртуальные функции

		virtual const std::string GetImageInfo() const override;
		virtual const size_t GetImageFreeSpace() const override;

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

		virtual bool GetStartFileName(BKDirDataItem *pFR) override;

		virtual bool RenameRecord(BKDirDataItem *pFR) override;
};

