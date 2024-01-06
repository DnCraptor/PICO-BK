#pragma once
// StaticLink.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStaticLink window

class CStaticLink : public CStatic
{
		DECLARE_DYNAMIC(CStaticLink)

		bool    m_bVisited;
		CFont   m_Font, m_UnderlineFont;
		static COLORREF m_colorUnvisited;
		static COLORREF m_colorVisited;
		HCURSOR m_cursorHand, m_cursorArrow, m_cursor;
		TRACKMOUSEEVENT m_tme;

// Construction
	public:
		CStaticLink();
		virtual ~CStaticLink() override;

// Overrides
		// ClassWizard generated virtual function overrides
	protected:
		virtual void PreSubclassWindow() override;

// Implementation

		// Generated message map functions
	protected:
		afx_msg HBRUSH CtlColor(CDC *pDC, UINT nCtlColor);
		afx_msg void OnStnClicked();
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnMouseHover(UINT nFlags, CPoint point);
		afx_msg void OnMouseLeave();
		DECLARE_MESSAGE_MAP()
};


