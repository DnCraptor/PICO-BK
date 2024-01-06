// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCustomSliderCtrl window

class CSliderButton;

class CCustomSliderCtrl : public CSliderCtrl
{
		DECLARE_DYNAMIC(CCustomSliderCtrl)

// Construction
	public:
		CCustomSliderCtrl(CSliderButton &btnSlider);

// Attributes
	public:
		CSliderButton &m_btnSlider;

// Operations
	public:
// Overrides

// Implementation
	public:
		virtual ~CCustomSliderCtrl() override;

	protected:
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

		DECLARE_MESSAGE_MAP()
};


class CSliderButton : public CMFCToolBarButton
{
		DECLARE_SERIAL(CSliderButton)

	public:
		CSliderButton();
		CSliderButton(UINT uiId,
		              int iImage = -1,
		              DWORD dwStyle = 0,
		              int iWidth = 0);

		virtual ~CSliderButton() override;

		void SetRange(int iMin, int iMax);
		void SetValue(int iValue, BOOL bNotify = TRUE);
		int GetValue();

		static void SetValueSt(UINT uiCmd, int iValue, BOOL bNotify = TRUE);
		static int GetPosSt(UINT uiCmd);

// Overrides:
		virtual void Serialize(CArchive &ar) override;
		virtual void CopyFrom(const CMFCToolBarButton &src) override;
		virtual SIZE OnCalculateSize(CDC *pDC, const CSize &sizeDefault, BOOL bHorz) override;
		virtual void OnShow(BOOL bShow) override;
		virtual void OnChangeParentWnd(CWnd *pWndParent) override;
		virtual void OnMove() override;
		virtual void OnSize(int iSize) override;
		virtual HWND GetHwnd() override
		{
			return m_wndSlider.GetSafeHwnd();
		}

		virtual BOOL CanBeStretched() const override
		{
			return TRUE;
		}

		virtual BOOL HaveHotBorder() const override
		{
			return FALSE;
		}

		virtual void OnDraw(CDC * /*pDC*/, const CRect & /*rect*/, CMFCToolBarImages * /*pImages*/,
		                    BOOL /*bHorz*/ = TRUE, BOOL /*bCustomizeMode*/ = FALSE,
		                    BOOL /*bHighlight*/ = FALSE,
		                    BOOL /*bDrawBorder*/ = TRUE,
		                    BOOL /*bGrayDisabledButtons*/ = TRUE) override {}
// Attributes:
	public:
		CCustomSliderCtrl   m_wndSlider;
		int             m_iWidth;
		DWORD           m_dwStyle;
		DWORD           m_dwStyleEx;
		int             m_nMin;
		int             m_nMax;
		int             m_nValue;
};
