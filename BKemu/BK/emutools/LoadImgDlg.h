#pragma once

// CLoadImgDlg
#include "Config.h"

class CLoadImgDlg : public CFileDialog
{
		DECLARE_DYNAMIC(CLoadImgDlg)

		static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
		static void DrawPreview(HWND hdlg, const CString &strPath);

	public:
		CLoadImgDlg(BOOL bOpenFileDialog, // TRUE для FileOpen, FALSE для FileSaveAs
		            LPCTSTR lpszDefExt = nullptr,
		            LPCTSTR lpszFileName = nullptr,
		            DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		            LPCTSTR lpszFilter = nullptr,
		            CWnd *pParentWnd = nullptr,
		            DWORD dwSize = 0,
		            BOOL bVistaStyle = FALSE); // если сделать FALSE в Win7 в отладочной версии случается debug assertion, впрочем не влияющий на работу.
		// если bVistaStyle = TRUE то не работают хуки, и принципиально невозможно как-то свою информацию показывать.
		virtual ~CLoadImgDlg() override;

	protected:
		static void GetImgFormat(const CString &strImageName, CString &strFormatName, CString &strSystemFlag);

		static bool AnalyseImage(const fs::path &fname, IMGFormat *pFormat);

		DECLARE_MESSAGE_MAP()
};


