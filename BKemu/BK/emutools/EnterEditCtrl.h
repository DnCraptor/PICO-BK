#pragma once

class CEnterEdit : public CEdit
{
	///	DECLARE_DYNAMIC(CEnterEdit)

	public:
		CEnterEdit();
		virtual ~CEnterEdit() override;

	protected:
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnKillFocus(CWnd *pNewWnd);
	///	DECLARE_MESSAGE_MAP()
};


