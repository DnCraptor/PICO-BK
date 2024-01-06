#pragma once
#include "DisasmCtrl.h"
#include "EnterEditCtrl.h"
#include "DocPaneDlgViewBase.h"

class CDebugger;

class CDisasmView : public CDocPaneDlgViewBase
{
	///	DECLARE_DYNAMIC(CDisasmView)

		CDisasmCtrl     m_ListDisasm;
		CEnterEdit      m_EditAddr;
		CString         m_strDisasmAddr;

	public:
		enum { IDD = IDD_BKDISASM_DLG };
		CDisasmView();
		virtual ~CDisasmView() override;

		void            SetAddr(uint16_t addr);

		inline CDisasmCtrl *GetDisasmCtrl()
		{
			return &m_ListDisasm;
		}

		void            AttachDebugger(CDebugger *pDebugger);

		uint16_t        GetCursorAddr();
		uint16_t        GetBottomAddr();

	protected:
		void            SetDisasmAddr(uint16_t addr);
		afx_msg void    ChangeInstructionSet();

	///	virtual void    DoDataExchange(CDataExchange *pDX) override;    // DDX/DDV support
		afx_msg LRESULT HandleInitDialog(WPARAM wp, LPARAM lp);
		afx_msg void    OnSetFocus(CWnd *pOldWnd);
		afx_msg LRESULT OnDisasmTopAddressUpdate(WPARAM, LPARAM);
		afx_msg LRESULT OnDisasmTopAddressSet(WPARAM wp, LPARAM);
		afx_msg LRESULT OnDisasmCurrentAddressChange(WPARAM wp, LPARAM);
	///	DECLARE_MESSAGE_MAP()
};

