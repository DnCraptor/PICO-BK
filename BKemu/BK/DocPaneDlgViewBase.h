// Базовые классы для диалога и присоединяемой панели
// внутри которой этот диалог используется.

#pragma once
#include "resource.h"

class CDocPaneDlgViewBase : public CPaneDialog
{
	//	DECLARE_DYNAMIC(CDocPaneDlgViewBase)

		CFont           m_hFont;
		CMFCToolTipCtrl m_ToolTip;

	public:
		CDocPaneDlgViewBase();
		virtual ~CDocPaneDlgViewBase() override;

	protected:
		void            CorrectHeight();
		virtual void    AdjustLayout() override;
		virtual void    OnAfterFloat() override;
		virtual void    OnAfterDockFromMiniFrame() override;
		virtual BOOL    OnShowControlBarMenu(CPoint pt) override;
		virtual BOOL    PreTranslateMessage(MSG *pMsg) override;
		virtual BOOL    PreCreateWindow(CREATESTRUCT &cs) override;
		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg BOOL    OnEraseBkgnd(CDC *pDC);
		afx_msg HBRUSH  OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
	///	DECLARE_MESSAGE_MAP()
};

