#pragma once

#include "resource.h"       // основные символы
#include "Config.h"
#include "BaseDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Диалоговое окно CBKPaletteDlg


class CBKPaletteDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CBKPaletteDlg)

		CMFCColorButton m_btnPalMonoBK;
		CMFCColorButton m_btnPalMonoWH;

		CMFCColorButton m_arPalMonoAdapt[4];
		CMFCColorButton m_arPalColor[16][4];

		CComboBox m_comboPresetsMono;

	public:
		CBKPaletteDlg(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~CBKPaletteDlg() override;

// Данные диалогового окна
		enum { IDD = IDD_BKPALETTE_DLG };

	protected:
		COLORREF    BGRtoColorref(DWORD bgr) const;
		DWORD       ColorrefToBGR(COLORREF col) const;
		void        UpdateFileNameCtrl(bool clear = false) const;
		void        GrabPalette();
		void        FixPalette();
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		afx_msg void OnBnClickedButtonResetMonopal();
		afx_msg void OnBnClickedButtonResetMonoadaptPal();
		afx_msg void OnBnClickedButtonResetColorpal();
		afx_msg void OnCbnSelchangePresetMono();
		afx_msg void OnBnClickedButtonSaveColorpal();
		afx_msg void OnBnClickedButtonLoadColorpal();
		DECLARE_MESSAGE_MAP()
};
