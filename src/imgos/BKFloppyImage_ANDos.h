#pragma once
#include "BKFloppyImage_Prototype.h"

#define IDS_LC_SPECIFIC_ANDOS_HDR       722

#pragma pack(push)
#pragma pack(1)
struct AndosFileRecord
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

	uint8_t     attr;
	uint8_t     reserved[8];
	uint8_t     dir_num;
	uint8_t     parent_dir_num;
	uint16_t    address;
	uint16_t    date;
	uint16_t    first_cluster;
	uint32_t    length;
	AndosFileRecord()
	{
		memset(this, 0, sizeof(AndosFileRecord));
	}
	AndosFileRecord &operator = (const AndosFileRecord &src)
	{
		memcpy(this, &src, sizeof(AndosFileRecord));
		return *this;
	}
	AndosFileRecord &operator = (const AndosFileRecord *src)
	{
		memcpy(this, src, sizeof(AndosFileRecord));
		return *this;
	}
};
#pragma pack(pop)

class CBKFloppyImage_ANDos :
	public CBKFloppyImage_Prototype
{
		unsigned int        m_nClusterSectors;      // Количество секторов в кластере
		unsigned int        m_nClusterSize;         // размер кластера в байтах
		unsigned int        m_nBootSectors;         // Число секторов в загрузчике
		unsigned int        m_nRootFilesNum;        // Максимальное число файлов в корневом каталоге
		unsigned int        m_nRootSize;            // размер корневого каталога в байтах
		unsigned int        m_nFatSectors;          // Число секторов в одной фат
		unsigned int        m_nFatSize;             // размер одной фат в байтах
		// вспомогательные переменные
		unsigned int        m_nRootSectorOffset;    // начало корневого каталога
		unsigned int        m_nDataSectorOffset;    // начало области данных

		std::unique_ptr<uint8_t[]> m_pFatTbl;
		std::unique_ptr<uint8_t[]> m_pCatTbl;
		AndosFileRecord *m_pDiskCat;

		int                 GetNextFat(int fat);
		int                 FindFreeFat(int fat);
		uint16_t            SetFat(int fat, uint16_t val);

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int                 FindRecord(AndosFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int                 FindRecord2(AndosFileRecord *pRec, bool bFull = true);

		bool                SeekToCluster(int nCluster);
#ifdef _DEBUG
		void                DebugOutCatalog(AndosFileRecord *pMKRec);
#endif
	protected:
		virtual void        ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void        ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
		virtual void        OnReopenFloppyImage() override;

	public:
		CBKFloppyImage_ANDos(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_ANDos() override;

		// виртуальные функции

		virtual unsigned int HasSpecificData() const override
		{
			return IDS_LC_SPECIFIC_ANDOS_HDR;
		}
		virtual const std::string GetSpecificData(BKDirDataItem *fr) const override;

		virtual const std::string GetImageInfo() const override;
		virtual const size_t GetImageFreeSpace() const override;

		/* прочитать каталог образа.
		на выходе: заполненная структура m_sDiskCat */
		virtual bool        ReadCurrentDir() override;
		/* записать каталог образа */
		virtual bool        WriteCurrentDir() override;
		/* прочитать файл из образа
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сохранена
		*       и реальная запись о файле, где сохранены все необходимые данные,
		*       даже с дублированием.
		* pBuffer - указатель на заранее выделенный массив памяти, куда будем сохранять файл.
		*   что мы будем делать потом с данными буфера, функцию не волнует. */
		virtual bool        ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
		/* записать файл в образ
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сформирована
		*       реальная запись о файле, где по возможности сохранены все необходимые данные для записи,
		*       даже с дублированием.
		* pBuffer - указатель на заранее выделенный массив памяти, где заранее подготовлен массив сохраняемого файла. */
		virtual bool        WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze) override;
		/* удалить файл в образе
		* Параметры.
		* pFR - указатель на абстрактную запись о файле, внутри которой сохранена
		*       и реальная запись о файле, где сохранены все необходимые данные,
		*       даже с дублированием. */
		virtual bool        DeleteFile(BKDirDataItem *pFR, bool bForce = false) override;
		/* создать директорию в образе */
		virtual bool        CreateDir(BKDirDataItem *pFR) override;
		/* проверить, существует ли такая директория в образе */
		virtual bool        VerifyDir(BKDirDataItem *pFR) override;
		/* удалить директорию в образе */
		virtual bool        DeleteDir(BKDirDataItem *pFR) override;

		virtual bool        GetNextFileName(BKDirDataItem *pFR) override;

		virtual bool        RenameRecord(BKDirDataItem *pFR) override;
};


/*
Распределение дискового пространства

сектор  назначение
------------------------------
0       Загрузчик
1-2     Первая копия фат
3-4     Вторая копия фат
5-11.   Каталог (7 секторов, макс.112. записей)
12.-15. 2й кластер (4 сектора), и т.д.

все параметры можно узнать из BPB в нулевом секторе.

BPB андоса

смещение длина значение         назначение
-----------------------------------                           сист. не сист.
00       2      ***             Признак системной дискеты       0240   0753
02       2      ***             Команда перехода на загрузчик   0436   020220
04       7      "ANDOS  "       OEM ID (Название ОС)
013      2      512.            Число байт в секторе
015      1      4               Количество секторов в кластере
016      2      1               Число секторов в загрузчике
020      1      2               Число фат
021      2      112.            Максимальное число файлов в корневом каталоге
023      2      ***             Общее число блоков на диске
025      1      0371            Media descriptor
026      2      2               Число секторов в одной фат
030      2      10.             Число секторов на дорожке
032      2      ***             Число головок
034      15.    0               В андосе не используются
053      11.    "           "   Метка тома
066      8.     "FAT12   "      Идентификатор файловой системы
076      2      0               В андосе не используются

Формат каталога

Длина элемента каталога 32. байта, в одном секторе 16. элементов.

смещение длина назначение
-----------------------------------
0        8      Имя файла. Первый байт имени файла имеет спец. назначение:
0 - элемент никогда не использовался,
0345 - файл был удалён, любой другой символ - первая буква
имени файла.
8        3      Расширение имени файла
11       1      Атрибуты файла (а андосе обычно не используются)
12       8      Зарезервировано
20       1      Признак подкаталога и одновременно его номер
21       1      Номер родительского подкаталога для каталога,
номер каталога, которому принадлежит файл для файла
22       2      Адрес файла/имя диска для ссылки (в мс-дос - время)
24       2      Дата создания файла
26       2      Номер начального кластера. Первый кластер начинается с номера 2.
28       2      Длина файла
30       2      Расширение длины файла (для файлов больше 64 кбайт)

в этом сраном гугле хер чо найдёшь. запишу тут, чтобы не забыть:
дата в формате МСДОС:
Биты    Предназначение
0–4     День месяца (1–31)
5–8     Месяц (1 = Январь, 2 = Февраль и так далее)
9–15    Год отсчитывается от 1980 (добавьте 1980, чтобы получить фактический год)

время в формате МСДОС:
Биты    Предназначение
0–4     Секунды делятся на 2
5–10    Минуты (0–59)
11–15   Часы (0–23 при 24-часовом формате)


Подкаталог андос - фиктивная запись с нулевым адресом, длиной и начальным
кластером, в байте атрибута у них записана константа 010, из-за чего мс-дос
считает её меткой диска

Элементы фат
0 - пустой кластер
07777 - последний кластер файла
07760-07766 - зарезервировано
07767 - плохой кластер
всё остальное - занятый кластер, значение - номер, указывает на следующий кластер

Распаковка кластера.
взять слово по (Номер кластера * 3 / 2)
если номер кластера чётный - взять младшие 12 бит, иначе - старшие 12

Преобразование номера кластера в номер сектора
Сектор = (Кластер + 1) * 4

Ссылка на другой диск - запись типа директория в поле адреса котрой буква диска.

*/
