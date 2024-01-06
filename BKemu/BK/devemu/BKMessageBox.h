// BKMessageBox.h: interface for the CBKMessageBox class.
//

#pragma once
#include "afx.h"
#include "resource.h"

class CBKMessageBox {
		HWND m_hwnd;
	public:
		CBKMessageBox() {
			m_hwnd = 0;// AfxGetMainWnd()->GetSafeHwnd();
		};
		virtual ~CBKMessageBox() = default;
		int Show(CString strText, UINT nType = MB_OK, UINT nIDHelp = 0);
		int Show(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0);
		int Show(UINT strID, UINT nType = MB_OK, UINT nIDHelp = 0);
};

extern CBKMessageBox g_BKMsgBox;
