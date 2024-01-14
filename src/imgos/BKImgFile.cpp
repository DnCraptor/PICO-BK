#include "pch.h"
#include "BKImgFile.h"
extern "C" {
	#include "debug.h"
}

#pragma warning(disable:4996)

constexpr auto BLOCK_SIZE = 512;

CBKImgFile::CBKImgFile()
	: m_open(false)
	, m_nCylinders(83)
	, m_nHeads(2)
	, m_nSectors(10)
{
}

CBKImgFile::CBKImgFile(const fs::path &strName, const bool bWrite)
    : m_open(false)
	, m_nCylinders(83)
	, m_nHeads(2)
	, m_nSectors(10)
{
	Open(strName, bWrite);
}


CBKImgFile::~CBKImgFile()
{
	Close();
}

bool CBKImgFile::Open(const fs::path &pathName, const bool bWrite)
{
	DBGM_PRINT(("CBKImgFile::Open(%s, %d)", pathName.c_str(), bWrite));
	bool bNeedFDRaw = false;
	bool bFDRaw = false;
	DWORD dwRet;
	std::string strName = pathName.string();
    // открываем образ
	BYTE mode = bWrite ? (FA_WRITE | FA_OPEN_APPEND | FA_READ) : FA_READ;
    m_open = f_open(&m_f, strName.c_str(), mode) == FR_OK;
	if (m_open)
	{
		m_strName = pathName;
	}
	return m_open;
}

void CBKImgFile::Close()
{
	DBGM_PRINT(("CBKImgFile::Close() m_open: %d", m_open));
	if (m_open)
	{
		f_close(&m_f);
		m_open = false;
	}
}

void CBKImgFile::SetGeometry(const uint8_t c, const uint8_t h, const uint8_t s)
{
	if (c != 0xff)
	{
		m_nCylinders = c;
	}

	if (h != 0xff)
	{
		m_nHeads = h;
	}

	if (s != 0xff)
	{
		m_nSectors = s;
	}
}

CBKImgFile::CHS CBKImgFile::ConvertLBA(const UINT lba) const
{
	CHS ret;
	// превратить смещение в байтах в позицию в c:h:s;
	// поскольку формат строго фиксирован: c=80 h=2 s=10, а перемещения предполагаются только по границам секторов
	UINT t = static_cast<UINT>(m_nHeads) * static_cast<UINT>(m_nSectors);
	ret.c = lba / t;
	t = lba % t;
	ret.h = static_cast<uint8_t>(t / static_cast<UINT>(m_nSectors));
	ret.s = static_cast<uint8_t>(t % static_cast<UINT>(m_nSectors)) + 1;
	return ret;
}

UINT CBKImgFile::ConvertCHS(const CHS chs) const
{
	return ConvertCHS(chs.c, chs.h, chs.s);
}

UINT CBKImgFile::ConvertCHS(const uint8_t c, const uint8_t h, const uint8_t s) const
{
	UINT lba = (static_cast<UINT>(c) * static_cast<UINT>(m_nHeads) + static_cast<UINT>(h)) * static_cast<UINT>(m_nSectors) + static_cast<UINT>(s) - 1;
	return lba;
}
/***
bool CBKImgFile::IOOperation(const DWORD cmd, FD_READ_WRITE_PARAMS *rwp, void *buffer, const UINT numSectors) const
{
	UINT nSectorSize = 128 << rwp->size;
	auto pDataBuf = reinterpret_cast<uint8_t *>(buffer);
	DWORD dwRet;
	UINT cyl = rwp->cyl;
	BOOL b = DeviceIoControl(m_h, IOCTL_FDCMD_SEEK, &cyl, sizeof(cyl), nullptr, 0, &dwRet, nullptr);

	for (UINT n = 0; n < numSectors; ++n)
	{
		rwp->eot = rwp->sector + 1;
		rwp->phead = rwp->head;

		// посекторно читаем, потому что fdraw не умеет блочно с переходом на следующую дорожку
		if (!DeviceIoControl(m_h, cmd, rwp, sizeof(FD_READ_WRITE_PARAMS), pDataBuf, nSectorSize, &dwRet, nullptr))
		{
			return false;
		}

		pDataBuf += nSectorSize;

		if (++rwp->sector > m_nSectors)
		{
			rwp->sector = 1;

			if (++rwp->head >= m_nHeads)
			{
				rwp->head = 0;

				if (++rwp->cyl > m_nCylinders) // допускаем дорожки 0..81
				{
					return false;
				}

				cyl = rwp->cyl;
				b = DeviceIoControl(m_h, IOCTL_FDCMD_SEEK, &cyl, sizeof(cyl), nullptr, 0, &dwRet, nullptr);
			}
		}
	}

	return true;
}
***/

bool CBKImgFile::ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors)
{
	if (m_open)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
		if (FR_OK == f_lseek(&m_f, lba))
		{
			lba = numSectors * BLOCK_SIZE;
			UINT br;
			return FR_OK == f_read(&m_f, buffer, lba, &br);
		}
	}
	return false;
}

bool CBKImgFile::WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors)
{
	if (m_open)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
		if (FR_OK == f_lseek(&m_f, lba))
		{
			lba = numSectors * BLOCK_SIZE;
			UINT bw;
			return (FR_OK == f_write(&m_f, buffer, lba, &bw));
		}
	}
	return false;
}

bool CBKImgFile::ReadLBA(void *buffer, const UINT lba, const UINT numSectors)
{
	if (m_open)
	{
		if (FR_OK == f_lseek(&m_f, lba * BLOCK_SIZE))
		{
			UINT size = numSectors * BLOCK_SIZE;
			UINT br;
			return FR_OK == f_read(&m_f, buffer, size, &br);
		}
	}
	return false;
}

bool CBKImgFile::WriteLBA(void *buffer, const UINT lba, const UINT numSectors)
{
	if (m_open)
	{
		if (FR_OK == f_lseek(&m_f, lba * BLOCK_SIZE))
		{
			UINT size = numSectors * BLOCK_SIZE;
			UINT bw;
			return FR_OK == f_write(&m_f, buffer, size, &bw);
		}
	}
	return false;
}

long CBKImgFile::GetFileSize() const
{
	if (m_open)
	{
		return f_size(&m_f);
	}
	return -1;
}

bool CBKImgFile::SeekTo00()
{
	if (m_open)
	{
		return FR_OK == f_lseek(&m_f, 0);
	}
	return false;
}
