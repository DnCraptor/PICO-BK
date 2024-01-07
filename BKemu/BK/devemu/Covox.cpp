// Covox.cpp: implementation of the CCovox class.
//


#include "pch.h"
#include "Config.h"
#include "Covox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CCovox::CCovox()
{
	if (CreateFIRBuffers(FIR_LENGTH))
	{
		ReInit();
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}

CCovox::~CCovox()
    = default;

void CCovox::ReInit()
{
	double w0 = 2 * 11000.0 / double(g_Config.m_nSoundSampleRate);
	double w1 = 0.0;
///	int res = fir_linphase(m_nFirLength, w0, w1, FIR_FILTER::LOWPASS,
///	                       FIR_WINDOW::BLACKMAN_HARRIS, true, 0.0, m_pH.get());
}

void CCovox::SetData(uint16_t inVal)
{
	if (m_bEnableSound)
	{
		m_dLeftAcc  = double(LOBYTE(inVal)) / 256.0;
		m_dRightAcc = m_bStereo ? double(HIBYTE(inVal)) / 256.0 : m_dLeftAcc;
	}
	else
	{
		m_dLeftAcc = m_dRightAcc = 0.0;
	}
}

void CCovox::GetSample(sOneSample *pSm)
{
	SAMPLE_INT l = m_dLeftAcc;
	SAMPLE_INT r = m_dRightAcc;

	if (m_bDCOffset)
	{
		l = DCOffset(l, m_dAvgL, m_pdBufferL.get(), m_nBufferPosL);
		r = DCOffset(r, m_dAvgR, m_pdBufferR.get(), m_nBufferPosR);
	}

	pSm->s[OSL] = FIRFilter(l, m_pLeftBuf.get(), m_nLeftBufPos);
	pSm->s[OSR] = FIRFilter(r, m_pRightBuf.get(), m_nRightBufPos);
}
