#pragma once
#include "BKFloppyImage_Prototype.h"
#include <memory>

#pragma pack(push)
#pragma pack(1)
struct MicrodosFileRecord
{
	uint8_t     stat0;      // статус записи: бит 7 - бэд блок, остальные значения - неизвестно
	uint8_t     stat1;      // статус записи: значения неизвестны
	uint8_t     name[14];   // имя файла, если начинается с 0177 - каталог. и значит это не микродос
	uint16_t    start_block;// начальный блок
	uint16_t    len_blk;    // длина в блоках, если len_blk == 0 то это тоже с большой долей вероятности каталог
	uint16_t    address;    // стартовый адрес
	uint16_t    length;     // длина, или остаток длины от длины в блоках, если размер файла > 64кб
	MicrodosFileRecord()
	{
		memset(this, 0, sizeof(MicrodosFileRecord));
	}
	MicrodosFileRecord &operator = (const MicrodosFileRecord &src)
	{
		memcpy(this, &src, sizeof(MicrodosFileRecord));
		return *this;
	}
	MicrodosFileRecord &operator = (const MicrodosFileRecord *src)
	{
		memcpy(this, src, sizeof(MicrodosFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto FMT_MIKRODOS_CAT_RECORD_NUMBER = 030;
constexpr auto FMT_MIKRODOS_TOTAL_FILES_USED_BLOCKS = 032;
constexpr auto FMT_MIKRODOS_DISK_SIZE = 0466;
constexpr auto FMT_MIKRODOS_CAT_BEGIN = 0500;

class CBKFloppyImage_MicroDos :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;      // буфер каталога
		MicrodosFileRecord *m_pDiskCat;
		unsigned int    m_nCatSize;         // размер буфера каталога, выровнено по секторам, включая служебную область
		unsigned int    m_nMKCatSize;       // размер каталога в записях
		unsigned int    m_nMKLastCatRecord; // индекс последней записи в каталоге, чтобы определять конец каталога

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord(MicrodosFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord2(MicrodosFileRecord *pRec, bool bFull = true);

		bool Squeeze();
#ifdef _DEBUG
		void DebugOutCatalog(MicrodosFileRecord *pRec);
#endif
	protected:
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;

	public:
		CBKFloppyImage_MicroDos(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_MicroDos() override;

		// виртуальные функции

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

		virtual bool GetNextFileName(BKDirDataItem *pFR) override;

		virtual bool RenameRecord(BKDirDataItem *pFR) override;
};

/*
Поскольку формат микродос остаётся тайной-тайн, будем делать так.
первые 2 байта stat0 и stat1 будем просто игнорировать, кроме бита 7 первого байта.
бит 7 stat0 - бэдблок, бит 7 stat1 - удалённый, кажется так
каталогов нет. в ячейке 030 - общее кол-во записей.
*/