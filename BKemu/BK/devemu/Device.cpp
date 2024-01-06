// Device.cpp: implementation of the CDevice class.
//


#include "pch.h"
#include "Device.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CDevice::CDevice()
	: m_pParent(nullptr)
	, m_tickCount(0)
{
}

CDevice::~CDevice()
    = default;


void CDevice::Reset()
{
	m_tickCount = 0;
	OnReset();
}


void CDevice::NextTick()
{
	m_tickCount++;
}
