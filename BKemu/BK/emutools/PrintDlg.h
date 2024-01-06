#pragma once

//

class CDebugger;

/////////////////////////////////////////////////////////////////////////////
// CPrintDlg dialog

class CPrintDlg : public CPrintDialog
{
		DECLARE_DYNAMIC(CPrintDlg)

		void                CalcPages();
		void                HideControls();
		BOOL                GetStartAddress();
		BOOL                GetEndAddress();

	public:
		CPrintDlg(BOOL bPrintSetupOnly,  // TRUE for Print Setup, FALSE for Print Dialog
		          DWORD dwFlags = PD_USEDEVMODECOPIES | PD_NOPAGENUMS
		                          | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
		          CWnd *pParentWnd = nullptr);
		virtual ~CPrintDlg() override;
		CDebugger          *m_pDebugger;
		CString             m_strTitle;
		BOOL                m_bPrintScreen;
		BOOL                m_bInverse;
		uint16_t            m_startAddr;
		uint16_t            m_endAddr;
		int                 m_nPages;

	protected:
		afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
		afx_msg void OnPrintScreen();
		afx_msg void OnPrintCode();
		afx_msg void OnInverse();
		afx_msg void OnOk();
		afx_msg void OnStartAddr();
		afx_msg void OnEndAddr();
		DECLARE_MESSAGE_MAP()
};
