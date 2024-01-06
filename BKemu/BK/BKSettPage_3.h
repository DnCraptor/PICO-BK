#pragma once
#include "resource.h"       // основные символы

#include "BaseDialog.h"
#include "Config.h"

// Диалоговое окно BKSettDlg_3

class BKSettDlg_3 : public CBaseDialog
{
		DECLARE_DYNAMIC(BKSettDlg_3)

		BOOL        m_bCurrOrigScreenshotsSize;
		BOOL        m_bCurrBigButtons;
		BOOL        m_bCurrExclusiveOpenImages;
		BOOL        m_b2ndAyEnable;
		BOOL        m_nDateInsteadOfScreenshotNumber;
		int         m_nCurrScreenshotNumber;
		int         m_nCurrSSRPos;
		int         m_nCurrSndChipModelPos[AY_NUMS];
		int         m_nCurr2AYWorkMode;
		int         m_nCurrScreenRenderType;
		int         m_nCurrOscRenderType;
		CString     m_strCurrFFMPEGLine;
		CString     m_strCurrSoundChipFrequency;

	public:
		BKSettDlg_3(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~BKSettDlg_3() override;

// Данные диалогового окна
//#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_SETTINGS_3 };
//#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		virtual void OnCancel() override;

		DWORD Save();

		afx_msg LRESULT OnSendToTab(WPARAM, LPARAM);
		afx_msg void OnBnClickedButtonFfmpegDefault();
		afx_msg void OnBnClickedCheckDateInsteadOfScreenshotNumber();
		DECLARE_MESSAGE_MAP()
	public:
		afx_msg void OnBnClickedCheckSettSndchipmodel2();
};
