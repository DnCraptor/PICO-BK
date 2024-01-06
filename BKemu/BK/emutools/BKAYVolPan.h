#pragma once


#include "Config.h"
#include "BaseDialog.h"

// Диалоговое окно CAYVolPan

class CBKAYVolPan : public CBaseDialog
{
		DECLARE_DYNAMIC(CBKAYVolPan)

	public:
		CBKAYVolPan(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~CBKAYVolPan() override;

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_AYVOLPAN_DLG };
#endif

		struct slPan
		{
			CSliderCtrl ctrl;
			int nPanL;  // значения в процентах, для наглядности
			int nPanR;  // значения в процентах, для наглядности
		};

		struct slVol
		{
			CSliderCtrl ctrl;
			int nVol;   // значения в процентах, для наглядности
		};

		struct AYChan
		{
			slPan pan;
			slVol vol;
		};

		struct AYData
		{
			AYChan chan[AY_CHANS];
			// оригинальные значения, чтобы при отмене вернуть всё как было
			CConfig::AYVolPan_s orig;
			// тут будет храниться текущее значение
			CConfig::AYVolPan_s curr;
		};

	protected:
		AYData m_AY[AY_NUMS];
		void SetPanSlider(int nAY, CConfig::AYVolPan_s *s);
		void SetVolSlider(int nAY, CConfig::AYVolPan_s *s);
		void Save();

	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		virtual void OnCancel() override;
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
		afx_msg void OnBnClickedButtonAy1panRevert();
		afx_msg void OnBnClickedButtonAy1panDefault();
		afx_msg void OnBnClickedButtonAy1volRevert();
		afx_msg void OnBnClickedButtonAy1volDefault();
		afx_msg void OnBnClickedButtonAy2panRevert();
		afx_msg void OnBnClickedButtonAy2panDefault();
		afx_msg void OnBnClickedButtonAy2volRevert();
		afx_msg void OnBnClickedButtonAy2volDefault();
		DECLARE_MESSAGE_MAP()

};
