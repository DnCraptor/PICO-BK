// BK_settings.h : Диалоговое окно CSettingsDlg используется для описания и изменения Настроек приложения
//
#pragma once
#include "afx.h"
#ifndef __AFXWIN_H__
#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы
#include "BaseDialog.h"


class CSettingsDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CSettingsDlg)

	public:
		// Данные диалогового окна
		enum { IDD = IDD_SETTINGSBOX };
		CSettingsDlg();
		virtual ~CSettingsDlg() override
		    = default;

	protected:
		CStatic         m_wndTabLoc;
		CMFCTabCtrl     m_ctrTab;           // Пусть в этой переменной будут торчать наши паги
		//static const int m_log_page = 0;    // Temp : deactivate Log Page (VERSION_HISTORY), because code not present
		std::vector<CBaseDialog *> m_vDlgs;

		DWORD           Save();
		void            ReleaseTabs();
		void            AddTab(CBaseDialog *pDlg, UINT nDlgID, UINT nTitleID);

		// Реализация
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		virtual void OnCancel() override;

		DECLARE_MESSAGE_MAP()
};
