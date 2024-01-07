
// BKView.h : интерфейс класса CBKView
//

#pragma once

#include "Debugger.h"
#include "Config.h"
#include <vector>

class CScreen;
class CBKDoc;
class CPrintDlg;
class CBKVKBDView;

class CBKView : public CView
{
	protected: // создать только из сериализации
		CBKView();
	///	DECLARE_DYNCREATE(CBKView)

// Атрибуты
	public:
		CBKDoc             *GetDocument() const;

// Операции
	public:

// Переопределение
	public:
		virtual void        OnDraw(CDC *pDC) override;  // переопределено для отрисовки этого представления
		virtual BOOL        PreCreateWindow(CREATESTRUCT &cs) override;
///	protected:
	//	virtual BOOL        OnPreparePrinting(CPrintInfo *pInfo) override;
	//	virtual void        OnBeginPrinting(CDC *pDC, CPrintInfo *pInfo) override;
	//	virtual void        OnEndPrinting(CDC *pDC, CPrintInfo *pInfo) override;

// Реализация
	public:
		virtual ~CBKView() override;
#ifdef _DEBUG
		virtual void        AssertValid() const override;
		virtual void        Dump(CDumpContext &dc) const override;
#endif
		bool                ReCreateSCR();
		inline CScreen     *GetScreen()
		{
			return m_pScreen;
		}
		void                ClearKPRS()
		{
			m_kprs.clear();
		}
		inline void         DrawScreen();

		void                CapsLockOn();
		void                CapsLockOff();

	protected:
		enum                {PRINT_LINES_PER_PAGE = 66};
		enum                {PRINT_TAB_LENGTH = 8};

		struct BKKeyPrs_t
		{
			std::vector<UINT> vKeys; // вектор, всех одновременно нажатых клавиш. Если он пуст - то ни одна клавиша не нажата
			bool bKeyPressed; // флаг нажатия и удержания любой алфавитно-цифровой клавиши (для отключения автоповтора)
			void clear()
			{
				bKeyPressed = false;
				vKeys.clear();
			}
		};
		BKKeyPrs_t          m_kprs; // захват нажатых клавиш. Чтобы как в ВП1-014 было, имеет значение первая из всех одновременно нажатых клавиш.
		// все остальные нажатые - полностью игнорируются, пока не будут отпущены все нажатые.

		CONF_SCREEN_RENDER  m_nRenderType;  // тип рендера, который применён в текущем объекте m_pScreen
		CScreen*            m_pScreen;
		CDebugger           m_debugger;     // отладчик
		// начальные флаги диалога печати
		bool                m_bPrintScreen;
		bool                m_bPrintInverse;
		CString             m_strPrintTitle;
		enum class CAPS_STATE { UNINIT, PRESSED, UNPRESSED };
		CAPS_STATE			m_nCurrentCapsState; // текущее состояние кнопки капслок.

		bool                AddKeyToKPRS(const uint8_t nScanCode);
		bool                DelKeyFromKPRS(const uint8_t nScanCode);
		void                Print(CPrintDlg *pPrintDlg);
	///	uint16_t            PrintPage(CDC *pDC, CPrintInfo *pInfo, uint16_t currAddress, uint16_t endAddress);
		int                 CreateSCR();
		void                DeleteSCR();

		void                EmulateKeyDown(UINT nChar, UINT nFlags);
		void                EmulateKeyUp(UINT nChar, UINT nFlags);
		static HHOOK        m_hHook;
		static bool         m_bAppFocused;
		static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

		// CBKVKBDView        *GetKBDView();

// Созданные функции схемы сообщений
	protected:
		virtual void        OnInitialUpdate() override;
		afx_msg LRESULT     OnVirtualKeyDown(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT     OnVirtualKeyUp(WPARAM wParam, LPARAM lParam);
		afx_msg int         OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void        OnSize(UINT nType, int cx, int cy);
		afx_msg void        OnFilePrintPreview();
		afx_msg void        OnContextMenu(CWnd *pWnd, CPoint point);
		afx_msg void        OnCustomFilePrint();
		afx_msg void        OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void        OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void        OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void        OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void        OnClose();
		afx_msg void        OnSetFocus(CWnd *pOldWnd);
		afx_msg void        OnKillFocus(CWnd *pNewWnd);
	///	DECLARE_MESSAGE_MAP()
};

#ifdef _DEBUG  // отладочная версия в BKView.cpp
inline CBKDoc *CBKView::GetDocument() const
{
	return reinterpret_cast<CBKDoc *>(m_pDocument);
}
#endif

