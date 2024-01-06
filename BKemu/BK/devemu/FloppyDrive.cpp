#include "pch.h"
#include "SafeReleaseDefines.h"
#include "FloppyDrive.h"

const int CFloppyDrive::m_nSectorSizes[4] = { 128, 256, 512, 1024 };

CFloppyDrive::CFloppyDrive()
	: m_bReadOnly(false)
	, m_bTrackChanged(false)
	, m_nDataTrack(-1)
	, m_nDataSide(-1)
	, m_nDataPtr(0)
	, m_pData(nullptr)
	, m_pMarker(nullptr)
	  // геометрия дискеты
	, m_nTracks(FLOPPY_TRACKS + 3)          // количество дорожек
	, m_nSides(FLOPPY_SIDES)                // количество сторон
	, m_nSectorsPerTrk(FLOPPY_TRACK_LEN)    // количество секторов на дорожке
	, m_nSectorSize(FLOPPY_SECTOR_TYPE)     // тип размера сектора
{
	SetArrays();
}

CFloppyDrive::~CFloppyDrive()
{
}

bool CFloppyDrive::SetArrays()
{
	m_nTrackSize = static_cast<size_t>(m_nSectorsPerTrk) * m_nSectorSizes[m_nSectorSize];
	m_nRawTrackSize = FLOPPY_TRKLEN_DD;
	m_nRawMarkerSize = m_nRawTrackSize / 2;

	if (m_pData)
	{
		m_pData.reset();
	}

	if (m_pMarker)
	{
		m_pMarker.reset();
	}

	m_pData   = std::make_unique<uint8_t[]>(m_nRawTrackSize);
	m_pMarker = std::make_unique<uint8_t[]>(m_nRawMarkerSize);

	if (m_pData && m_pMarker)
	{
		memset(m_pData.get(), 0, m_nRawTrackSize);
		memset(m_pMarker.get(), 0, m_nRawMarkerSize);
		return true;
	}

	return false;
}

void CFloppyDrive::Reset()
{
	m_nDataTrack = m_nDataSide = -1;
	m_nDataPtr = 0;
}

bool CFloppyDrive::Attach(const fs::path &strFile, bool bExclusive)
{
	// если пытаемся приаттачить уже ранее приаттаченный этот же образ
	if (m_strFileName.cstr().CollateNoCase(strFile.c_str()) == 0)
	{
		return true; // то просто выйдем, как будто всё как надо сделали.
	}

	// Если какой-то другой образ подсоединён, сперва отсоединим
	Detach();
	UINT nOpenFlag = bExclusive ? CFile::shareDenyWrite : CFile::shareDenyNone;
	m_bReadOnly = false;
	bool bRes = !!m_File.Open(strFile.c_str(), CFile::modeReadWrite | nOpenFlag); // сперва для чтения-записи

	if (!bRes) // если не получилось
	{
		m_bReadOnly = true; // ставим защиту от записи
		bRes = !!m_File.Open(strFile.c_str(), CFile::modeRead | nOpenFlag); // то просто для чтения

		if (!bRes)
		{
			return false; // вообще никак не получилось открыть
		}
	}

	m_strFileName = strFile;
	Reset();
	return true;
}

void CFloppyDrive::Detach()
{
	FlushChanges();

	if (isAttached())
	{
		m_File.Close();
	}

	m_strFileName.clear();
	m_bReadOnly = false;
	Reset();
}

void CFloppyDrive::SetGeometry(int nTracks, int nSides, int nSectPerTrk, int nSectSz)
{
	m_nTracks = nTracks;
	m_nSides = nSides;
	m_nSectorsPerTrk = nSectPerTrk;
	m_nSectorSize = nSectSz & 3;
	SetArrays();
}

// Читаем дорожку из файла и заполняем m_data
void CFloppyDrive::PrepareTrack(int nTrack, int nSide)
{
	if (m_nDataTrack == nTrack && m_nDataSide == nSide)
	{
		return;
	}

	FlushChanges();
	m_nDataPtr = 0;
	m_nDataTrack = nTrack;
	m_nDataSide = nSide;
	// Предполагаем, что дорожка состоит из 10 секторов, 512 байтов каждый; смещение в файле === ((Track<<1)+SIDE)*m_nTrackSize
	LONGLONG foffset = (static_cast<LONGLONG>(nTrack) * m_nSides + nSide) * m_nTrackSize;
	auto data = std::make_unique<uint8_t[]>(m_nTrackSize);

	if (data)
	{
		memset(data.get(), 0, m_nTrackSize);

		if (isAttached())
		{
			if (foffset == m_File.Seek(foffset, CFile::SeekPosition::begin))
			{
				size_t count = m_File.Read(data.get(), UINT(m_nTrackSize));
				// TODO: Контроль ошибок чтения файла.
			}
		}

		// Заполняем массив m_data и массив m_marker промаркированными данными
		EncodeTrackData(data.get());
	}
}

void CFloppyDrive::FlushChanges()
{
	if (!m_bTrackChanged || !isAttached())
	{
		return;
	}

	// Декодируем данные дорожки из m_data
	auto data = std::make_unique<uint8_t[]>(m_nTrackSize);

	if (data)
	{
		bool decoded = DecodeTrackData(data.get());

		if (decoded)  // Записываем файл только если дорожка корректно декодировалась из raw data
		{
			// Предполагаем, что дорожка состоит из 10 секторов, 512 байтов каждый; смещение в файле === ((Track<<1)+SIDE)*m_nTrackSize
			size_t foffset = ((static_cast<size_t>(m_nDataTrack) * 2) + (m_nDataSide)) * m_nTrackSize;
			size_t foffset_end = foffset + m_nTrackSize;
			// Проверяем длину файла
			size_t currentFileSize = m_File.GetLength();
			uint8_t datafill[512];
			memset(datafill, 0, 512);

			while (currentFileSize < foffset_end)
			{
				// увеличиваем файл посекторно на нужное кол-во секторов
				UINT bytesToWrite = (foffset_end - currentFileSize) % 512;

				if (bytesToWrite == 0)
				{
					bytesToWrite = 512;
				}

				m_File.Write(datafill, bytesToWrite);
				// TODO: Проверка на ошибки записи
				currentFileSize += bytesToWrite;
			}

			// Сохраняем данные.
			m_File.Seek(foffset, CFile::SeekPosition::begin);
			// size_t dwBytesWritten =
			m_File.Write(data.get(), static_cast<UINT>(m_nTrackSize));
			// TODO: Проверка на ошибки записи
		}
		else
		{
			TRACE0("FDD::Ошибка декодирования дорожки.\n");
			// а иначе, просто не записываем, чтоб не попортить образ
		}
	}

	m_bTrackChanged = false;
}


// Заполняем массив m_data и массив m_marker промаркированными данными
void CFloppyDrive::EncodeTrackData(uint8_t *pSrc)
{
	memset(m_pMarker.get(), 0, m_nRawMarkerSize);
	size_t ptr = 0;
	int gap = 42;  // длина GAP4a + GAP1

	for (int sect = 0; sect < m_nSectorsPerTrk; ++sect)
	{
		// GAP
		for (int count = 0; count < gap; ++count)
		{
			m_pData[ptr++] = 0x4e;
		}

		// заголовок сектора
		for (int count = 0; count < 12; ++count)
		{
			m_pData[ptr++] = 0x00;
		}

		// маркер
		m_pMarker[ptr / 2] = true;  // Индексный маркер; начало подсчёта CRC
		uint8_t *pCrcPtr = &m_pData[ptr]; size_t nCrcPtr = ptr;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xfe;
		m_pData[ptr++] = LOBYTE(m_nDataTrack);
		m_pData[ptr++] = LOBYTE(m_nDataSide);
		m_pData[ptr++] = sect + 1;
		m_pData[ptr++] = m_nSectorSize; // Предполагается 512 байтов на сектор;
		// crc
		uint16_t crc = getCrc(pCrcPtr, ptr - nCrcPtr); // TODO: Подсчёт CRC
		m_pData[ptr++] = HIBYTE(crc);
		m_pData[ptr++] = LOBYTE(crc);

		// синхропоследовательность
		for (int count = 0; count < 22; ++count)
		{
			m_pData[ptr++] = 0x4e;
		}

		// заголовок данных
		for (int count = 0; count < 12; ++count)
		{
			m_pData[ptr++] = 0x00;
		}

		// маркер
		m_pMarker[ptr / 2] = true;  // Маркер данных; начало подсчёта CRC
		pCrcPtr = &m_pData[ptr]; nCrcPtr = ptr;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xa1;
		m_pData[ptr++] = 0xfb;
		// данные
		size_t nSectSize = m_nSectorSizes[m_nSectorSize];
		memcpy(m_pData.get() + ptr, pSrc, nSectSize);
		ptr += nSectSize;
		pSrc += nSectSize;
		// crc
		crc = getCrc(pCrcPtr, ptr - nCrcPtr); // TODO: Подсчёт CRC
		m_pData[ptr++] = HIBYTE(crc);
		m_pData[ptr++] = LOBYTE(crc); // CRC stub вот такое у нас пока ЦРЦ
		gap = 36;  // длина GAP3
	}

	// заполняем GAP4B до конца дорожки
	while (ptr < m_nRawTrackSize)
	{
		m_pData[ptr++] = 0x4e;
	}
}

// Декодирование данных дорожки из raw data
// pDest - массив m_nTrackSize байтов
// Возвращается: true - декодировано, false - ошибка декодирования
bool CFloppyDrive::DecodeTrackData(uint8_t *pDest) const
{
	uint8_t sectcyl, secthd, sectsec, sectno;
	int sectorsize;
	size_t ptr = 0;  // Смещение в массиве m_data

	for (;;)
	{
		while (ptr < m_nRawTrackSize && m_pData[ptr] == 0x4e)
		{
			ptr++;    // Пропускаем GAP1 или GAP3
		}

		if (ptr >= m_nRawTrackSize)
		{
			break;    // Конец дорожки или ошибка
		}

		while (ptr < m_nRawTrackSize && m_pData[ptr] == 0x00)
		{
			ptr++;    // Пропускаем синхропоследовательность
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		uint8_t *pCrcPtr = &m_pData[ptr]; size_t nCrcPtr = ptr;

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		if (m_pData[ptr++] != 0xfe)
		{
			TRACE0("FDD::индексный маркер не найден.\n");
			return false;    // Индексный маркер не найден
		}

		if (ptr < m_nRawTrackSize)
		{
			sectcyl = m_pData[ptr++];

			if (sectcyl != m_nDataTrack)
			{
				TRACE0("FDD::неверный номер дорожки.\n");
				return false; // неверный номер дорожки
			}
		}

		if (ptr < m_nRawTrackSize)
		{
			secthd = m_pData[ptr++];

			if (secthd != m_nDataSide)
			{
				TRACE0("FDD::неверный номер стороны.\n");
				return false; // неверный номер стороны
			}
		}

		if (ptr < m_nRawTrackSize)
		{
			sectsec = m_pData[ptr++];

			if (sectsec == 0 || sectsec > m_nSectorsPerTrk)
			{
				TRACE0("FDD::неверный номер сектора.\n");
				return false; // несуществующий номер сектора
			}
		}

		if (ptr < m_nRawTrackSize)
		{
			sectno = m_pData[ptr++];

			switch (sectno)
			{
				case 0:
					sectorsize = 128;
					break;

				case 1:
					sectorsize = 256;
					break;

				case 2:
					sectorsize = 512;
					break;

				case 3:
					sectorsize = 1024;
					break;

				default:
					TRACE0("FDD::неверный тип размера сектора.\n");
					return false;    // Что-то не так: неправильный размер сектора
			}
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		// crc
		uint16_t crc = getCrc(pCrcPtr, ptr - nCrcPtr);
		uint16_t crcRd = 0xffff;

		if (ptr < m_nRawTrackSize)
		{
			crcRd = (m_pData[ptr++]) << 8;
		}

		if (ptr < m_nRawTrackSize)
		{
			crcRd |= (m_pData[ptr++]);
		}

		if (crc != crcRd)
		{
			TRACE0("FDD::неверный CRC заголовка.\n");
			return false; // несуществующий номер сектора
		}

		while (ptr < m_nRawTrackSize && m_pData[ptr] == 0x4e)
		{
			ptr++;    // Пропускаем GAP2
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		while (ptr < m_nRawTrackSize && m_pData[ptr] == 0x00)
		{
			ptr++;    // Пропускаем синхропоследовательность
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		pCrcPtr = &m_pData[ptr]; nCrcPtr = ptr;

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr < m_nRawTrackSize && m_pData[ptr] == 0xa1)
		{
			ptr++;
		}

		if (ptr >= m_nRawTrackSize)
		{
			return false;    // Что-то не так
		}

		if (m_pData[ptr++] != 0xfb)
		{
			TRACE0("FDD::маркер данных не найден.\n");
			return false;    // Маркер данных не найден
		}

		// Копируем данные сектора, так мы можем пытаться интерпретировать пользовательский формат.
		// вообще почти любой нестандартный, включая интерлив.
		// исключение - нельзя задавать произвольные номера дорожки и стороны.
		size_t destptr = (static_cast<size_t>(sectsec) - 1) * sectorsize;
		// если в процессе вылезем за пределы массива-приёмника
		bool bBrk = false;

		if (destptr + sectorsize >= m_nTrackSize)
		{
			sectorsize -= int((destptr + sectorsize) - m_nTrackSize); // корректируем количество копируемых данных
			bBrk = true;
		}

		int ndd = 0;

		// но если и теперь в процессе вылезем за пределы массива m_data
		if (ptr + sectorsize >= m_nRawTrackSize)
		{
			ndd = sectorsize;
			sectorsize -= int((ptr + sectorsize) - m_nRawTrackSize); // то корректируем количество  копируемых данных
			ndd -= sectorsize; // а разницу надо будет заполнить нулями
		}

		// копируем
		memcpy(pDest + destptr, m_pData.get() + ptr, sectorsize);
		ptr += sectorsize;

		if (bBrk) // всё, конец дорожки
		{
			if (ndd) // ещё и вообще хреновый конец
			{
				memset(pDest + destptr + sectorsize, 0, ndd);
				return false;
			}

			break;
		}

		// crc
		crc = getCrc(pCrcPtr, ptr - nCrcPtr);
		crcRd = 0xffff;

		if (ptr < m_nRawTrackSize)
		{
			crcRd = (m_pData[ptr++]) << 8;
		}

		if (ptr < m_nRawTrackSize)
		{
			crcRd |= (m_pData[ptr++]);
		}

		if (crc != crcRd)
		{
			TRACE0("FDD::неверный CRC данных.\n");
			return false; // несуществующий номер сектора
		}
	}

	return true;
}

uint16_t CFloppyDrive::getCrc(uint8_t *ptr, size_t len) const
{
	int crc = 0xffff;

	while (len--)
	{
		uint8_t val = *ptr++;
		crc ^= val << 8;

		for (int i = 0; i < 8; ++i)
		{
			if ((crc <<= 1) & 0x10000)
			{
				crc ^= 0x1021;
			}
		}
	}

	return (crc & 0xffff);
}

LONGLONG CFloppyDrive::FDSeek(LONGLONG pos, UINT from)
{
	return m_File.Seek(pos, from);
}

UINT CFloppyDrive::FDRead(void *lpBuf, UINT nCount)
{
	return m_File.Read(lpBuf, nCount);
}

void CFloppyDrive::FDWrite(void *lpBuf, UINT nCount)
{
	m_File.Write(lpBuf, nCount);
}

void CFloppyDrive::MovePtr()
{
	m_nDataPtr += 2;

	if (m_nDataPtr >= m_nRawTrackSize)
	{
		m_nDataPtr = 0;
	}
}

void CFloppyDrive::WrData(uint16_t w)
{
	*(reinterpret_cast<uint16_t *>(&m_pData[m_nDataPtr])) = w;
	m_bTrackChanged = true;
}

uint16_t CFloppyDrive::RdData() const
{
	//return (m_pData[m_nDataPtr] << 8) | m_pData[m_nDataPtr + 1];
	uint16_t t = *(reinterpret_cast<uint16_t *>(&m_pData[m_nDataPtr]));
	return ((t & 0377) << 8) | ((t >> 8) & 0377);
}

