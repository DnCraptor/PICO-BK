#pragma once

#include "BKKbdButn.h"
#include "resource.h"
#include "Config.h"

// CBKVKBDView

class CBKVKBDView : public CDockablePane
{
		DECLARE_DYNAMIC(CBKVKBDView)

		CRgn        m_rgnRes;
		CBrush      m_br;
		CBKKbdButn* m_pKbdButn;
		UINT        m_nViewID;

		static const char LatShiftDigitTable[10];
		static const char RusShiftDigitTable[10];
		static const char RusAlphaBetTable[26];
		static const char RusAlphaBetTableShift[26];

	public:
		CBKVKBDView(UINT nID = IDB_BITMAP_SOFT);
		virtual ~CBKVKBDView() override;
		int             SetKeyboardView(UINT nID);
		bool            TranslateKey(int key, const bool bExtended, uint16_t *nScanCode, uint16_t *nInt) const;
		uint8_t         GetUniqueKeyNum(const uint16_t nScanCode) const;
		inline void     SetAR2Status(const bool b)
		{
			m_pKbdButn->SetAR2Status(b);
		}
		inline bool     GetAR2Status() const
		{
			return m_pKbdButn->GetAR2Status();
		}
		inline void     SetShiftStatus(const bool b)
		{
			m_pKbdButn->SetShiftStatus(b);
		}
		inline bool     GetShiftStatus() const
		{
			return m_pKbdButn->GetShiftStatus();
		}
		inline void     SetSUStatus(const bool b)
		{
			m_pKbdButn->SetSUStatus(b);
		}
		inline bool     GetSUStatus() const
		{
			return m_pKbdButn->GetSUStatus();
		}
		inline void     SetCapitalStatus(const bool b)
		{
			m_pKbdButn->SetCapitalStatus(b);
		}
		inline bool     GetCapitalStatus() const
		{
			return m_pKbdButn->GetCapitalStatus();
		}
		inline void     SetXLatStatus(const bool b)
		{
			m_pKbdButn->SetXLatStatus(b);
		}
		inline bool     GetXLatStatus() const
		{
			return m_pKbdButn->GetXLatStatus();
		}

		void            SetKeyboardStatus(const STATUS_FIELD pane, const bool set);
		bool            GetKeyboardStatus(const STATUS_FIELD pane) const;

		void            OutKeyboardStatus(const STATUS_FIELD pane) const;

	protected:
		int             CreateKeyboard();
		int             LatModeTranslation(const int key) const;
		int             RusModeTranslation(const int key) const;
		virtual void    OnAfterFloat() override;
		virtual void    OnAfterDockFromMiniFrame() override;

	protected:
		afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void    OnSize(UINT nType, int cx, int cy);
		afx_msg void    OnPaint();
		afx_msg BOOL    OnEraseBkgnd(CDC *pDC);
		afx_msg void    OnDestroy();
		DECLARE_MESSAGE_MAP()
};


