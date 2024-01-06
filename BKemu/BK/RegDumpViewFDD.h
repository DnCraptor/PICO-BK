#pragma once

#include "MultiEditListCtrl.h"
#include "DocPaneDlgViewBase.h"

class CDebugger;

class CRegDumpViewFDD : public CDocPaneDlgViewBase
{
		DECLARE_DYNAMIC(CRegDumpViewFDD)

		// номера колонок для listFDD
		enum LSTFDD_COL : int
		{
			NAME = 0,
			VALUE
		};

		// номера строки для listFDD
		enum LSTFDD_ROW : int
		{
			REG177130IN = 0,
			REG177130OUT,
			REG177132IN,
			REG177132OUT,
			FDDROWNUM
		};

		// номера колонок для listHDD
		enum LSTHDD_COL : int
		{
			NAME1 = 0,
			VALUE11,
			VALUE12,
			NAME2,
			VALUE21,
			VALUE22
		};

		// номера строк для listHDD
		enum LSTHDD_ROW : int
		{
			MODE = 0,
			R1F0,
			R1F1,
			R1F2,
			R1F3,
			R1F4,
			R1F5,
			R1F6,
			R1F7,
			R3F6,
			R3F7,
			HDDROWNUM
		};

		CDebugger         *m_pDebugger;
		CMultiEditListCtrl m_listFDD;
		CMultiEditListCtrl m_listHDD;

		static const UINT    m_pLISTFDDIDs[LSTFDD_ROW::FDDROWNUM];
		static const CString m_plistHDDHDrs[LSTHDD_ROW::HDDROWNUM];

	public:
		enum { IDD = IDD_REGDUMP_FDD_DLG };
		CRegDumpViewFDD();
		virtual ~CRegDumpViewFDD() override;

		void AttachDebugger(CDebugger *pDbgr)
		{
			m_pDebugger = pDbgr;
		}

		void            DisplayRegDump();

	protected:
		void            DisplayFDDData();
		void            DisplayHDDData();
		void            ResizeList(CMultiEditListCtrl *list, CWnd *grp, int height, CPoint &ptLT, int &maxGW);
		void            SetGrpWidth(CWnd *grp, int width);

	///	virtual void    DoDataExchange(CDataExchange *pDX) override;    // DDX/DDV support

		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg void    OnLvnItemchangedMdFddData(NMHDR *pNMHDR, LRESULT *pResult);

		DECLARE_MESSAGE_MAP()
};

