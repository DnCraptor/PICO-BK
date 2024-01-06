#pragma once
#include "afx.h"
// MultiEditListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMultiEdit window

class CMultiEditListCtrl;

class CMultiEdit : public CEdit
{
	//	DECLARE_DYNAMIC(CMultiEdit)

		CMultiEditListCtrl *m_pParent;
		int                 m_nItem;
		int                 m_nSubItem;
		bool                m_bFirstPress;
		void                SetItem();

// Construction
	public:
		CMultiEdit(CMultiEditListCtrl *pListCtrl, const int nItem, const int nSubItem);

// Attributes
	public:

// Operations
	public:

// Implementation
	public:
		virtual ~CMultiEdit() override;

		// Generated message map functions
	protected:
		afx_msg void OnKillFocus(CWnd *pNewWnd);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMultiEditListCtrl window
constexpr COLORREF MODIFIED_COLOR = RGB(255, 0, 0);

class CMultiEditListCtrl : public CListCtrl
{
	//	DECLARE_DYNAMIC(CMultiEditListCtrl)
		CMultiEdit         *m_pEdit;
		CFont               m_font;
		bool                m_bDigits;  // флаг, что можно принимать только цифры
	protected:
		struct ITEM_INFO
		{
			bool            bEditable;      // флаг, что поле можно редактировать
			bool            bModified;      // флаг, что значение в поле изменилось
			bool            bEdited;        // флаг, что происходит процесс редактирования
			DWORD           nValue;         // текущее значение
			COLORREF        clrText;        // текущий цвет текста
			COLORREF        clrTextBk;      // текущий цвет фона
			COLORREF        clrModText;     // заданный пользователем цвет текста
			COLORREF        clrModTextBk;   // заданный пользователем цвет фона
			ITEM_INFO(): bEditable(true), bModified(false), bEdited(false), nValue(0xffffffff), clrText(0), clrTextBk(0), clrModText(0), clrModTextBk(0) {};
		};

		std::vector<std::vector<ITEM_INFO>> m_itemsTable;

		COLORREF            m_StdColor;
		COLORREF            m_StdBgColor;

		void                InitColorTable();
		void                ClearColorTable();
		int                 GetColumnCount() const;

	public:
		CMultiEditListCtrl();
		virtual ~CMultiEditListCtrl() override;
		void                AcceptDigits(bool bDigits)
		{
			m_bDigits = bDigits;
		}
		bool                IsDigitsOnly() const
		{
			return m_bDigits;
		}
		void                EnableEdit(const bool bEnable, const int nRow, const int nColumn);
		void                EnableRowEdit(const bool bEnable, const int nRow);
		void                EnableColumnEdit(const bool bEnable, const int nColumn);
		void                SetItemColor(const COLORREF col, const int nRow, const int nColumn);
		void                SetRowColor(const COLORREF col, const int nRow);
		void                SetColumnColor(const COLORREF col, const int nColumn);
		void                SetItemBkColor(const COLORREF col, const int nRow, const int nColumn);
		void                SetRowBkColor(const COLORREF col, const int nRow);
		void                SetColumnBkColor(const COLORREF col, const int nColumn);
		void                SetItemWithModified(const uint16_t word, const int nItem, const int nSubitem, bool bColored = true);
		void                SetItemWithModifiedASCII(const CString &str, const int nItem, const int nSubitem);
		void                SetEndEdit(const int nItem, const int nSubitem);
// Overrides
	protected:
	///	virtual void        PreSubclassWindow() override;

// Generated message map functions
	protected:
		afx_msg void OnDblclk(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnLvnInsertitem(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnLvnDeleteitem(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnLvnDeleteallitems(NMHDR *pNMHDR, LRESULT *pResult);
	///	DECLARE_MESSAGE_MAP()

};


