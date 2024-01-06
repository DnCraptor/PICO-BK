#pragma once

#include "resource.h"       // основные символы

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*============================================================================*/
// CBaseDialog - стандартный микрософтовский диалог из MFC, отнаследованный от CBasePane
// чтобы можно было применять визуальные стили.
// Причём, это работает ТОЛЬКО ИСКЛЮЧИТЕЛЬНО для дочерних диалогов.
// для самостоятельных - нихера, потому что CBasePane и всем остальных фичам нужен родитель.
// если его нет - всё валится, в самых разных неожиданных местах.
// Однако сами стандартные контролы отображаются ужасно, они на это не рассчитаны. Особеннов в Windows XP

class CBaseDialog : public CBasePane
{
		DECLARE_DYNAMIC(CBaseDialog)
	protected:
		UINT            m_nIDHelp;              // Help ID (0 for none, see HID_BASE_RESOURCE)
		// parameters for 'DoModal'
		HGLOBAL         m_hDialogTemplate;      // indirect (m_lpDialogTemplate == nullptr)
		LPCDLGTEMPLATE  m_lpDialogTemplate;     // indirect if (m_lpszTemplateName == nullptr)
		void           *m_lpDialogInit;         // DLGINIT resource data
		CWnd           *m_pParentWnd;           // parent/owner window
		HWND            m_hWndTop;              // top level parent window (may be disabled)
		BOOL            m_bClosedByEndDialog;   // indicates that the dialog was closed by calling EndDialog method
	///	_AFX_OCC_DIALOG_INFO *m_pOccDialogInfo;

		// Modeless construct
	public:
		CBaseDialog();
		void Initialize();

		virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd *pParentWnd = nullptr);
		virtual BOOL Create(UINT nIDTemplate, CWnd *pParentWnd = nullptr);
		virtual BOOL CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd = nullptr,
		                            void *lpDialogInit = nullptr);
		virtual BOOL CreateIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd = nullptr);

		// Modal construct
	public:
		explicit CBaseDialog(LPCTSTR lpszTemplateName, CWnd *pParentWnd = nullptr);
		explicit CBaseDialog(UINT nIDTemplate, CWnd *pParentWnd = nullptr);
		BOOL InitModalIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd = nullptr,
		                       void *lpDialogInit = nullptr);
		BOOL InitModalIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd = nullptr);

		// Attributes
	public:
		void MapDialogRect(LPRECT lpRect) const;
		void SetHelpID(UINT nIDR);

		// Operations
	public:
		// modal processing
		virtual INT_PTR DoModal();

		// support for passing on tab control - use 'PostMessage' if needed
		void NextDlgCtrl() const;
		void PrevDlgCtrl() const;
		void GotoDlgCtrl(CWnd *pWndCtrl);

		// default button access
		void SetDefID(UINT nID);
		DWORD GetDefID() const;

		// termination
		void EndDialog(int nResult);

		// Overridables (special message map entries)
		virtual BOOL OnInitDialog();
		virtual void OnSetFont(CFont *pFont);
	protected:
		virtual void OnOK();
		virtual void OnCancel();

		// Implementation
	public:
		virtual ~CBaseDialog() override;
#ifdef _DEBUG
		virtual void AssertValid() const override;
		virtual void Dump(CDumpContext &dc) const override;
#endif
		virtual BOOL PreTranslateMessage(MSG *pMsg) override;
		virtual BOOL OnCmdMsg(UINT nID, int nCode, void *pExtra,
		                      AFX_CMDHANDLERINFO *pHandlerInfo) override;
	///	virtual BOOL CheckAutoCenter() override;

	protected:
	///	virtual BOOL SetOccDialogInfo(_AFX_OCC_DIALOG_INFO *pOccDialogInfo) override;
	///	virtual _AFX_OCC_DIALOG_INFO *GetOccDialogInfo() override;
		virtual void PreInitDialog();

		// implementation helpers
		HWND PreModal();
		void PostModal();

		BOOL CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd,
		                    void *lpDialogInit, HINSTANCE hInst);
		BOOL CreateIndirect(HGLOBAL hDialogTemplate, CWnd *pParentWnd,
		                    HINSTANCE hInst);

	protected:
		afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);
		afx_msg void OnSetFont(CFont *pFont, BOOL bRedraw);
		afx_msg BOOL OnQueryEndSession();
		afx_msg void OnEndSession(BOOL bEnding);
		afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
		afx_msg void OnGetMinMaxInfo(MINMAXINFO *lpMMI);

		DECLARE_MESSAGE_MAP()
};
