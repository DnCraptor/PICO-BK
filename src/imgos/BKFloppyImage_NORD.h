#pragma once
#include "BKFloppyImage_Prototype.h"
#include <memory>

#pragma pack(push)
#pragma pack(1)
struct NordFileRecord
{
	uint8_t     status;     // статус файла, или номер директории для записи-директории
	uint8_t     dir_num;    // номер директории, которой принадлежит запись
	uint8_t     name[14];   // имя файла/каталога, если начинается с байта 0177 - это каталог.
	uint16_t    start_block;// начальный блок
	uint16_t    len_blk;    // длина в блоках
	uint16_t    address;    // стартовый адрес
	uint16_t    length;     // длина
	NordFileRecord()
	{
		memset(this, 0, sizeof(NordFileRecord));
	}
	NordFileRecord &operator = (const NordFileRecord &src)
	{
		memcpy(this, &src, sizeof(NordFileRecord));
		return *this;
	}
	NordFileRecord &operator = (const NordFileRecord *src)
	{
		memcpy(this, src, sizeof(NordFileRecord));
		return *this;
	}
};
#pragma pack(pop)

constexpr auto FMT_NORD_BEGIN_BLOCK = 026;
constexpr auto FMT_NORD_CAT_RECORD_NUMBER = 030;
constexpr auto FMT_NORD_FIRST_FREE_BLOCK = 032;
constexpr auto FMT_NORD_WRITE_PROTECT_MARKER = 034;
constexpr auto FMT_NORD_DISK_TYPE = 042;
constexpr auto FMT_NORD_CAT_NUMBER = 044;
constexpr auto FMT_NORD_DISK_SIZE = 0466;
constexpr auto FMT_NORD_CAT_BEGIN = 0500;

class CBKFloppyImage_Nord :
	public CBKFloppyImage_Prototype
{
		std::unique_ptr<uint8_t[]> m_pCatBuffer;      // буфер каталога
		NordFileRecord *m_pDiskCat;
		unsigned int    m_nCatSize;         // размер буфера каталога, выровнено по секторам, включая служебную область
		unsigned int    m_nMKCatSize;       // размер каталога в записях
		unsigned int    m_nMKLastCatRecord; // индекс последней записи в каталоге, чтобы определять конец каталога
		int     m_nVersion;         // версия норда.

		/* поиск заданной записи в каталоге.
		делается поиск на полное бинарное соответствие
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord(NordFileRecord *pRec);
		/* поиск заданной записи в каталоге.
		если bFull==false делается поиск по имени
		если bFull==true делается поиск по имени и другим параметрам
		выход: -1 если не найдено,
		номер записи в каталоге - если найдено.
		*/
		int     FindRecord2(NordFileRecord *pRec, bool bFull = true);

		bool Squeeze();

		// проверка на конец каталога
		bool IsEndOfCat(NordFileRecord *pRec);
#ifdef _DEBUG
		void DebugOutCatalog(NordFileRecord *pMKRec);
#endif
	protected:
		virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
		virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
		virtual void OnReopenFloppyImage() override;

	public:
		CBKFloppyImage_Nord(const PARSE_RESULT &image);
		virtual ~CBKFloppyImage_Nord() override;

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
формат каталога - микродос.
секторы 0-011 - каталог, 012-023 - копия каталога
размер каталога - 012(10.) секторов
формат каталога:

Адрес   Значение
------------------------------------------
026     нoль для oбычнoгo кaтaлoгa либо нoмep ceктopa для
        кaтaлoгa лoгичecкoгo диcкa
        NORD 3.5: (этo знaчeниe иcпoльзyeтcя
        для aвтoмaтичecкoй oбpaбoтки кaтaлoгa
        лoгичecкoгo диcкa, кoгдa oн cкoпиpoвaн кaк
        цeлoe нa дpyгoe физичecкoe ycтpoйcтвo)
030     общее кол-во имён в каталоге (включая удалённые файлы
        и имена подкаталогов)
        NORD 3.5:
        oбщee кoличecтвo имeн в кaтaлoгe (нe! включaя!
        yдaлeнныe фaйлы. Вот жеж ЖОПА. из-за этого нельзя сделать сохранение на диск,
        т.к. никак нельзя достоверно определить версию норда. );
032     первый пустой сектор на диске после всех файлов.
        при активном удалении и создании файлов, эта ячейка очень скоро начинает указывать на последний + 1 сектор
        и получается тут размер диска. несмотря на наличие кучи дыр в каталоге.
034     признак защиты диска от записи, если тут содержится 0123456
        то диск защищён
036     NORD 3.5:
        ячeйкa зaдaния cкopocтeй диcкoвoдoв нa зaгpyзoчнoм
        диcкe (036 - диcкoвoдa A:, 037 - B:);

042     тип диска:
        0 - 80 дор. 2 стороны
        1 - 40 дор. 2 стороны
        2 - 80 дор. 1 сторона
        3 - 40 дор. 1 сторона
044     кол-во и состояние копий каталогов на диске:
        0,1 - одна копия каталога, начальный сектор 0
        2 - одна копия, начальный сектор 012
        3 - две копии. 0 и 012 секторы
        NORD 3.5 - место под вторую копию каталога резервируется, в старых нордах - нет.
        особенно касается логических дисков.
046     признак заполнения виртуального диска Е: (только для вирт. диска)
        Если виртуальный диск заполнен более чем наполовину, то
        эта ячейка обнуляется. Это сигнал, что системная страница БК-0011М
        испорчена и файлы, написанные для этого компьютера запускать нельзя.
        Иначе система НОРД может запускать файлы в собственном мониторе
        БК-0011М, если в каталоге они помечены расширением .11 или .11М
0100    байт идентификации каталога (равен 0)
0400    ячейка идентификации каталога (равна 0123456)
0402    NORD 3.5:
        буферная ячейка для возможности работы диска в системе MKDOS без
        порчи загрузчика;
0466    объём диска в секторах
        NORD 3.5:
        номер сектора после конца диска, т.е. размер диска в блоках.
        Иногда указывается размер 32767, что навевает на подозрения.
0500    начало каталога.

Формат записи о файле

Смещение Значение
---------------------------------------------
0       байт атрибута файла:
        0 - обычный
        1 - защищённый, для NORD 3.5 ещё и 010,0100
        2 - плохой
        4 - NORD 3.5: не cнимaeмaя защита
        0200-0377 - удалённый
        Для имён подкаталогов - их номера
1       байт - номер каталога, которому принадлежит запись
2       имя файла или подкаталога. Если первый байт = 0177, то это подкаталог
020     начальный сектор файла
022     длина файла в секторах
024     начальный адрес файла
026     длина файла в байтах (по модулю 0200000, т.е. если файл больше 64к,
        то тут хранится остаток длины файла)


в общем тут выяснилось, что у каталога есть явно задаваемый конец каталога.
это одна запись с такими параметрами
байт атрибута файла == 0377
номер каталога = 0377
имя файла  == 14 байтов 0377 для норд 3.5 или "-?-?-?-?-?-?-?" для норд 2.х
номер блока == размер диска в блоках
длина в блоках == 0177777
адрес == 0177777
длина == 0177777
причём, эта запись идёт дополнительно к записи о свободной области.

Но! При работе с дисками Норд в МКдосе концевая запись дублируется.
Вот почему на некоторых дисках их две.


Существует логический диск. Опознаётся по своему имени, лог.диск имеет имя F:, G: и т.п.


норд 1.10
норд 2.11
норд 3.5
*/