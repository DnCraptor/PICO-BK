// Board_10.cpp: implementation of the CMotherBoard_10 class.
//


#include "pch.h"
#include "Board_10.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

constexpr auto BRD_10_FOCAL_BNK = 10;
constexpr auto BRD_10_TESTS_BNK = 14;


CMotherBoard_10::CMotherBoard_10()
    = default;

CMotherBoard_10::~CMotherBoard_10()
    = default;


MSF_CONF CMotherBoard_10::GetConfiguration()
{
	return MSF_CONF::BK10;
}

bool CMotherBoard_10::InitMemoryModules()
{
	m_ConfBKModel.nROMPresent = 0;

	if (LoadRomModule(IDS_INI_BK10_RE2_017_MONITOR, BRD_10_MON10_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_MON10_BNK);
	}

	if (LoadRomModule(IDS_INI_BK10_RE2_084_FOCAL, BRD_10_FOCAL_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_FOCAL_BNK);
	}

	if (LoadRomModule(IDS_INI_BK10_RE2_019_MSTD, BRD_10_TESTS_BNK))
	{
		m_ConfBKModel.nROMPresent |= (3 << BRD_10_TESTS_BNK);
	}

	// и проинициализируем карту памяти
	MemoryManager();
	return true;
}


