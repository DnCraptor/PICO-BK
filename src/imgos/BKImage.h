﻿#pragma once
extern "C" {
#include "debug.h"
#include "emulator.h"
}
#include "BKFloppyImage_Prototype.h"

/*
Класс, где будут все основные методы для работы с образом.
Открытие образа, закрытие,
добавление файлов/директорий (групповое)
удаление файлов/директорий (групповое и рекурсивное)
создание директорий
извлечение файлов и преобразование форматов
*/
enum class ADD_ERROR : int
{
	OK_NOERROR = 0, // нет ошибок
	IMAGE_NOT_OPEN, // файл образа не открыт
	FILE_TOO_LARGE, // файл слишком большой
	USER_CANCEL,    // операция отменена пользователем
	IMAGE_ERROR,    // ошибку смотри в nImageErrorNumber
	NUMBERS
};

extern std::string g_AddOpErrorStr[];

struct ADDOP_RESULT
{
	bool            bFatal;     // флаг необходимости прервать работу.
	ADD_ERROR       nError;     // номер ошибки в результате добавления объекта в образ
	IMAGE_ERROR     nImageErrorNumber; // номер ошибки в результате операций с образом
	BKDirDataItem   afr;        // экземпляр абстрактной записи, которая вызвала ошибку.
	// Она нам нужна будет для последующей обработки ошибок
	ADDOP_RESULT()
		: bFatal(false)
		, nError(ADD_ERROR::OK_NOERROR)
		, nImageErrorNumber(IMAGE_ERROR::OK_NOERRORS)
	{
		afr.clear();
	}
};

#define LVIS_FOCUSED            0x0001
#define LVIS_SELECTED           0x0002
#define LVIS_CUT                0x0004
#define LVIS_DROPHILITED        0x0008
#define LVIS_GLOW               0x0010
#define LVIS_ACTIVATING         0x0020

#define LVIS_OVERLAYMASK        0x0F00
#define LVIS_STATEIMAGEMASK     0xF000

#define LVNI_ALL                0x0000

#define LVNI_FOCUSED            0x0001
#define LVNI_SELECTED           0x0002
#define LVNI_CUT                0x0004
#define LVNI_DROPHILITED        0x0008
#define LVNI_STATEMASK          (LVNI_FOCUSED | LVNI_SELECTED | LVNI_CUT | LVNI_DROPHILITED)

#define LVNI_VISIBLEORDER       0x0010
#define LVNI_PREVIOUS           0x0020
#define LVNI_VISIBLEONLY        0x0040
#define LVNI_SAMEGROUPONLY      0x0080

#define LVNI_ABOVE              0x0100
#define LVNI_BELOW              0x0200
#define LVNI_TOLEFT             0x0400
#define LVNI_TORIGHT            0x0800
#define LVNI_DIRECTIONMASK      (LVNI_ABOVE | LVNI_BELOW | LVNI_TOLEFT | LVNI_TORIGHT)


#define LVM_GETNEXTITEM         (LVM_FIRST + 12)
#define ListView_GetNextItem(hwnd, i, flags) \
    (int)SNDMSG((hwnd), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))

#define LVFI_PARAM              0x0001
#define LVFI_STRING             0x0002
#define LVFI_SUBSTRING          0x0004  // Same as LVFI_PARTIAL
#define LVFI_PARTIAL            0x0008
#define LVFI_WRAP               0x0020
#define LVFI_NEARESTXY          0x0040

typedef unsigned int ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

class CBKListCtrl {
	/**
    typedef struct {
       DWORD_PTR p_data;
       std::string fname;
       std::string fsize;
	   std::string fattr;
    } row_t;
	std::vector<row_t> m_rows;
	*/
public:
	// номера колонок для основного режима
	enum { LC_FNAME_POS = 0, LC_TYPE_POS, LC_BLK_SIZE_POS, LC_ADDRESS_POS, LC_SIZE_POS, LC_ATTR_POS, LC_SPECIFIC_POS };
    void SetSpecificColumn(UINT nID) {
		DBGM_PRINT(("SetSpecificColumn(%d)", nID));
	}
	int GetTopIndex() {
		DBGM_PRINT(("GetTopIndex"));
		//// Извлекает индекс самого видимого элемента в представлении списка или представлении отчета.
		return 0;
	}
	int GetNextItem(int nItem, int nFlags) {
		/* TODO:
Геометрическое отношение запрошенного элемента к указанному элементу и состоянию запрошенного элемента. Геометрическое отношение может быть одним из следующих значений:
LVNI_ABOVE Выполняет поиск элемента, превышающего указанный элемент.
LVNI_ALL Выполняет поиск последующего элемента по индексу (значение по умолчанию).
LVNI_BELOW Ищет элемент, который находится под указанным элементом.
LVNI_TOLEFT Выполняет поиск элемента слева от указанного элемента.
LVNI_TORIGHT Выполняет поиск элемента справа от указанного элемента.
Состояние может быть равно нулю или может быть одним или несколькими из следующих значений:
LVNI_DROPHILITED Элемент имеет набор флага LVIS_DROPHILITED состояния.
LVNI_FOCUSED Элемент имеет набор флага LVIS_FOCUSED состояния.
LVNI_SELECTED Элемент имеет набор флага LVIS_SELECTED состояния.
		*/
		DBGM_PRINT(("GetNextItem(%d, %d)", nItem, nFlags));
		if (nFlags & LVNI_SELECTED) { // TODO:
			return -1;
		}
		return nItem + 1;
	}
	int InsertItem(int i, const char* str) {
		DBGM_PRINT(("InsertItem(%d, '%s')", i, str));
		return m_add_file_ext(i, str);
	}
	void SetItemText(int i, int c, const char* str) {
		DBGM_PRINT(("SetItemText(%d, %d, '%s')", i, c, str));
		m_set_file_attr(i, c, str);
	}
	void SetItemData(int i, DWORD_PTR pdata) {
		DBGM_PRINT(("SetItemData(%d, %08Xh)", i, pdata));
		m_set_file_attr(i, -1, (const char*)pdata);
	}
	DWORD_PTR GetItemData(int i) {
		DBGM_PRINT(("GetItemData(%d)", i));
		return (DWORD_PTR)m_get_file_data(i);
	}
	void DeleteAllItems() {
		DBGM_PRINT(("DeleteAllItems"));
        m_cleanup_ext();
	}
	size_t GetSelectedCount() {
		DBGM_PRINT(("GetSelectedCount"));
		return 0;
	}
};

class CBKImage
{
		fs::path                m_strStorePath;     // путь, куда будем сохранять файлы
		CBKListCtrl             m_ListCtrl;

		PaneInfo                m_PaneInfo;
		std::vector<PaneInfo>   m_vSelItems;

		std::unique_ptr<CBKFloppyImage_Prototype> m_pFloppyImage;
		std::vector<std::unique_ptr<CBKFloppyImage_Prototype>> m_vpImages; // тут будут храниться объекты, когда мы заходим в лог.диски

		bool m_bCheckUseBinStatus;      // состояние чекбоксов "использовать формат бин"
		bool m_bCheckUseLongBinStatus;  // состояние чекбоксов "использовать формат бин"
		bool m_bCheckLogExtractStatus;  // и "создавать лог извлечения" соответственно, проще их тут хранить, чем запрашивать сложными путями у родителя

	protected:
		void StepIntoDir(BKDirDataItem *fr);
		bool StepUptoDir(BKDirDataItem *fr);
		void OutCurrFilePath();

		bool ExtractObject(BKDirDataItem *fr);
		bool ExtractFile(BKDirDataItem *fr);
		bool DeleteRecursive(BKDirDataItem *fr);
		bool AnalyseExportFile(AnalyseFileStruct *a);
		void SetStorePath(const fs::path &str);
		bool ViewFile(BKDirDataItem *fr);
		bool ViewFileRT11(BKDirDataItem *fr);
		bool ViewFileAsSprite(BKDirDataItem *fr);

	public:
		CBKImage();
		~CBKImage();

		uint32_t Open(PARSE_RESULT &pr, const bool bLogDisk = false); // открыть файл по результатам парсинга
		uint32_t ReOpen(); // переинициализация уже открытого образа
		void Close(); // закрыть текущий файл
		const std::string GetImgFormatName(IMAGE_TYPE nType = IMAGE_TYPE::UNKNOWN);
		void ClearImgVector();
		void PushCurrentImg();
		bool PopCurrentImg();

		struct ItemPanePos
		{
			int nTopItem;
			int nFocusedItem;
			ItemPanePos() : nTopItem(0), nFocusedItem(0) {}
			ItemPanePos(int t, int f) : nTopItem(t), nFocusedItem(f) {}
		};

		CBKImage::ItemPanePos GetTopItemIndex();

		bool ReadCurrentDir(CBKImage::ItemPanePos pp);

		inline bool IsImageOpen()
		{
			return (m_pFloppyImage != nullptr);
		}

		inline bool GetImageOpenStatus()
		{
			return m_pFloppyImage->GetImageOpenStatus();
		}
		inline unsigned long GetBaseOffset() const
		{
			return m_pFloppyImage->GetBaseOffset();
		}
		inline unsigned long GetImgSize() const
		{
			return m_pFloppyImage->GetImgSize();
		}
		inline void SetCheckBinExtractStatus(bool bStatus = false)
		{
			m_bCheckUseBinStatus = bStatus;
		}
		inline void SetCheckUseLongBinStatus(bool bStatus = false)
		{
			m_bCheckUseLongBinStatus = bStatus;
		}
		inline void SetCheckLogExtractStatus(bool bStatus = false)
		{
			m_bCheckLogExtractStatus = bStatus;
		}
		void ItemProcessing(int nItem, BKDirDataItem *fr);
		void ViewSelected(const bool bViewAsText = true);
		void ExtractSelected(const fs::path &strOutPath);
		void RenameRecord(BKDirDataItem *fr);
		void DeleteSelected();

		// добавить в образ файл/директорию
		ADDOP_RESULT AddObject(const fs::path &findFile, bool bExistDir = false);
		void OutFromDirObject(BKDirDataItem *fr);
		// удалить из образа файл/директорию
		ADDOP_RESULT DeleteObject(BKDirDataItem *fr, bool bForce = false);
};
