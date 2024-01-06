#pragma once

// DisasmCtrl.h : header file
//
#include <mutex>
#include "MultiEditListCtrl.h"

// массив номеров колонок списка
enum DISASM_LIST : int
{
	COL_MARK = 0,
	COL_ADDR,
	COL_INSTR,
	COL_COMMENT,
	COLSNUM
};

class CDebugger;

/////////////////////////////////////////////////////////////////////////////
// CDisasmCtrl window

class CDisasmCtrl : public CMultiEditListCtrl
{
	///	DECLARE_DYNAMIC(CDisasmCtrl)

		CDebugger          *m_pDebugger;
		int                 m_nSelection;
		std::mutex          m_lock;

	///	HICON               m_hBPIcon, m_hCurrIcon;

		struct ItemParam
		{
			uint16_t nAddr; // адрес дизассемблированной инструкции
			int nLen;       // длина в словах дизассемблированной инструкции
			CString instr;  // текст дизассемблированой инструкции
		};

		std::vector<ItemParam> m_vItemsParam;

// Construction
	public:
		CDisasmCtrl();
		virtual ~CDisasmCtrl() override;

// Operations
	public:
		void                AttachDebugger(CDebugger *pDebugger);
		void                OutContent();
		void                Init();
		void                ChangeInstructionSet();
		uint16_t            GetCursorAddr() const;
		uint16_t            GetBottomAddr() const;

	protected:
		void                DisasmStepUp(int nCount = 1) const;
		void                DisasmStepDn(int nCount = 1) const;
		void                DrawColoredText(CDC *pDC, CRect &rect, CString &str);
		uint16_t            GetLineAddress(int nRow) const;
		uint16_t            StepBackward(int nSteps = 1) const;
		uint16_t            StepForward(int nSteps = 1) const;

// Generated message map functions
	protected:
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnSetFocus(CWnd *pOldWnd);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	///	DECLARE_MESSAGE_MAP()
};

