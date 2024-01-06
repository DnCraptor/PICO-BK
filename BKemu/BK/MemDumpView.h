#pragma once

#include "DumpListCtrl.h"
#include "EnterEditCtrl.h"
#include "Config.h"
#include "DocPaneDlgViewBase.h"
#include "BaseDialog.h"

class CDebugger;

struct DISPLAY_MODE_PARAM
{
	DUMP_DISPLAY_MODE mode;
	UINT strID;
};

constexpr auto DISPLAY_MODES_NUM = 2;

class CMemDumpView : public CDocPaneDlgViewBase
{
		DECLARE_DYNAMIC(CMemDumpView)

		CDebugger      *m_pDebugger;

		CDumpListCtrl   m_ListMemory;
		CEnterEdit      m_EditAddr;

		CComboBox       m_cbWndList;
		CComboBox       m_cbWndAddr;

		CString         m_strDumpAddr;
		int             m_nCurrentDDM; // индекс текущего режима
		static DISPLAY_MODE_PARAM m_dmArray[DISPLAY_MODES_NUM]; // в этом массиве

		uint8_t        *m_pDmpEAPtr;    // указатель на начало окна в массиве m_pMemory
		VWinParam      *m_pV;

// Создание
	public:
		enum { IDD = IDD_MEMDUMP_DLG };
		CMemDumpView();
		virtual ~CMemDumpView() override;

// Реализация
	public:
		void            DisplayMemDump();

		void            AttachDebugger(CDebugger *pDbgr);

		uint16_t        GetAddress()
		{
			return m_ListMemory.GetAddress();
		}
		void            SetAddress(uint16_t addr);
		void            SetDumpWindows(VWinParam *pV, int nPageListPos, int nAddrListPos);

		int             GetDumpPageListPos();
		int             GetDumpAddrListPos();

		uint16_t        GetDebugMemDumpWord(uint16_t dumpAddress);
		uint8_t         GetDebugMemDumpByte(uint16_t dumpAddress);
		void            SetDebugMemDumpWord(uint16_t dumpAddress, uint16_t value);
		void            SetDebugMemDumpByte(uint16_t dumpAddress, uint8_t value);

		void            RefreshDump(uint8_t *pDmpEAPtr);

	protected:
		void            LoadFunc(uint16_t nLoadAddr = 0);
	///	virtual void    DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV

		// Созданные функции схемы сообщений
		LRESULT         OnDumpAddressChange(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg void    OnSetFocus(CWnd *pOldWnd);
		afx_msg void    OnBnClickedButtonDumpMode();
		afx_msg void    OnBnClickedButtonSaveDump();
		afx_msg void    OnBnClickedButtonLoadDump();
		afx_msg void    OnUpdateButton(CCmdUI *pCmdUI);
		afx_msg void    OnCbnSelchangeComboMdWindows();
		afx_msg void    OnCbnSelchangeComboMdWindaddr();
		DECLARE_MESSAGE_MAP()
};

struct SvDmpParam
{
	int nBgnAddr;
	int nLength;
	int nPage0;
	int nPage1;
	CRect rect;
	SvDmpParam()
		: nBgnAddr(0)
		, nLength(0)
		, nPage0(1)
		, nPage1(2)
	{};
};

class CSvDmpParamDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CSvDmpParamDlg)
		// Создание
	public:
		CSvDmpParamDlg(SvDmpParam *pParam = nullptr, CWnd *pParent = nullptr);
		virtual ~CSvDmpParamDlg() override;
		// Данные диалогового окна

		enum { IDD = IDD_SVDMP_PARAM_DLG };

	protected:
	///	virtual void DoDataExchange(CDataExchange *pDX) override;   // поддержка DDX/DDV

		// Реализация
	protected:
		CEnterEdit m_EditBgnAddr;
		CEnterEdit m_EditEndAddr;
		CEnterEdit m_EditLength;
		int m_nBgnAddr;
		int m_nEndAddr;
		int m_nLength;
		CString m_strBgnAddr;
		CString m_strEndAddr;
		CString m_strLength;
		SvDmpParam *m_pParam;

		// Созданные функции схемы сообщений
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		LRESULT OnEditChange(WPARAM wParam, LPARAM lParam);
		afx_msg void OnEnKillfocusEditSvdmpBgnaddr();
		afx_msg void OnEnKillfocusEditSvdmpEndaddr();
		afx_msg void OnEnKillfocusEditSvdmpLength();
		DECLARE_MESSAGE_MAP()

};

class CLdDmpParamDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CLdDmpParamDlg)
		// Создание
	public:
		CLdDmpParamDlg(SvDmpParam *pParam = nullptr, CWnd *pParent = nullptr);
		virtual ~CLdDmpParamDlg() override;
		// Данные диалогового окна

		enum { IDD = IDD_LDDMP_PARAM_DLG };

	protected:
	///	virtual void DoDataExchange(CDataExchange *pDX) override;   // поддержка DDX/DDV

		// Реализация
	protected:
		CEnterEdit m_EditBgnAddr;
		CEnterEdit m_EditPG0;
		CEnterEdit m_EditPG1;
		int m_nBgnAddr;
		CString m_strBgnAddr;
		int m_nPg0, m_nPg1;
		SvDmpParam *m_pParam;

		// Созданные функции схемы сообщений
		virtual BOOL OnInitDialog() override;
		virtual void OnOK() override;
		LRESULT OnEditChange(WPARAM wParam, LPARAM lParam);
		afx_msg void OnEnKillfocusEditLddmpAddr();

		DECLARE_MESSAGE_MAP()

};

