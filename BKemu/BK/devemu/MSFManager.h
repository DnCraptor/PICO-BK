// MSFManager.h: interface for the CMSFManager class.
//


#pragma once

#include "MSF.h"
#include "FDDController.h"
#include "Config.h"

constexpr auto MSF_STATE_ID = 65536;
constexpr auto MSF_VERSION_MINIMAL = 19;
constexpr auto MSF_VERSION_CURRENT = 19;


class CMSFManager
{
		CFile               m_fileMSF;
		bool                m_bOpenForLoad;
		MSF_FILE_HEADER     m_header;

		CArray<MSF_BLOCK_INFO, MSF_BLOCK_INFO &> m_blocks;

		bool                Seek(int nOffset, UINT nFrom);
		bool                ReleaseFile();

		bool                CheckBlockSize(MSF_BLOCK_INFO *pbi, DWORD size);

	public:
		CMSFManager();
		virtual ~CMSFManager();

		bool                CheckFile(const fs::path &strPath, bool bSilent = false);
		bool                OpenFile(const fs::path &strPath, bool bLoad, bool bSilent = false);

		inline void         SetType(DWORD type)
		{
			m_header.type = type;
		}
		inline DWORD        GetType()
		{
			return m_header.type;
		}

		inline void         SetVersion(DWORD version)
		{
			m_header.version = version;
		}
		inline DWORD        GetVersion()
		{
			return m_header.version;
		}
		inline void         SetConfiguration(CONF_BKMODEL configuration)
		{
			m_header.configuration = static_cast<DWORD>(configuration);
		}
		inline CONF_BKMODEL     GetConfiguration()
		{
			return static_cast<CONF_BKMODEL>(m_header.configuration);
		}
		inline bool         IsLoad()
		{
			return m_bOpenForLoad;
		}
		inline bool         IsSave()
		{
			return !m_bOpenForLoad;
		}

		bool                FindBlock(DWORD blockType, MSF_BLOCK_INFO *pBi);
		bool                SetBlockHeader(MSF_BLOCK_INFO *pBi);
		bool                GetBlockHeader(MSF_BLOCK_INFO *pBi);
		bool                SetBlockData(uint8_t *pBuff, DWORD length);
		bool                GetBlockData(uint8_t *pBuff, DWORD length);

		bool                SetBlockPreview(HBITMAP hScreenshot);
		bool                GetBlockPreview(HBITMAP *hScreenshot, uint8_t **pBits);
		bool                SetBlockBaseMemory(uint8_t *pMemory);
		bool                GetBlockBaseMemory(uint8_t *pMemory);
		bool                SetBlockBaseMemory11M(uint8_t *pMemory);
		bool                GetBlockBaseMemory11M(uint8_t *pMemory);
		bool                SetBlockMemorySMK512(uint8_t *pMemory);
		bool                GetBlockMemorySMK512(uint8_t *pMemory);
		bool                SetBlockCPURegisters(MSF_CPU_REGISTERS *pCPURegisters);
		bool                GetBlockCPURegisters(MSF_CPU_REGISTERS *pCPURegisters);
		bool                SetBlockPortRegs(MSF_PORT_REGS *pPortRegs);
		bool                GetBlockPortRegs(MSF_PORT_REGS *pPortRegs);

		bool                GetBlockMemMap(BKMEMBank_t *MBType, ConfBKModel_t *CBKMType);
		bool                SetBlockMemMap(BKMEMBank_t *MBType, ConfBKModel_t *CBKMType);

		bool                GetBlockConfig();
		bool                SetBlockConfig();

		bool                GetBlockFrameData(MSF_FRAMEDATA *FDBlock);
		bool                SetBlockFrameData(MSF_FRAMEDATA *FDBlock);

		bool                SetBlockExt32Memory(DWORD nPage, uint8_t *pMemoryExt);
		bool                GetBlockExt32Memory(DWORD *pnPage, uint8_t *pMemoryExt);
		bool                SetBlockExt16Memory(uint8_t *pMemoryExt);
		bool                GetBlockExt16Memory(uint8_t *pMemoryExt);
		bool                SetBlockTape(uint8_t *pPackedWave, DWORD nBitsLength, DWORD nPackLength);
		bool                GetBlockTape(uint8_t *pPackedWave, DWORD *pnBitsLength, DWORD *pnPackLength);
};

