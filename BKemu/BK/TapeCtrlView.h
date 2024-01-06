#pragma once

#include "DocPaneDlgViewBase.h"

// CTapeCtrlView
class CTape;

class CTapeCtrlView : public CDocPaneDlgViewBase
{
		DECLARE_DYNAMIC(CTapeCtrlView)

		CTape          *m_pTape;
	///	HICON           m_iconRecordActive, m_iconRecordPassive, m_iconRecordStop;
	///	HICON           m_iconRecordStart, m_iconRecordPause;

		CButton         m_btnRecord;
		CButton         m_btnPlay;
		CButton         m_btnStop;
		bool            m_bPlay;

	public:
		enum { IDD = IDD_TAPE_CONTROL };
		CTapeCtrlView();
		virtual ~CTapeCtrlView() override;

		void            InitParams(CTape *pTape);
		void            StartPlayTape();

	protected:
		void            ShowSaveDialog();
	///	virtual void    DoDataExchange(CDataExchange *pDX) override;    // DDX/DDV support
		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg void    OnTimer(UINT_PTR nIDEvent);
		afx_msg void    OnDestroy();

		afx_msg void    OnTcRecord();
		afx_msg void    OnTcStop();
		afx_msg void    OnTcPlay();
		afx_msg void    OnUpdateTcRecord(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateTcStop(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateTcPlay(CCmdUI *pCmdUI);
		afx_msg void    OnBnClickedTcAutobegin();
		afx_msg void    OnBnClickedTcAutoend();
		afx_msg void    OnUpdateTcAutobegin(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateTcAutoend(CCmdUI *pCmdUI);
		DECLARE_MESSAGE_MAP()
};
