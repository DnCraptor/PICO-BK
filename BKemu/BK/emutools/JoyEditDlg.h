#pragma once

// CJKField

class CJKField : public CStatic
{
		DECLARE_DYNAMIC(CJKField)

	public:
		CJKField();
		virtual ~CJKField() override;
		void        SetVKey(UINT nVKey, CString &strVKName);
		UINT        GetVKey() const
		{
			return m_nVKey;
		}
		CString     GetVKeyName() const
		{
			return m_strVKeyName;
		}

	protected:
		UINT        m_nVKey;
		CString     m_strVKeyName;
		virtual void PreSubclassWindow() override;
		void        SetText(CString strText);

	protected:
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnClicked();
		afx_msg void OnKillFocus(CWnd *pNewWnd);
		DECLARE_MESSAGE_MAP()
};

#include "BaseDialog.h"
// Диалоговое окно CJoyEditDlg

class CJoyEditDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CJoyEditDlg)

		static const uint16_t masks[16];

	public:
		CJoyEditDlg(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~CJoyEditDlg() override;

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_JOYSTICKEDIT_DLG };
#endif

	protected:
		int m_nJoyValuesPos[BKJOY_PARAMLEN];
		CJKField m_ctrlEdJoy[BKJOY_PARAMLEN];
		void CheckIntegrity(int num);
		void SetDialogItems();
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		afx_msg void OnCbnSelchangeComboJoyUp();
		afx_msg void OnCbnSelchangeComboJoyDn();
		afx_msg void OnCbnSelchangeComboJoyLt();
		afx_msg void OnCbnSelchangeComboJoyRt();
		afx_msg void OnCbnSelchangeComboJoyFr();
		afx_msg void OnCbnSelchangeComboJoyAFr();
		afx_msg void OnCbnSelchangeComboJoyBtA();
		afx_msg void OnCbnSelchangeComboJoyBtB();
		afx_msg void OnBnClickedButtonJoydefault();
		DECLARE_MESSAGE_MAP()
};
