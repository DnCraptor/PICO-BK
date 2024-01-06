#pragma once
#include "resource.h"
#include "MultiEditListCtrl.h"

#include <mutex>

enum class DUMP_DISPLAY_MODE : int
{
	DD_UNINIT = 0,
	DD_WORD_VIEW,
	DD_BYTE_VIEW
};

class CDumpListCtrl : public CMultiEditListCtrl
{
	///	DECLARE_DYNAMIC(CDumpListCtrl)

		DUMP_DISPLAY_MODE   m_DisplayMode;

		int                 m_nDumpAddr;    // адрес верхнего итема
		int                 m_nBottomAddr;  // адрес нижнего итема списка

		std::mutex          m_lock;

	public:
		CDumpListCtrl();
		virtual ~CDumpListCtrl() override;

		// переменные, чтобы можно было листать в ограниченном пространстве,
		// например в окне 16 кб.
		int                 m_nDmpWndAddr;      // начальный адрес окна дампа
		int                 m_nDmpWndLen;       // длина окна дампа
		int                 m_nDmpWndEndAddr;   // конечный адрес == m_nDmpWndAddr + m_nDmpWndLen


		enum DUMP_LISTW : int
		{
			W_ADDR = 0,
			W_COL1,
			W_COL2,
			W_COL3,
			W_COL4,
			W_ASCII
		};
		enum DUMP_LISTB : int
		{
			B_ADDR = 0,
			B_COL1,
			B_COL2,
			B_COL3,
			B_COL4,
			B_COL5,
			B_COL6,
			B_COL7,
			B_COL8,
			B_ASCII
		};

		void                SetAddress(const uint16_t addr);

		uint16_t            GetAddress() const;

		void                Init(DUMP_DISPLAY_MODE mode);
		void                RefreshItems(uint16_t nNewAddr = 0, bool bNewAddrIsValid = false);

		void                SetDisplayMode(DUMP_DISPLAY_MODE mode, bool bFill = true);

		inline const DUMP_DISPLAY_MODE GetDisplayMode()
		{
			return m_DisplayMode;
		}

		void                SetItemWithModifiedByte(uint8_t byte, int nItem, int nSubitem, bool bColored = true);
		void                AdjustRows();
		void                AdjustScroll();

	protected:
		void                SetHeader();
		void                SetEditAddress();
		void                ChangeAddr();
		void                InsertNewLine(const int row);

		void                AddrPlus(const int nValue);
		void                AddrMinus(const int nValue);


	protected:
		virtual BOOL PreCreateWindow(CREATESTRUCT &cs) override;
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	//	DECLARE_MESSAGE_MAP()
};

