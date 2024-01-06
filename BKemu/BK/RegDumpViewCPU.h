#pragma once

#include "MultiEditListCtrl.h"
#include "DocPaneDlgViewBase.h"

constexpr auto CUSTOMVIEW_REGS_NUM = 2;
enum CUSTOMVIEW_MODE : int
{
	CV_DEC_VIEW = 0,
	CV_HEX_VIEW
};

struct CUSTOMVIEW_MODE_PARAM
{
	CUSTOMVIEW_MODE mode;
	UINT strID;
};

class CDebugger;

class CRegDumpViewCPU : public CDocPaneDlgViewBase
{
		DECLARE_DYNAMIC(CRegDumpViewCPU)

		// номера колонок для listCPU
		enum LISTCPU_C : int
		{
			COL_NAME = 0,
			COL_VALUE,
			COL_CUSTOM
		};
		// номера строк для listCPU
		enum LISTCPU_L : int
		{
			LINE_R0 = 0,
			LINE_R1,
			LINE_R2,
			LINE_R3,
			LINE_R4,
			LINE_R5,
			LINE_SP,
			LINE_PC,
			LINE_PSW,
			CPU_L_NUM
		};

		// номера колонок для listSys
		enum LISTSYS_C : int
		{
			COL_NAME1 = 0,
			COL_VALUE1,
			COL_NAME2,
			COL_VALUE2
		};
		// номера строк для listSys, колонки 0,1
		enum LISTSYS_L : int
		{
			LINE_REG177660 = 0,
			LINE_REG177662IN,
			LINE_REG177662OUT,
			LINE_REG177664,
			LINE_REG177714IN,
			LINE_REG177714OUT,
			LINE_REG177716IN,
			LINE_REG177716OUT_TAP,
			LINE_REG177716OUT_MEM,
			SYS_L_NUM
		};
		// номера строк для listSys, колонки 2,3
		enum LISTSYS_L2 : int
		{
			LINE_REG177700 = 0,
			LINE_REG177702,
			LINE_REG177704,
			LINE_REG177706,
			LINE_REG177710,
			LINE_REG177712
		};

		// номера колонок для listAltPro
		enum LISTALTPRO_C : int
		{
			COL_NAMEA = 0,
			COL_VALUEA
		};
		// номера строк для listAltPro
		enum LISTALTPRO_L : int
		{
			LINE_MODE = 0,
			LINE_CODE,
			AP_L_NUM
		};

		CDebugger          *m_pDebugger;
		CMultiEditListCtrl  m_listCPU;
		CMultiEditListCtrl  m_listSys;
		CMultiEditListCtrl  m_listAltPro;

		static const UINT   m_pListCpuIDs[LISTCPU_L::CPU_L_NUM];
		static const UINT   m_pListSysIDs[2][LISTSYS_L::SYS_L_NUM];
		static const UINT   m_pListSysRegs[2][LISTSYS_L::SYS_L_NUM];
		static const UINT   m_pListAltProIDs[LISTALTPRO_L::AP_L_NUM];

		int             m_nCurrentCVM; // индекс текущего режима
		static CUSTOMVIEW_MODE_PARAM m_cvArray[CUSTOMVIEW_REGS_NUM]; // в этом массиве

		int             m_nCurrCPUFreq;
		CSpinButtonCtrl m_spinCPUFreq;

	public:
		enum { IDD = IDD_REGDUMP_CPU_DLG };
		CRegDumpViewCPU();
		virtual ~CRegDumpViewCPU() override;

		void            AttachDebugger(CDebugger *pDbgr);
		void            SetFreqParam();
		void            UpdateFreq();
		void            DisplayRegDump();

	protected:
		void            DisplayPortRegs();
		void            DisplayRegisters();
		void            DisplayAltProData();
		void            SetPSW(uint16_t value);
		uint16_t        GetPSW(CString &strPSW);
		CString         FormatPSW(uint16_t value);
		void            ResizeList(CMultiEditListCtrl *list, CWnd *grp, int height, CPoint &ptLT, int &maxGW);
		void            SetGrpWidth(CWnd *grp, int width);
		CString         WordToCustom(uint16_t value);
		uint16_t        CustomToWord(const CString &str);

		// Generated message map functions
	///	virtual void    DoDataExchange(CDataExchange *pDX) override;    // DDX/DDV support

		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg void    OnLvnItemchangedMdCpuRegisters(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void    OnLvnItemchangedMdAltProData(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void    OnLvnItemchangedMdSysRegisters(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void    OnEnChangeEditDbg1Cpufreq();
		afx_msg void    OnBnClickedButtonDbg1CpufreqDec();
		afx_msg void    OnBnClickedButtonDbg1CpufreqInc();
		afx_msg void    OnBnClickedButtonDbg1CpufreqBaseset();
		afx_msg void    OnBnClickedButtonDbg1CpufreqMaxset();
		afx_msg void    OnBnClickedRegdumpCvmode();
		afx_msg void    OnUpdateButtonDbg1CpufreqDec(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateButtonDbg1CpufreqInc(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateButtonDbg1CpufreqMaxset(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateButtonDbg1CpufreqBaseset(CCmdUI *pCmdUI);
		afx_msg void    OnUpdateRegdumpCvmode(CCmdUI *pCmdUI);
		DECLARE_MESSAGE_MAP()
};

