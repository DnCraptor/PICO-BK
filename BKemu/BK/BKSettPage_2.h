#pragma once
#include "resource.h"       // основные символы

#include "BaseDialog.h"

struct Bool_Items
{
	UINT itemID;
	bool *id;       // Указатель на переменную
	BOOL bValue;    // текущее значение
};

constexpr auto OPTIONS_ARRAYSIZE = 33;

// Диалоговое окно BKSettDlg_2
class BKSettDlg_2 : public CBaseDialog
{
		DECLARE_DYNAMIC(BKSettDlg_2)

		static Bool_Items arOptionItems[OPTIONS_ARRAYSIZE];
		int m_nVKBDVw;
		int m_nRegDumpIntrv;

	public:
		BKSettDlg_2(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~BKSettDlg_2() override;

// Данные диалогового окна
		enum { IDD = IDD_SETTINGS_2 };

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		virtual void OnCancel() override;

		DWORD Save();

		afx_msg LRESULT OnSendToTab(WPARAM, LPARAM);
		afx_msg void OnBnClickedCheckSettEmucovox();
		afx_msg void OnBnClickedCheckSettEmumenestrel();
		afx_msg void OnBnClickedCheckSettEmuay();
		afx_msg void OnBnClickedCheckSettJoystick();
		afx_msg void OnBnClickedCheckSettEmumouse();
		afx_msg void OnBnClickedCheckSettIclblock();
		DECLARE_MESSAGE_MAP()
};
