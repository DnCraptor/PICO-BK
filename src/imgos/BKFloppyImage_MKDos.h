#pragma once
#include "BKFloppyImage_Prototype.h"
#include <memory>

#pragma pack(push)
#pragma pack(1)
struct MKDosFileRecord
{
	uint8_t     status;     // статус файла, или номер директории для записи-директории
	uint8_t     dir_num;    // номер директории, которой принадлежит запись
	uint8_t     name[14];   // имя файла/каталога, если начинается с байта 0177 - это каталог.
	uint16_t    start_block;// начальный блок
	uint16_t    len_blk;    // длина в блоках
	uint16_t    address;    // стартовый адрес
	uint16_t    length;     // длина
	MKDosFileRecord()
	{
		memset(this, 0, sizeof(MKDosFileRecord));
	}
	MKDosFileRecord &operator = (const MKDosFileRecord &src)
	{
		memcpy(this, &src, sizeof(MKDosFileRecord));
		return *this;
	}
	MKDosFileRecord &operator = (const MKDosFileRecord *src)
	{
		memcpy(this, src, sizeof(MKDosFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto FMT_MKDOS_CAT_RECORD_NUMBER = 030;
constexpr auto FMT_MKDOS_TOTAL_FILES_USED_BLOCKS = 032;
constexpr auto FMT_MKDOS_DISK_SIZE = 0466;
constexpr auto FMT_MKDOS_FIRST_FILE_BLOCK = 0470;
constexpr auto FMT_MKDOS_CAT_BEGIN = 0500;
constexpr auto MKDOS_CAT_RECORD_SIZE = 173;

class CBKFloppyImage_MKDos :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;      // буфер каталога
		MKDosFileRecord *m_pDiskCat;
		unsigned int    m_nCatSize;         // размер буфера каталога, выровнено по секторам, включая служебную область
		unsigned int    m_nMKCatSize;       // размер каталога в записях
		unsigned int    m_nMKLastCatRecord; // индекс последней записи в каталоге, чтобы определять конец каталога

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord(MKDosFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord2(MKDosFileRecord *pRec, bool bFull = true);

		bool Squeeze();
		bool OptimizeCatalog();
#ifdef _DEBUG
		void DebugOutCatalog(MKDosFileRecord *pRec);
#endif
	protected:
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
		virtual void OnReopenFloppyImage() override;

	public:
		CBKFloppyImage_MKDos(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_MKDos() override;

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
		/* сменить текущую директорию в образе */
		virtual bool ChangeDir(BKDirDataItem *pFR) override;
		/* проверить, существует ли такая директория в образе */
		virtual bool VerifyDir(BKDirDataItem *pFR) override;
		/* создать директорию в образе */
		virtual bool CreateDir(BKDirDataItem *pFR) override;
		/* удалить директорию в образе */
		virtual bool DeleteDir(BKDirDataItem *pFR) override;

		virtual bool GetNextFileName(BKDirDataItem *pFR) override;

		virtual bool RenameRecord(BKDirDataItem *pFR) override;
};


/*
формат каталога мкдос

формат нулевого блока

Адрес   Значение
------------------------------------------
030     Количество файлов в каталоге, включая записи директорий (Не записей!)
        Удалённые записи в это число не входят. Последняя запись, с данными об
        оставшемся свободном месте так же не входит.
032     Суммарное количество блоков в файлах (Не записях!) каталога
0400    Метка принадлежности к формату Micro DOS (0123456)
0402    Метка формата каталога MK-DOS (051414)
0466    Размер диска в блоках, величина абсолютная для системы
(в отличие от NORD, NORTON и т.п.) принимающая не два значения под
40 или 80 дорожек, а скажем, если ваш дисковод понимает только 76
дорожек, то в этой ячейке нужно указать соответствующее число блоков
(это делается при инициализации)
0470    Номер блока первого файла
0500    Первая запись о файле

Формат записи о файле

Смещение Значение
---------------------------------------------
0       Статус файла (0 - обычный, 1 - защищён, 2 - логический диск,
0200 - BAD файл, 0377 - удалён)
причём сравнивать надо на == не бывает одновременно логический диск и защищён. и кстати,
если там любое другое значение отличное от вышеперечисленных - то статус - обычный.
1       Номер подкаталога которому принадлежит запись (0 - корень, для удалённых записей - 0377)
2       Имя файла 14. символов
020     Номер блока
022     Длина в блоках
024     Адрес
026     Длина (остаток длины по модулю 0200000)

Если первый символ в имени файла имеет код 0177, то это подкаталог.
Тогда вместо статуса у него байтом указан номер этого подкаталога,
и все файлы (в т.ч. подкаталоги), имеющие эту цифру в ячейке номера
подкаталога, принадлежат этому подкаталогу.

в общем тут выяснилось, что у каталога может быть явно задаваемый конец каталога,
а может и не быть.
это последняя запись с такими параметрами
статус файла == 0
номер подкаталога == 0
имя файла == 14. нулей (любое значение)
номер блока == номер первого свободного блока
длина в блоках == размер свободной области
адрес == 0 (любое значение)
длина == 0 (любое значение)
Последняя запись не входит в число записей (число по смещению 030)

Эксперимент показал, что размер каталога MKDOS 0255(173.) записи, больше в него не лезет.
Причём при добавлении последней записи происходят какие-то глюки. Там признак конца каталога
уже не влезает, и на экран выводится мусор, а потом и в каталог захватывается мусор

*/
