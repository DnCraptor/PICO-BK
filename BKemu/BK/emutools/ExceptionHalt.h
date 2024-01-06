// ExeptionHalt.h: interface for the CExeptionHalt class.
//


#pragma once

class CExceptionHalt
{
	public:
		uint16_t        m_addr;
		CString         m_info;

		CExceptionHalt(uint16_t addr, CString info = _T("Unknown error"));
		virtual ~CExceptionHalt();
};

