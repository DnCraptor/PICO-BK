#pragma once

#include "BKOscScr.h"

// COscillatorlView

class COscillatorlView : public CDockablePane
{
		DECLARE_DYNAMIC(COscillatorlView)

		CONF_OSCILLOSCOPE_RENDER m_nRenderType;
		std::unique_ptr<CBKOscScr> m_pBKOSC;

	public:
		COscillatorlView();
		virtual ~COscillatorlView() override;

		inline CBKOscScr *GetobjPtr()
		{
			return m_pBKOSC.get();
		}
		inline void     SetBuffer(int buffer_len_in_samples)
		{
			m_pBKOSC->SetBuffer(buffer_len_in_samples);
		}
		inline void     FillBuffer(SAMPLE_INT *buffer)
		{
			m_pBKOSC->FillBuffer(buffer);
		}
		bool            ReCreateOSC();

	protected:
		void            DeleteOSC();
		int             CreateOSC();
		virtual void    OnAfterFloat() override;
		virtual void    OnAfterDockFromMiniFrame() override;
		virtual void    AdjustLayout() override;

	protected:
		afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void    OnSize(UINT nType, int cx, int cy);
		afx_msg void    OnPaint();
		afx_msg BOOL    OnEraseBkgnd(CDC *pDC);
		afx_msg void    OnDestroy();
		DECLARE_MESSAGE_MAP()
};


