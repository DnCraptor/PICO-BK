// ExeptionHalt.cpp: implementation of the CExeptionHalt class.
//


#include "pch.h"
#include "ExceptionHalt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CExceptionHalt::CExceptionHalt(uint16_t addr, CString info)
	: m_addr(addr)
	, m_info(info)
{
	TRACE(_T("Exception Halt:: address: %06o, %s\n"), addr, info);
}

CExceptionHalt::~CExceptionHalt()
{
}
