#pragma once
#include "resource.h"       // основные символы

#include "Config.h"
#include "BaseDialog.h"


// Диалоговое окно Settings_1

class BKSettDlg_1 : public CBaseDialog
{
		DECLARE_DYNAMIC(BKSettDlg_1)

		// HDD Path's
		enum class DrvNmSetOPerate_t : int { OP_NONE, OP_EXCHANGE, OP_REPLACE };
		struct DrvNmInfo
		{
			UINT nDlgID;
			fs::path Old;
			fs::path New;
			DrvNmSetOPerate_t Operation;
		};

		DrvNmInfo m_aHDD[static_cast<int>(HDD_MODE::NUM_HDD)]
		{
			{ IDC_EDIT_SETT_HDD0, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE },
			{ IDC_EDIT_SETT_HDD1, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE }
		};

		DrvNmInfo m_aFDD[static_cast<int>(FDD_DRIVE::NUM_FDD)]
		{
			{ IDC_EDIT_SETT_FDDA, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE },
			{ IDC_EDIT_SETT_FDDB, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE },
			{ IDC_EDIT_SETT_FDDC, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE },
			{ IDC_EDIT_SETT_FDDD, g_strEmptyUnit.GetString(), g_strEmptyUnit.GetString(), DrvNmSetOPerate_t::OP_NONE }
		};

		CString m_arStrCurrDumpAddr[NUMBER_VIEWS_MEM_DUMP];
		CString m_strCurrDisasmAddr;
		CString m_strCurrCPURunAddr;
		int m_nCurrCPUFrequency;

	public:
		BKSettDlg_1(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~BKSettDlg_1() override;
		enum { IDD = IDD_SETTINGS_1 };

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		virtual void OnCancel() override;

		DWORD Save();

		void GetFileImage(const HDD_MODE eDrive);
		void GetFileImage(const FDD_DRIVE eDrive);
		void LoadFileImage(const HDD_MODE eDrive);
		void LoadFileImage(const FDD_DRIVE eDrive);
		void LoadFileImage_1(const CString &strFilterIMG, DrvNmInfo &di);
		void ClearImgPath(const HDD_MODE eDrive);
		void ClearImgPath(const FDD_DRIVE eDrive);
		void ClearImgPath_1(DrvNmInfo &di);
		void RenewFileImage_1(const fs::path &strImgName, DrvNmInfo &di);
		DWORD SaveDrvImgName(const HDD_MODE eDrive);
		DWORD SaveDrvImgName(const FDD_DRIVE eDrive);

		afx_msg void OnBnClickedSelect0();
		afx_msg void OnBnClickedSelect1();
		afx_msg void OnBnClickedExchange();
		afx_msg LRESULT OnSendToTab(WPARAM, LPARAM);
		afx_msg void OnBnClickedButtonClearHdd0();
		afx_msg void OnBnClickedButtonClearHdd1();
		afx_msg void OnBnClickedButtonClearFddA();
		afx_msg void OnBnClickedButtonClearFddB();
		afx_msg void OnBnClickedButtonClearFddC();
		afx_msg void OnBnClickedButtonClearFddD();
		afx_msg void OnBnClickedButtonSelectFddA();
		afx_msg void OnBnClickedButtonSelectFddB();
		afx_msg void OnBnClickedButtonSelectFddC();
		afx_msg void OnBnClickedButtonSelectFddD();
		DECLARE_MESSAGE_MAP()
};
