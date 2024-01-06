#pragma once

// LoadMemoryDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadMemoryDlg dialog

class CLoadMemoryDlg : public CFileDialog
{
		DECLARE_DYNAMIC(CLoadMemoryDlg)

		static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
		static void ClearItems(CWnd *pDlg, const CString &strMsg);
		static void DrawPreview(HWND hdlg, const fs::path &strPath);

	public:
		CLoadMemoryDlg(BOOL bOpenFileDialog,  // TRUE for FileOpen, FALSE for FileSaveAs
		               LPCTSTR lpszDefExt = nullptr,
		               LPCTSTR lpszFileName = nullptr,
		               DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		               LPCTSTR lpszFilter = nullptr,
		               CWnd *pParentWnd = nullptr,
		               DWORD dwSize = 0,
		               BOOL bVistaStyle = FALSE); // если сделать FALSE в Win7 в отладочной версии случается debug assertion, впрочем не влияющий на работу.

	protected:
		DECLARE_MESSAGE_MAP()
};
