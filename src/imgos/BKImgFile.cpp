#include "pch.h"
#include "BKImgFile.h"
#include <cchar>

#pragma warning(disable:4996)

constexpr auto BLOCK_SIZE = 512;

CBKImgFile::CBKImgFile()
	: m_f(nullptr)
	, m_h(INVALID_HANDLE_VALUE)
	, m_nCylinders(83)
	, m_nHeads(2)
	, m_nSectors(10)
{
}

CBKImgFile::CBKImgFile(const fs::path &strName, const bool bWrite)
	: m_f(nullptr)
	, m_h(INVALID_HANDLE_VALUE)
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
	bool bNeedFDRaw = false;
	bool bFloppy = false; // флаг, true - обращение к реальному флопику, false - к образу
	bool bFDRaw = false;
	DWORD dwRet;
	std::string strName = pathName.string();

	// сперва проверим, что за имя входного файла
	if (strName.length() >= 4)
	{
		std::string st = strName.substr(0, 4);

		if (st == "\\\\.\\")   // если начинается с этого
		{
			bFloppy = true;     // то обращаемся к реальному дисководу
			st = strName.substr(4, 5);

			if (st == "fdraw") // а если ещё и нужен fdraw
			{
				// то проверим, установлен ли драйвер
				bNeedFDRaw = true;
				HANDLE hFD = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

				if (hFD != INVALID_HANDLE_VALUE) // да, что-то установлено
				{
					DWORD dwVersion = 0;
					DeviceIoControl(hFD, IOCTL_FDRAWCMD_GET_VERSION, nullptr, 0, &dwVersion, sizeof(dwVersion), &dwRet, nullptr);
					CloseHandle(hFD);

					if (dwVersion && (HIWORD(dwVersion) == HIWORD(FDRAWCMD_VERSION)))
					{
						bFDRaw = true;
						// если dwVersion == 0, то "fdrawcmd.sys не установлен, смотрите: http://simonowen.com/fdrawcmd/\n";
						// если версия не совпадает, то тоже плохо ("Установленный fdrawcmd.sys не совместим с этой программой.\n");
					}
				}
			}
		}
	}

	bool bRet = false;

	if (bFloppy)
	{
		if (bNeedFDRaw) // если нужен fdraw
		{
			if (bFDRaw) // и он установлен
			{
				// открываем реальное устройство
				m_h = CreateFile(pathName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

				if (m_h != INVALID_HANDLE_VALUE) // если оно открылось, т.е. физически лоступно
				{
					// устанавливаем параметры доступа
					int DISK_DATARATE = FD_RATE_250K;          // 2 is 250 kbit/sec
					DeviceIoControl(m_h, IOCTL_FD_SET_DATA_RATE, &DISK_DATARATE, sizeof(DISK_DATARATE), nullptr, 0, &dwRet, nullptr);
					DeviceIoControl(m_h, IOCTL_FD_RESET, nullptr, 0, nullptr, 0, &dwRet, nullptr);
					bRet = true;
				}
			}

			// если не установлен то ничего.
		}
		else
		{
			// если fdraw не нужен, то просто открываем как есть
			// открываем реальное устройство
			m_h = CreateFile(pathName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

			if (m_h != INVALID_HANDLE_VALUE) // если оно открылось, т.е. физически доступно
			{
				bRet = true;
			}
		}
	}
	else
	{
		// открываем образ
		std::string strMode = bWrite ? "r+b" : "rb";
		m_f = _wfopen(pathName.c_str(), strMode.c_str());

		if (m_f)
		{
			bRet = true;
		}
	}

	if (bRet)
	{
		m_strName = pathName;
	}

	return bRet;
}

void CBKImgFile::Close()
{
	if (m_f)
	{
		fclose(m_f);
		m_f = nullptr;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_h);
		m_h = INVALID_HANDLE_VALUE;
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

bool CBKImgFile::ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const
{
	if (m_f)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;

		if (0 == fseek(m_f, lba, SEEK_SET))
		{
			lba = numSectors * BLOCK_SIZE;
			return (lba == fread(buffer, 1, lba, m_f));
		}

		return false;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, head, cyl, head, sector, 2, sector, 0x0a, 0xff };
		return IOOperation(IOCTL_FDCMD_READ_DATA, &rwp, buffer, numSectors);
	}

	return false;
}

bool CBKImgFile::WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const
{
	if (m_f)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;

		if (0 == fseek(m_f, lba, SEEK_SET))
		{
			lba = numSectors * BLOCK_SIZE;
			return (lba == fwrite(buffer, 1, lba, m_f));
		}

		return false;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, head, cyl, head, sector, 2, sector, 0x0a, 0xff };
		return IOOperation(IOCTL_FDCMD_WRITE_DATA, &rwp, buffer, numSectors);
	}

	return false;
}

bool CBKImgFile::ReadLBA(void *buffer, const UINT lba, const UINT numSectors) const
{
	if (m_f)
	{
		if (0 == fseek(m_f, lba * BLOCK_SIZE, SEEK_SET))
		{
			UINT size = numSectors * BLOCK_SIZE;
			return (size == fread(buffer, 1, size, m_f));
		}

		return false;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		CHS chs = ConvertLBA(lba);
		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, chs.h, chs.c, chs.h, chs.s, 2, chs.s, 0x0a, 0xff };
		return IOOperation(IOCTL_FDCMD_READ_DATA, &rwp, buffer, numSectors);
	}

	return false;
}




bool CBKImgFile::WriteLBA(void *buffer, const UINT lba, const UINT numSectors) const
{
	if (m_f)
	{
		if (0 == fseek(m_f, lba * BLOCK_SIZE, SEEK_SET))
		{
			UINT size = numSectors * BLOCK_SIZE;
			return (size == fwrite(buffer, 1, size, m_f));
		}

		return false;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		CHS chs = ConvertLBA(lba);
		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, chs.h, chs.c, chs.h, chs.s, 2, chs.s, 0x0a, 0xff };
		return IOOperation(IOCTL_FDCMD_WRITE_DATA, &rwp, buffer, numSectors);
	}

	return false;
}

long CBKImgFile::GetFileSize() const
{
	if (m_f)
	{
// #ifdef TARGET_WINXP
//      FILE *f = _tfopen(m_strName.c_str(), _T("rb"));
//
//      if (f)
//      {
//          fseek(f, 0, SEEK_END);
//          long sz = ftell(f);
//          fclose(f);
//          return sz;
//      }
//
//      return -1;
// #else
//      // вот этого нет в Windows XP, оно возвращает -1 вместо размера.
//      // оказывается это древний баг в рантайме, который даже чинили
//      // но он всё равно просочился и на него ужа забили из-за прекращения
//      // поддержки WinXP
//      struct _stat stat_buf;
//      int rc = _wstat(m_strName.c_str(), &stat_buf);
//      return rc == 0 ? stat_buf.st_size : -1;
// #endif
		std::error_code ec;
		long rc = fs::file_size(m_strName, ec);
		return (ec) ? -1 : rc;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		return static_cast<long>(m_nCylinders) * static_cast<long>(m_nSectors) * static_cast<long>(m_nHeads) * BLOCK_SIZE;
	}

	return -1;
}

bool CBKImgFile::SeekTo00() const
{
	if (m_f)
	{
		return (0 == fseek(m_f, 0, SEEK_SET));
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		DWORD dwRet;
		int cyl = 0;
		return !!DeviceIoControl(m_h, IOCTL_FDCMD_SEEK, &cyl, sizeof(cyl), nullptr, 0, &dwRet, nullptr);
	}

	return false;
}

bool CBKImgFile::IsFileOpen() const
{
	if (m_f)
	{
		return true;
	}

	if (m_h != INVALID_HANDLE_VALUE)
	{
		return true;
	}

	return false;
}


