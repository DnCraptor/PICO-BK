// Tape.cpp: implementation of the CTape class.
//


#include "pch.h"
#include "Tape.h"

#include "resource.h"
#include "Config.h"
#include "MSFManager.h"
#include "BKMessageBox.h"

///#include "libdspl-2.0.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


constexpr auto RECORD_BUFFER = (4 * 1024 * 1024);
constexpr auto RECORD_GROW = (2 * 1024 * 1024);
constexpr uint16_t BAD_CODE = 65535;
constexpr auto RESET_CODE = 256;
constexpr auto PREFIX = 256;
constexpr auto CODE = 257;
constexpr auto BAD_LENGTH = -1;


/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CTape::CTape()
	: m_WorkingWFX{ WAVE_FORMAT_PCM,            /*wFormatTag;      format type */
	BUFFER_CHANNELS,            /*nChannels;       number of channels (i.e. mono, stereo...) */
	DEFAULT_SOUND_SAMPLE_RATE,  /*nSamplesPerSec;  sample rate */
	0,                          /*nAvgBytesPerSec; for buffer estimation */
	0,                          /*nBlockAlign;     block size of data */
	SAMPLE_INT_BPS,             /*wBitsPerSample;  number of bits per sample of mono data */
	0 }                         /*cbSize;          the count in bytes of the size of */
, m_pWave(nullptr)
, m_nWaveLength(0)
, m_nPlayPos(0)
, m_pRecord(nullptr)
, m_nRecordPos(0)
, m_nRecordLength(0)
, m_bAutoBeginRecord(false)
, m_bAutoEndRecord(false)
, m_pBin(nullptr)
, m_nPos(0)
, m_nAverage(0)
, m_nAvgLength(0)
, m_dAvgLength(0.0)
, m_bInverse(false)
, m_nScanTableSize(0)
, m_bRecord(false)
, m_bPlay(false)
, m_bWaveLoaded(false)
{
	SetWaveParam(DEFAULT_SOUND_SAMPLE_RATE, BUFFER_CHANNELS);
}

CTape::~CTape()
{
	SAFE_DELETE_MEMORY(m_pWave);
	SAFE_DELETE_MEMORY(m_pRecord);
	// очищаем таблицы
	ClearTables();
}


void CTape::ClearTables()
{
	for (auto &y : m_pScanTable)
	{
		y.clear();
	}
}


void CTape::SetWaveParam(int nWorkingSSR /*= DEFAULT_SOUND_SAMPLE_RATE*/, int nWorkingChn /*= BUFFER_CHANNELS*/)
{
	m_WorkingWFX.nSamplesPerSec = nWorkingSSR;
	m_WorkingWFX.nChannels = nWorkingChn;
	m_WorkingWFX.nBlockAlign = m_WorkingWFX.nChannels * SAMPLE_INT_SIZE;
	m_WorkingWFX.nAvgBytesPerSec = m_WorkingWFX.nSamplesPerSec * m_WorkingWFX.nBlockAlign;
}


bool CTape::LoadWaveFile(const fs::path &strPath)
{
	bool bRet = false;
	CFile waveFile;
	WAVEFORMATEX wfx;
	DataHeader dataHeader;

	if (waveFile.Open(strPath.c_str(), CFile::modeRead)) // если файл открылся.
	{
		WaveHeader waveHeader;

		if (waveFile.Read(&waveHeader, sizeof(WaveHeader)) == sizeof(WaveHeader)) // если заголовок прочёлся
		{
			if (waveHeader.riffTag == RIFF_TAG && waveHeader.fmtTag == FMT_TAG) // если заголовок тот, что нам нужен
			{
				if (waveFile.Read(&wfx, waveHeader.fmtSize) == waveHeader.fmtSize) // если информация о формате прочиталась
				{
					wfx.cbSize = 0; // обнуляем поле доп. информации, мы всё равно не умеем её использовать

					if (wfx.wFormatTag == WAVE_FORMAT_PCM) // если формат WAV тот, что нам нужен
					{
						for (;;) // ищем нужный блок данных
						{
							if (waveFile.Read(&dataHeader, sizeof(DataHeader)) == sizeof(DataHeader)) // если заголовок прочёлся
							{
								if (dataHeader.dataTag == DATA_TAG) // если блок тот, что нам нужен
								{
									bRet = true; // всё, всё что нужно нашли
									break; // выходим
								}

	///							waveFile.Seek(dataHeader.dataSize, CFile::SeekPosition::current); // переходим к следующему блоку.
							}
							else
							{
								break;  // если прочлось не то, что нужно, то скорее всего уже конец файла
							}
						}
					}
				}
			}
		}
	}

	if (bRet)
	{
		/*
		тут нужна конвертация из любого формата wav WAVE_FORMAT_PCM, в m_WorkingWFX.nSamplesPerSec, fp64, 2 канала

		для WAVE_FORMAT_PCM данные могут быть только 8 или 16 бит,
		каналов 1 или 2 по стандарту, но никто не мешает сделать и больше
		частота дискретизации 8000, 11025, 22050, 44100, но никто не мешает сделать и больше
		*/
		m_nWaveLength = 0;
		auto inBuf = std::vector<uint8_t>(dataHeader.dataSize); // входной буфер в байтах

		if (inBuf.data())
		{
			if (waveFile.Read(inBuf.data(), dataHeader.dataSize) == dataHeader.dataSize)
			{
				m_nWaveLength = ConvertSamples(wfx, inBuf.data(), dataHeader.dataSize);
			}
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}

		waveFile.Close();
		CalculateAverage();
	}

	return bRet;
}

/*
преобразование массива m_pWave из частоты nSrcSSR в nDstSSR
Цифровое. Не подходит для музыки.
Правильный ресемплинг с помощью БПФ превращает цифровой звук в
полное непотребство.
*/
void CTape::ResampleBuffer(int nSrcSSR, int nDstSSR)
{
	if (nSrcSSR == nDstSSR)
	{
		return;
	}

	SAMPLE_INT *pOldWave = m_pWave;
	size_t nOldWaveLen = m_nWaveLength;
	// формируем параметры входного wave
	SetWaveParam(nSrcSSR);
	WAVEFORMATEX wfx = m_WorkingWFX;
	// формируем параметры результирующего wave
	SetWaveParam(nDstSSR);
	m_nWaveLength = ConvertSamples(wfx, pOldWave, nOldWaveLen * wfx.nBlockAlign);
	SAFE_DELETE_MEMORY(pOldWave);
}

static size_t max(size_t a, size_t b) {
	return a > b? a : b;
}

/*
Конвертирование сэмплов из одной частоты в другую.
Вход: wfx_in - параметры входного wave.
inBuf - буфер кусочка входных данных
nBufSize - размер буфера в байтах
результирующий массив формируется в m_pWave
выход - количество сэмплов, в ресемплированном массиве

m_nWaveMaxLen - текущий размер буфера m_pWave, при необходимости - увеличивается
*/
size_t CTape::ConvertSamples(WAVEFORMATEX wfx_in, void *inBuf, size_t nBufSize)
{
	size_t nSrcSamples = nBufSize / wfx_in.nBlockAlign; // сколько сэмплов в буфере
	auto inBuf8 = reinterpret_cast<uint8_t *>(inBuf); // буфер для 8 битного звука
	auto inBuf16 = reinterpret_cast<SAMPLE_IO *>(inBuf); // буфер для 16 битного звука
	auto inBufFF = reinterpret_cast<SAMPLE_INT *>(inBuf); // буфер для внутреннего звука в плавающем формате

	if (m_WorkingWFX.nSamplesPerSec == wfx_in.nSamplesPerSec)
	{
		// если частота дискретизации та же самая, то надо просто скопировать данные.
		// и преобразовать их в fp64
		if (!AllocWaveBuffer(nSrcSamples))
		{
			return 0;
		}

		size_t nWavePos = 0;

		for (size_t s = 0; s < nSrcSamples; ++s)
		{
			for (WORD channel = 0; channel < wfx_in.nChannels; ++channel)
			{
				if (channel < m_WorkingWFX.nChannels)
				{
					SAMPLE_INT sample;

					switch (wfx_in.wBitsPerSample)
					{
						case 8:
							sample = (SAMPLE_INT(uint64_t(inBuf8[s * wfx_in.nChannels + channel]) << 8) - FLOAT_BASE) / FLOAT_BASE; // unsigned 8 to signed 16
							break;

						case 16:
							sample = SAMPLE_INT(inBuf16[s * wfx_in.nChannels + channel]) / FLOAT_BASE;
							break;

						case SAMPLE_INT_BPS:
							sample = inBufFF[s * wfx_in.nChannels + channel];
							break;

						// в WAVE_FORMAT_PCM может быть только 8 и 16 бит и 1 или 2 канала
						// в WAVE_FORMAT_EXTENSIBLE может быть ещё и 24 и 32-х битный звук и много каналов
						// вот как-то нафиг, неохота возиться с этим.
						default:
							sample = 0;
					}

					m_pWave[nWavePos * m_WorkingWFX.nChannels + channel] = sample;
				}
			}

			if (wfx_in.nChannels == 1)
			{
				for (WORD channel = 1; channel < m_WorkingWFX.nChannels; ++channel)
				{
					m_pWave[nWavePos * m_WorkingWFX.nChannels + channel] = m_pWave[nWavePos * m_WorkingWFX.nChannels + 0];
				}
			}

			nWavePos++;
		}

		return nWavePos;
	}

	//сперва надо разбить входной вав на отдельные каналы
	std::vector<double *> src(wfx_in.nChannels, nullptr);
	std::vector<double *> dst(wfx_in.nChannels, nullptr);
	size_t nDstSamples = 0;
	bool bNoMem = false;

	for (auto &channel : src)
	{
		channel = new double[nSrcSamples];

		if (!channel)
		{
			bNoMem = true;
		}
	}

	if (!bNoMem)
	{
		for (size_t s = 0; s < nSrcSamples; ++s)
		{
			for (WORD channel = 0; channel < wfx_in.nChannels; ++channel)
			{
				SAMPLE_INT sample;

				switch (wfx_in.wBitsPerSample)
				{
					case 8:
						sample = (SAMPLE_INT(uint64_t(inBuf8[s * wfx_in.nChannels + channel]) << 8) - FLOAT_BASE) / FLOAT_BASE; // unsigned 8 to signed 16
						break;

					case 16:
						sample = SAMPLE_INT(inBuf16[s * wfx_in.nChannels + channel]) / FLOAT_BASE;
						break;

					case SAMPLE_INT_BPS:
						sample = inBufFF[s * wfx_in.nChannels + channel];
						break;

					// в WAVE_FORMAT_PCM может быть только 8 и 16 бит и 1 или 2 канала
					// в WAVE_FORMAT_EXTENSIBLE может быть ещё и 24 и 32-х битный звук и много каналов
					// вот как-то нафиг, неохота возиться с этим.
					default:
						sample = 0;
				}

				src[channel][s] = sample;
			}
		}

		// затем их по отдельности обработать
		for (WORD channel = 0; channel < wfx_in.nChannels; ++channel)
		{
			int ny = 0;
			int res = 1; /// farrow_lagrange(src[channel], nSrcSamples, m_WorkingWFX.nSamplesPerSec, wfx_in.nSamplesPerSec, 0.5,
			                ///          &dst[channel], ny);
//          int res = farrow_spline(src[channel], nSrcSamples, m_WorkingWFX.nSamplesPerSec, wfx_in.nSamplesPerSec, 0.5,
//                                  &dst[channel], nOutlen);

			if (res)
			{
				nDstSamples = 0;
				goto err_exit;
			}

			nDstSamples = max(nDstSamples, static_cast<size_t>(ny));
		}

		// теперь собираем обратно в один вав
		if (AllocWaveBuffer(nDstSamples))
		{
			for (size_t s = 0; s < nDstSamples; ++s)
			{
				for (WORD channel = 0; channel < wfx_in.nChannels; ++channel)
				{
					if (channel < m_WorkingWFX.nChannels)
					{
						m_pWave[s * m_WorkingWFX.nChannels + channel] = dst[channel][s];
					}

					if (wfx_in.nChannels == 1)
					{
						for (WORD channel = 1; channel < m_WorkingWFX.nChannels; ++channel)
						{
							m_pWave[s * m_WorkingWFX.nChannels + channel] = dst[0][s];
						}
					}
				}
			}
		}
	}

err_exit:

	for (WORD channel = 0; channel < wfx_in.nChannels; ++channel)
	{
		SAFE_DELETE_ARRAY(src[channel]);
		SAFE_DELETE_ARRAY(dst[channel]);
	}

	return nDstSamples;
}



/*
Расчёт среднего уровня массива. Это будет среднее арифметическое.
Так считать дольше, но корректнее.
*/
void CTape::CalculateAverage()
{
	double avg = 0.0;

	for (size_t i = 0; i < m_nWaveLength; ++i)
	{
		avg += GetCurrentSampleMono(m_pWave, i);
	}

	m_nAverage = SAMPLE_INT(avg / m_nWaveLength);
}

#define MAXINT32 0xFFFFFFFF

bool CTape::LoadTmpFile(const fs::path &strPath)
{
	// внутри TMP файла - данные типа SAMPLE_INT с частотой 44100
	CFile tmpFile;

	if (!tmpFile.Open(strPath.c_str(), CFile::modeRead))
	{
		return false;
	}

	size_t dataSize = tmpFile.GetLength();

	if (dataSize > MAXINT32)
	{
		return false;
	}

	m_nWaveLength = 0;
	auto inBuf = std::vector<uint8_t>(dataSize); // входной буфер в байтах

	if (inBuf.data())
	{
		// формируем параметры входного wave
		constexpr auto BlockAlign = (SAMPLE_INT_BPS >> 3) * BUFFER_CHANNELS;
		WAVEFORMATEX wfx =
		{
			WAVE_FORMAT_PCM,                            /*wFormatTag;      format type */
			BUFFER_CHANNELS,                            /*nChannels;       number of channels (i.e. mono, stereo...) */
			DEFAULT_SOUND_SAMPLE_RATE,                  /*nSamplesPerSec;  sample rate */
			DEFAULT_SOUND_SAMPLE_RATE * BlockAlign,     /*nAvgBytesPerSec; for buffer estimation */
			BlockAlign,                                 /*nBlockAlign;     block size of data */
			SAMPLE_INT_BPS,                             /*wBitsPerSample;  number of bits per sample of mono data */
			0                                           /*cbSize;          the count in bytes of the size of */
			/*                 extra information (after cbSize) */
		};

		if (tmpFile.Read(inBuf.data(), dataSize) == dataSize)
		{
			m_nWaveLength = ConvertSamples(wfx, inBuf.data(), dataSize);
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	tmpFile.Close();
	// Calculate average
	CalculateAverage();
	return true;
}

// такой недостаток - сохранение всегда с текущей частотой дискретизации.
// а нужно добавить опцию, с какой частотой сохранять.
// ибо не всегда нужно, цифровому ваве вполне достаточно 44100Гц.
bool CTape::SaveWaveFile(const fs::path &strPath) const
{
	CFile waveFile;

	if (!waveFile.Open(strPath.c_str(), CFile::modeCreate | CFile::modeReadWrite))
	{
		return false;
	}

	WAVEFORMATEX wfx = m_WorkingWFX;
	wfx.wBitsPerSample = SAMPLE_IO_BPS;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels; // количество байтов на сэмпл
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; // сколько байтов в секунду проиграется
	DataHeader dataHeader
	{
		DATA_TAG,
		static_cast<DWORD>(m_nWaveLength) *wfx.nBlockAlign
	};
	WaveHeader waveHeader
	{
		RIFF_TAG,
		sizeof(WaveHeader) + sizeof(WAVEFORMATEX) + sizeof(DataHeader) + dataHeader.dataSize,
		WAVE_TAG,
		FMT_TAG,
		sizeof(WAVEFORMATEX)
	};
	waveFile.Write(&waveHeader, sizeof(WaveHeader));
	waveFile.Write(&wfx, waveHeader.fmtSize);
	waveFile.Write(&dataHeader, sizeof(DataHeader));
	// сохраним ваве в формате 16 бит 2 канала, с заданной частотой дискретизации
	auto pTmpBuf = std::vector<SAMPLE_IO>(size_t(wfx.nSamplesPerSec) * wfx.nChannels);

	if (pTmpBuf.data())
	{
		size_t nRest = m_nWaveLength; // сколько сэмплов преобразовать
		size_t nWavePos = 0; // позиция в буфере m_pWave

		do
		{
			size_t nChunk = nRest > static_cast<size_t>(wfx.nSamplesPerSec) ? wfx.nSamplesPerSec : nRest; // сколько сэмплов возьмём
			nRest -= nChunk;

			for (size_t i = 0; i < nChunk; ++i)
			{
				for (WORD j = 0; j < wfx.nChannels; ++j)
				{
					pTmpBuf[i * wfx.nChannels + j] = SAMPLE_IO(m_pWave[nWavePos * wfx.nChannels + j] * FLOAT_BASE);
				}

				nWavePos++;
			}

			waveFile.Write(pTmpBuf.data(), static_cast<UINT>(nChunk) * wfx.nBlockAlign);
		}
		while (nRest > 0);
	}

	return true;
}

// упаковываем каждый сэмпл в битовый массив. 1 сэмпл - 1 бит
void CTape::PackBits(uint8_t *pPackBits, int nPackBitsLength)
{
	int pos = 0; // Текущая позиция упакованного байта
	uint8_t mask = 1; // Циклическая битовая маска
	ZeroMemory(pPackBits, nPackBitsLength);  // обнуляем массив, чтобы не выставлять вместе с 1 ещё и 0
	// преобразуем частоту дискретизации из текущей в 44100 (по умолчанию)
	ResampleBuffer(m_WorkingWFX.nSamplesPerSec, DEFAULT_SOUND_SAMPLE_RATE);

	for (size_t i = 0; i < m_nWaveLength; ++i)
	{
		// Берём текущий сэмпл
		if (GetCurrentSampleMono(m_pWave, i) >= m_nAverage) // Если больше нуля, то выставляем 1, иначе пропускаем
		{
			pPackBits[pos] |= mask;
		}

		mask <<= 1; // сдвигаем маску на 1 влево

		if (!mask) // если маска опустела,
		{
			// то байт заполнился, надо дальше заново установить маску и переходить к следующему байту
			mask = 1;
			pos++;
		}
	}
}


void CTape::UnpackBits(uint8_t *pPackBits, int nPackBitsLength)
{
	// Выделяем память для распакованных данных, тут уже должно быть 2 канала
	AllocWaveBuffer(static_cast<size_t>(nPackBitsLength) * 8);
	m_nWaveLength = 0;

	for (int i = 0; i < nPackBitsLength; ++i)
	{
		uint8_t mask = 1;

		// Разжимаем каждый байт в 8 сэмплов
		for (int b = 0; b < 8; ++b)
		{
			// из бита 1 делаем байт MAX_SAMPLE, из бита 0 делаем байт MIN_SAMPLE
			SAMPLE_INT sample = ((pPackBits[i] & mask) ? MAX_SAMPLE : MIN_SAMPLE) / FLOAT_BASE;
			SetCurrentSampleMono(m_pWave, m_nWaveLength, sample);
			mask <<= 1; // сдвигаем маску
			m_nWaveLength++;
		}
	}

	// преобразуем частоту дискретизации из 44100 (по умолчанию) в текущую
	ResampleBuffer(DEFAULT_SOUND_SAMPLE_RATE, m_WorkingWFX.nSamplesPerSec);
}


int CTape::PackLZW_Fast(uint8_t *pPackBits, int nPackBitsLength, uint16_t *pPackLZW)
{
	/*
	pPackBits[0] = 'A';
	pPackBits[1] = 'B';
	pPackBits[2] = 'C';
	pPackBits[3] = 'A';
	pPackBits[4] = 'B';
	pPackBits[5] = 'C';
	pPackBits[6] = 'A';
	pPackBits[7] = 'B';
	pPackBits[8] = 'C';
	pPackBits[9] = 'A';
	pPackBits[10] = 'B';
	pPackBits[11] = 'C';
	pPackBits[12] = 'A';
	pPackBits[13] = 'B';
	pPackBits[14] = 'C';
	pPackBits[15] = 'A';
	pPackBits[16] = 'B';
	pPackBits[17] = 'C';
	pPackBits[18] = 'A';
	pPackBits[19] = 'B';
	pPackBits[20] = 'C';
	pPackBits[21] = 'A';
	pPackBits[22] = 'B';
	pPackBits[23] = 'C';
	pPackBits[24] = 'A';
	pPackBits[25] = 'B';
	pPackBits[26] = 'C';
	pPackBits[27] = 'A';
	pPackBits[28] = 'B';
	pPackBits[29] = 'C';
	*/
	// Инициализируем таблицы
	InitTables();
	int nPackLZWLength = 0;
	uint16_t last_code = BAD_CODE;
	int read_pos;

	for (read_pos = 0; read_pos < nPackBitsLength; ++read_pos)
	{
		int code = TableLookup_Fast(pPackBits, last_code, read_pos);  // Берём текущую последовательность

		if (code == BAD_CODE || read_pos == (nPackBitsLength - 1))
		{
			// Если нет в таблице, или уже конец
			pPackLZW[nPackLZWLength++] = last_code;  // Упаковка: последовательность - 1 байт
			AddWord_Fast(pPackBits, last_code, read_pos);  // Добавление: последовательность в таблицу
			last_code = pPackBits[read_pos];
		}
		else
		{
			last_code = code;
		}
	}

	pPackLZW[nPackLZWLength++] = pPackBits[read_pos - 1]; // Пакуем последний байт как есть
	return nPackLZWLength; // Возвращаем длину сжатого массива
}


void CTape::UnpackLZW_Fast(uint8_t *pPackBits, int nPackBitsLength, uint16_t *pPackLZW, int nPackLZWLength)
{
	// Инициализируем таблицы
	InitTables();
	// Очищаем будущий распакованный массив. я так понимаю, что nPackBitsLength - это размер будущих распакованных данных
	ZeroMemory(pPackBits, nPackBitsLength);
	nPackBitsLength = 0;
	int unpack_pos = 0;
	/*
	m_pPackLZW[0] = 'A';
	m_pPackLZW[1] = 'B';
	m_pPackLZW[2] = 'C';
	m_pPackLZW[3] = 257;
	m_pPackLZW[4] = 259;
	m_pPackLZW[5] = 258;
	m_pPackLZW[6] = 260;
	m_pPackLZW[7] = 263;
	m_pPackLZW[8] = 262;
	m_pPackLZW[9] = 265;
	m_pPackLZW[10] = 261;
	m_pPackLZW[11] = 267;
	*/
	uint16_t last_code = BAD_CODE;

	for (int read_pos = 0; read_pos < nPackLZWLength; ++read_pos)
	{
		if (pPackLZW[read_pos] < 256) // Если текущий код < 256, распакуем его как есть
		{
			pPackBits[nPackBitsLength++] = uint8_t(pPackLZW[read_pos]);
		}
		else
		{
			// Иначе создаём таблицу от текущей позиции до позиции этого кода
			while (unpack_pos < nPackBitsLength)
			{
				int code = TableLookup_Fast(pPackBits, last_code, unpack_pos);

				if (code == BAD_CODE)
				{
					// Если нет текущей последовательности в таблице, добавим её
					AddWord_Fast(pPackBits, last_code, unpack_pos);
					last_code = pPackBits[unpack_pos];
				}
				else
				{
					last_code = code;
				}

				unpack_pos++;
			}

			if (!UnpackWord(pPackBits, nPackBitsLength, pPackLZW[read_pos]))  // Пробуем распаковать текущий код
			{
				// Если нет в таблице (случай XиX)
				int first_pos = nPackBitsLength;
				UnpackWord(pPackBits, nPackBitsLength, last_code);  // Распаковываем последний известный код
				pPackBits[nPackBitsLength++] = TableLookup_Fast(pPackBits, BAD_CODE, first_pos);  // Распаковываем последний известный код первого байта
				AddWord_Fast(pPackBits, last_code, unpack_pos);  // Добавление: Xпоследовательность + X в таблицу
				last_code = BAD_CODE;
			}
		}
	}
}


void CTape::InitTables()
{
	for (auto &y : m_pScanTable)
	{
		y.assign(258, BAD_CODE);
	}

	for (int y = 0; y < 256; ++y)
	{
		m_pScanTable[y][CODE] = y;
	}

	m_nScanTableSize = CODE;
}

int CTape::TableLookup_Fast(uint8_t *pPackBits, uint16_t prefix, int end_pos) const
{
	if (prefix == BAD_CODE)
	{
		return pPackBits[end_pos];
	}

	return m_pScanTable[prefix][pPackBits[end_pos]];
}


void CTape::AddWord_Fast(uint8_t *pPackBits, uint16_t prefix, int end_pos)
{
	// если размер таблицы превысил все допустимые рамки
	if (m_nScanTableSize >= MAX_TABLE_SIZE)
	{
		// обнулим его, и начнём заполнять сначала
		InitTables();
		return;
	}

	m_pScanTable[prefix][pPackBits[end_pos]] = m_nScanTableSize;
	m_pScanTable[m_nScanTableSize][PREFIX] = prefix;
	m_pScanTable[m_nScanTableSize][CODE] = pPackBits[end_pos];
	m_nScanTableSize++;
}


bool CTape::UnpackWord(uint8_t *pPackBits, int &nPackBitsLength, uint16_t code) const
{
	if (code == BAD_CODE)
	{
		return false;
	}

	if (code < 256)
	{
		pPackBits[nPackBitsLength++] = LOBYTE(code);
		return true;
	}

	if (!UnpackWord(pPackBits, nPackBitsLength, m_pScanTable[code][PREFIX]))
	{
		return false;
	}

	pPackBits[nPackBitsLength++] = (uint8_t)m_pScanTable[code][CODE];
	return true;
}


bool CTape::LoadMSFFile(const fs::path &strPath, bool bSilent)
{
	CMSFManager msf;

	if (!msf.OpenFile(strPath.c_str(), true, bSilent))
	{
		return false;
	}

	DWORD nPackBitsLength, nPackLZWLength;

	if (!msf.GetBlockTape(nullptr, &nPackBitsLength, &nPackLZWLength))
	{
		return false;
	}

	bool bRet = false;
	auto pPackBits = std::vector<uint8_t>(nPackBitsLength);
	auto pPackLZW = std::vector<uint16_t>(nPackLZWLength/2);

	if (pPackBits.data() && pPackLZW.data())
	{
		bRet = msf.GetBlockTape(reinterpret_cast<uint8_t *>(pPackLZW.data()), &nPackBitsLength, &nPackLZWLength);

		if (bRet)
		{
			UnpackLZW_Fast(pPackBits.data(), nPackBitsLength, pPackLZW.data(), nPackLZWLength / 2);
			UnpackBits(pPackBits.data(), nPackBitsLength);
			// Calculate average
			CalculateAverage();
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return bRet;
}


bool CTape::SaveMSFFile(const fs::path &strPath)
{
	CMSFManager msf;

	if (!msf.OpenFile(strPath.c_str(), false))
	{
		return false;
	}

	bool bRet = false;
	// получим длину в битах из длины в сэмплах.
	int nPackBitsLength = (m_nWaveLength + 7) >> 3;
	auto pPackBits = std::vector<uint8_t>(nPackBitsLength);
	auto pPackLZW = std::vector<uint16_t>(nPackBitsLength);

	if (pPackBits.data() && pPackLZW.data())
	{
		PackBits(pPackBits.data(), nPackBitsLength); // пакуем биты один сэмпл умещается в один бит
		int nPackLZW = PackLZW_Fast(pPackBits.data(), nPackBitsLength, pPackLZW.data()) * 2;

		bRet = msf.SetBlockTape(reinterpret_cast<uint8_t *>(pPackLZW.data()), nPackBitsLength, nPackLZW);
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return bRet;
}

bool CTape::GetWaveFile(TAPE_FILE_INFO *pTfi, bool bHeaderOnly)
{
	if (!m_pWave)
	{
		return false;
	}

	m_nPos = 0;     // начнём сначала
	m_bInverse = false;
	memset(pTfi, 255, sizeof(TAPE_FILE_INFO));
	size_t nLength = 0;

	// ищем настроечную последовательность в начале файла
	if (!FindTuning(1024, pTfi->start_tuning, nLength))
	{
		return false;
	}

	pTfi->synchro_start = m_nPos;

	// ищем маркер
	if (!FindMarker1())
	{
		return false;
	}

	// ищем настроечную последовательность в начале заголовка файла
	if (!FindTuning(7, pTfi->marker1, nLength))
	{
		return false;
	}

	pTfi->synchro_header = m_nPos;

	// ищем маркер
	if (!FindMarker())
	{
		return false;
	}

	pTfi->header = m_nPos;

	// читаем заголовок файла
	for (int i = 0; i < 20; ++i)
	{
		if (!ReadByte(((uint8_t *)&pTfi->address)[i]))
		{
			return false;
		}
	}

	// ищем настроечную последовательность в начале массива данных файла
	if (!FindTuning(7, pTfi->marker2, nLength))
	{
		return false;
	}

	pTfi->synchro_data = m_nPos;

	// ищем маркер
	if (!FindMarker())
	{
		return false;
	}

	if (bHeaderOnly)
	{
		return true;
	}

	if (m_pBin)
	{
		delete m_pBin;
		m_pBin = 0;
	}

	m_pBin = new uint8_t[pTfi->length];

	if (!m_pBin)
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		return false;
	}

	pTfi->data = m_nPos;

	// читаем данные
	for (int i = 0; i < pTfi->length; ++i)
	{
		if (!ReadByte(m_pBin[i]))
		{
			return false;
		}
	}

	// читаем контрольную сумму
	if (!ReadByte(((uint8_t *)&pTfi->crc)[0]))
	{
		return false;
	}

	if (!ReadByte(((uint8_t *)&pTfi->crc)[1]))
	{
		return false;
	}

	pTfi->synchro6 = m_nPos;

	// ищем финишную последовательность
	if (FindSyncro6())
	{
		FindTuning(128, pTfi->end_tuning, nLength);
		pTfi->end = pTfi->end_tuning + nLength;
	}
	else
	{
		// её может и не быть в нестандартных случаях
		pTfi->end = m_nWaveLength;
	}

	return true;
}

/*
 * Вход: nLength - ориентировочная длина искомой последовательности
 * Выход: wave_pos - начало найденной последовательности
 *        wave_length - длина найденной последовательности
 */
bool CTape::FindTuning(int nLength, size_t &wave_pos, size_t &wave_length)
{
	m_dAvgLength = 0;
	bool bFound = false;
	size_t syncro_pos = BAD_LENGTH;
	size_t last_pos = m_nPos;
	int imp_num = 0;
	size_t begin_pos = m_nPos;
	int nLastLen = 0;

	for (;;)
	{
		last_pos = m_nPos; // позиция перед импульсом
		int len = CalcImpLength(m_pWave, m_nPos, m_nWaveLength); // считаем длину импульса в сэмплах

		if (len == BAD_LENGTH)  // если не получилось посчитать
		{
			m_nPos = last_pos;  // откатываемся на позицию перед подсчётом
			return false;       // и выходим
		}

		int a = nLastLen - len; // вычисляем разницу в длине импульсов

		if (a < 0)
		{
			a = -a;     // берём модуль
		}

		if (a <= 3)     // если разница меньше разброса длительностей
		{
			// то это правильный импульс, будем с ним работать
			m_dAvgLength += double(len);    // прибавим к средней длительности, чтоб потом вычислить
			imp_num++;              // посчитаем этот импульс

			if (imp_num >= nLength && !bFound) // если количество правильных импульсов достигло ориентировочной длины, и ещё не зафиксировали
			{
				syncro_pos = begin_pos; // фиксируем начало синхропоследовательности
				bFound = true;      // отмечаем факт
			}
		}
		else if (!bFound)   // импульс неправильный и ещё начало не нашли
		{
			m_dAvgLength = 0.0; // то всё отменяем
			imp_num = 0;
			begin_pos = m_nPos; // сдвигаем предполагаемое начало синхропоследовательности
		}
		else    // импульс неправильный, а начало мы уже зафиксировали
		{
			break;  // выходим из цикла, наверно до маркера дошли
		}

		nLastLen = len; // текущую длину -> предыдущую длину
	}

	m_nPos = last_pos;  // откатываемся к предыдущему импульсу
	wave_pos = syncro_pos; // выдаём найденное начало синхропоследовательности
	wave_length = m_nPos - syncro_pos; // выдаём длительность найденной синхропоследовательности

	if (imp_num)        // если есть что считать
	{
		m_dAvgLength = m_dAvgLength / imp_num; // вычисляем среднюю длину с округлением в большую сторону
	}

	return true;
}

// разброс параметров относительно идеала
// тут главное не переборщить с допуском
constexpr double dDelta = 0.3;
constexpr double MinusDelta(double x)
{
	return x - x * dDelta;
}
constexpr double PlusDelta(double x)
{
	return x + x * dDelta;
}

// возвращаем длину импульса в средних импульсах
// 0 - очень короткий импульс (неправильный или ложный)
// 1 - "0" и синхроимпульс, их длительность == L (длина среднего импульса)
// 2 - "1", его длительность == 2L
// 4 - маркер, его длительность  == 4L
// 8 - очень длинный импульс (или финиш, или неправильный)
int CTape::DefineLength(int nLength) const
{
	double l = double(nLength) / m_dAvgLength;

	if (l < MinusDelta(1.0))
	{
		return 0;
	}

	if (l < PlusDelta(1.0))
	{
		return 1;
	}

	if (l < PlusDelta(2.0)) // упрощённые интервалы, без разрывов между допусками.
	{
		return 2;
	}

	if (l < PlusDelta(4.0))
	{
		return 4;
	}

	return 8;
}

// поиск первого маркера
bool CTape::FindMarker1()
{
	int size = 0;
	size_t pos = m_nPos;

	for (;;)
	{
		int len_1 = 0;

		// Считаем количество единиц
		while (GetCurrentSampleMono(m_pWave, pos) >= m_nAverage)
		{
			if (pos >= m_nWaveLength)
			{
				return false;
			}

			len_1++;
			pos++;
		}

		double l = double(len_1) / m_dAvgLength;
#ifdef _DEBUG
		constexpr double low = MinusDelta(4.0) / 2;
		constexpr double high = PlusDelta(4.0) / 2;
#endif

		if (MinusDelta(4.0) / 2 <= l && l <= PlusDelta(4.0) / 2)
		{
			break;
		}

		int len_0 = 0;

		// Считаем количество нулей
		while (GetCurrentSampleMono(m_pWave, pos) < m_nAverage)
		{
			if (pos >= m_nWaveLength)
			{
				return false;
			}

			len_0++;
			pos++;
		}

		l = double(len_0) / m_dAvgLength;

		if (MinusDelta(4.0) / 2 <= l && l <= PlusDelta(4.0) / 2)
		{
			m_bInverse = true;
			break;
		}
	}

	m_nPos = pos;
	bool bBit;

	if (ReadBit(bBit))
	{
		return bBit;    // в правильном маркере бит - "1"
	}

	return false;
}

bool CTape::FindMarker()
{
	int size = 0;
	int nLength = 0;

	for (;;)
	{
		nLength = CalcImpLength(m_pWave, m_nPos, m_nWaveLength);

		if (nLength == BAD_LENGTH)
		{
			return false;
		}

		size = DefineLength(nLength);

		if (size >= 4)
		{
			break;
		}
	}

	bool bBit;

	if (ReadBit(bBit))
	{
		return bBit;    // в правильном маркере бит - "1"
	}

	return false;
}


bool CTape::FindSyncro6()
{
	int nLength = CalcImpLength(m_pWave, m_nPos, m_nWaveLength);

	if (nLength == BAD_LENGTH)
	{
		return false;
	}

	int size = DefineLength(nLength);
	return (size >= 6);
}


bool CTape::ReadBit(bool &bBit)
{
	size_t oldPos = m_nPos;
	int length0 = CalcImpLength(m_pWave, m_nPos, m_nWaveLength); // длина информационного импульса

	if (length0 == BAD_LENGTH)
	{
		m_nPos = oldPos;
		return false;
	}

	int length1 = CalcImpLength(m_pWave, m_nPos, m_nWaveLength); // длина синхроимпульса за инфоимпульсом

	if (length1 == BAD_LENGTH)
	{
		m_nPos = oldPos;
		return false;
	}

	int size0 = DefineLength(length0);
//  int size1 = DefineLength(length1); // не имеет значения

	if (size0 == 1)
	{
		bBit = false;
		return true;
	}

	if (size0 == 2)
	{
		bBit = true;
		return true;
	}

	m_nPos = oldPos; // при неудаче откатываемся к началу бита
	return false;
}


bool CTape::ReadByte(uint8_t &byte)
{
	// int oldPos = m_nPos;
	uint8_t mask = 1;
	byte = 0;
	bool bBit;

	for (int i = 0; i < 8; ++i)
	{
		if (!ReadBit(bBit))
		{
			// m_nPos = oldPos; // при неудаче откатываемся к началу байта
			// это не очень полезно при анализе, где встретился плохой бит.
			return false;
		}

		if (bBit)
		{
			byte |= mask;
		}

		mask <<= 1;
	}

	return true;
}

// на входе: pWave - указатель на массив, pos - позиция в сэмплах
SAMPLE_INT CTape::GetCurrentSampleMono(SAMPLE_INT *pWave, const size_t pos) const
{
	size_t n = pos * m_WorkingWFX.nChannels;
	SAMPLE_INT s = 0.0;

	for (WORD chan = 0; chan < m_WorkingWFX.nChannels; ++chan)
	{
		// микшируем
		SAMPLE_INT t = s * pWave[n];
		s += pWave[n];

		if (t > 0.0) // если множители были одного знака
		{
			if (pWave[n] > 0.0) // если оба положительные
			{
				s -= t;
			}
			else // если оба отрицательные
			{
				s += t;
			}
		}

		// если разного или 0, то просто сумма
		n++;
	}

	return s;
}

// на входе: pWave - указатель на массив, pos - позиция в сэмплах
void CTape::SetCurrentSampleMono(SAMPLE_INT *pWave, const size_t pos, const SAMPLE_INT sample) const
{
	size_t n = pos * m_WorkingWFX.nChannels;

	for (WORD chan = 0; chan < m_WorkingWFX.nChannels; ++chan)
	{
		pWave[n++] = sample; // размножаем сэмпл по каналам
	}
}

/////////////////////////////////////////////////////////////////////////////
// Вход: pos - текущая позиция в сэмплах
//       nLength - длина wave в сэмплах, предел, дальше которого нельзя
// Выход: длина импульса в сэмплах
int CTape::CalcImpLength(SAMPLE_INT *pWave, size_t &pos, size_t nLength) const
{
	if (pos >= nLength)
	{
		return BAD_LENGTH;
	}

	int len_1 = 0;
	int len_0 = 0;

	if (m_bInverse)
	{
		// Пропускаем инверсные импульсы
		while (GetCurrentSampleMono(pWave, pos) >= m_nAverage)
		{
			if (++pos >= nLength)
			{
				return BAD_LENGTH;
			}
		}

		// Считаем количество единиц
		while (GetCurrentSampleMono(pWave, pos) < m_nAverage)
		{
			len_1++;

			if (++pos >= nLength)
			{
				return BAD_LENGTH;
			}
		}

		// Считаем количество нулей
		while (GetCurrentSampleMono(pWave, pos) >= m_nAverage)
		{
			len_0++;

			if (++pos >= nLength)
			{
				return BAD_LENGTH;
			}
		}

		return len_1 + len_0;
	}

	// Пропускаем инверсные импульсы
	while (GetCurrentSampleMono(pWave, pos) < m_nAverage)
	{
		if (++pos >= nLength)
		{
			return BAD_LENGTH;
		}
	}

	// Считаем количество единиц
	while (GetCurrentSampleMono(pWave, pos) >= m_nAverage)
	{
		len_1++;

		if (++pos >= nLength)
		{
			return BAD_LENGTH;
		}
	}

	// Считаем количество нулей
	while (GetCurrentSampleMono(pWave, pos) < m_nAverage)
	{
		len_0++;

		if (++pos >= nLength)
		{
			return BAD_LENGTH;
		}
	}

	return len_1 + len_0;
}


uint16_t CTape::CalcCRC(TAPE_FILE_INFO *pTfi) const
{
	if (!m_pBin)
	{
		return false;
	}

	int crc = 0;

	for (int i = 0; i < pTfi->length; ++i)
	{
		crc += m_pBin[i];

		if (crc & 0xFFFF0000)
		{
			// если случился перенос в 17 разряд (т.е. бит С для word)
			crc &= 0x0000FFFF; // его обнулим
			crc++; // но прибавим к сумме
		}
	}

	return LOWORD(crc);
}


bool CTape::LoadBinFile(const fs::path &strPath, TAPE_FILE_INFO *pTfi)
{ /***
	CFile binFile;

	if (!binFile.Open(strPath.c_str(), CFile::modeRead))
	{
		return false;
	}

	// читаем первое слово - адрес загрузки
	if (binFile.Read(&pTfi->address, sizeof(uint16_t)) != sizeof(uint16_t))
	{
		return false;
	}

	// читаем второе слово, длину.
	if (binFile.Read(&pTfi->length, sizeof(uint16_t)) != sizeof(uint16_t))
	{
		return false;
	}

	// проверим настоящий бин файл или просто расширение совпало.
	ULONGLONG ulllen = binFile.GetLength(); // размер файла

	if (ulllen > 65535)
	{
		// слишком большой файл, незнай что делать, пока будем просто выходить
		return false;
	}

	auto len = uint16_t(ulllen);
	bool bIsCRC = false;

	if (pTfi->length == len - 4)
	{
		bIsCRC = false;
	}
	else if (pTfi->length == len - 6)
	{
		bIsCRC = true;
	}
	else if (pTfi->length == len - 22)
	{
		bIsCRC = true;
		// а если сложный, надо пропустить немного ненужных данных
		binFile.Seek(20, CFile::SeekPosition::begin);
	}
	else
	{
		// return false;
		// не бин, но всё равно будем читать такой файл
		binFile.Seek(0, CFile::SeekPosition::begin); // перемещаемся в начало
		pTfi->length = len;
		pTfi->address = 0;
	}

	if (m_pBin)
	{
		m_pBin.reset();
	}

	m_pBin = std::make_unique<uint8_t[]>(pTfi->length);

	if (!m_pBin)
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		return false;
	}

	if (binFile.Read(m_pBin.get(), pTfi->length) != pTfi->length)
	{
		return false;
	}

	// прочитаем контрольную сумму, если она там есть
	if (bIsCRC && binFile.Read(&pTfi->crc, sizeof(uint16_t)) == sizeof(uint16_t))
	{
		if (CalcCRC(pTfi) != pTfi->crc)
		{
			TRACE("CRC Mismatch!\n");
		}
	}
	else
	{
		pTfi->crc = CalcCRC(pTfi);
	}

	fs::path strName = strPath.filename();
	CString strExt = strPath.extension().c_str();

	if (!strExt.CollateNoCase(CString(MAKEINTRESOURCE(IDS_FILEEXT_BINARY))))
	{
		strName = strPath.stem();
	}

	// кроме всего этого, тут надо отлавливать бейсиковские файлы. у них свой формат имени.
	// 6 символов - имя (пустое место забивается пробелами), потом .COD, .BIN, .ASC
	// а потом конец забивается нулями, а не пробелами.
	// .ASC обрабатывать не будем. .ASC нормальные люди не используют
	strExt = CString(strName.extension().c_str()).MakeUpper();

	if (strExt == _T(".COD")) // если это бейсиковский .COD
	{
		memset(pTfi->name, 0, 16);
		CString strBKName = strName.stem().c_str();
		// преобразуем юникодную строку в бкашную
		Global::UNICODEtoBK(strBKName, pTfi->name, 6, true);
		pTfi->name[6] = '.';
		pTfi->name[7] = 'C';
		pTfi->name[8] = 'O';
		pTfi->name[9] = 'D';
	}
	else if (strExt == _T(".BIN")) // если это бейсиковский .BIN
	{
		memset(pTfi->name, 0, 16);
		CString strBKName = strName.stem().c_str();
		// преобразуем юникодную строку в бкашную
		Global::UNICODEtoBK(strBKName, pTfi->name, 6, true);
		pTfi->name[6] = '.';
		pTfi->name[7] = 'B';
		pTfi->name[8] = 'I';
		pTfi->name[9] = 'N';
	}
	else
	{
		CString strBKName = strName.c_str();
		// преобразуем юникодную строку в бкашную
		Global::UNICODEtoBK(strBKName, pTfi->name, 16, true);
	}

	SAFE_DELETE_MEMORY(m_pWave); // m_pWave = 0
	m_nPos = 0; // тут будет формироваться длина

	if (!SetWaveFile(pTfi)) // рассчитываем размер массива
	{
		return false;
	}

	AllocWaveBuffer(m_nPos); // выделяем память под массив
	m_nPos = 0; m_nWaveLength = m_nWaveMaxLen;

	if (!SetWaveFile(pTfi)) // и теперь генерируем файл по настоящему.
	{
		return false;
	}

	// посчитаем средний уровень проще. массив-то полностью цифровой
	// m_nAverage = AVG_SAMPLE;
	// посчитаем полноценно
	CalculateAverage();***/
	return true;
}


bool CTape::SaveBinFile(const fs::path &strPath, TAPE_FILE_INFO *pTfi) const
{
	if (m_pBin)
	{
		CFile binFile;

		if (!binFile.Open(strPath.c_str(), CFile::modeCreate | CFile::modeWrite))
		{
			binFile.Write(&pTfi->address, sizeof(uint16_t));
			binFile.Write(&pTfi->length, sizeof(uint16_t));

			if (g_Config.m_bUseLongBinFormat)
			{
				binFile.Write(&pTfi->name, 16);
			}

			binFile.Write(m_pBin, pTfi->length);

			if (g_Config.m_bUseLongBinFormat)
			{
				binFile.Write(&pTfi->crc, sizeof(uint16_t));
			}

			return true;
		}
	}

	return false;
}


bool CTape::SetWaveFile(TAPE_FILE_INFO *pTfi)
{
	m_nPos = 0;
	m_nAvgLength = 15;
	ASSERT(m_nAvgLength & 1);

	if (SaveTuning(4096))
	{
		if (SaveTuning(8))
		{
			for (int i = 0; i < 20; ++i)
			{
				if (!SaveByte(((uint8_t *)&pTfi->address)[i]))
				{
					return false;
				}
			}

			if (SaveTuning(8))
			{
				for (int i = 0; i < pTfi->length; ++i)
				{
					if (!SaveByte(m_pBin[i]))
					{
						return false;
					}
				}

				pTfi->crc = CalcCRC(pTfi);

				if (SaveByte(((uint8_t *)&pTfi->crc)[0]))
				{
					if (SaveByte(((uint8_t *)&pTfi->crc)[1]))
					{
						if (SaveSyncro6())
						{
							if (SaveTuning(256))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


bool CTape::SaveTuning(int nLength)
{
	for (int i = 0; i < nLength; ++i)
	{
		if (!SaveImp(1))
		{
			return false;
		}
	}

	// запись маркера
	if (SaveImp(4))
	{
		if (SaveBit(true))
		{
			return true;
		}
	}

	return false;
}



bool CTape::SaveSyncro6()
{
	if (SaveImp(12))
	{
		return true;
	}

	return false;
}


bool CTape::SaveImp(int size)
{
	if (m_pWave)
	{
		for (int i = 0; i < size * (m_nAvgLength / 2 + 1); ++i)
		{
			if (m_nPos >= m_nWaveLength)
			{
				return false;
			}

			SetCurrentSampleMono(m_pWave, m_nPos++, double(MAX_SAMPLE) / FLOAT_BASE);
		}

		for (int i = 0; i < size * (m_nAvgLength / 2); ++i)
		{
			if (m_nPos >= m_nWaveLength)
			{
				return false;
			}

			SetCurrentSampleMono(m_pWave, m_nPos++, double(MIN_SAMPLE) / FLOAT_BASE);
		}
	}
	else
	{
		m_nPos += static_cast<size_t>(size) * m_nAvgLength;
	}

	return true;
}


bool CTape::SaveBit(bool bBit)
{
	if (bBit)
	{
		if (!SaveImp(2))
		{
			return false;
		}
	}
	else
	{
		if (!SaveImp(1))
		{
			return false;
		}
	}

	if (SaveImp(1))
	{
		return true;
	}

	return false;
}


bool CTape::SaveByte(uint8_t byte)
{
	uint8_t mask = 1;

	for (int i = 0; i < 8; ++i)
	{
		if (!SaveBit(!!(byte & mask)))
		{
			return false;
		}

		mask <<= 1;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// на входе, nLenInSamples - размер в сэмплах
bool CTape::AllocWaveBuffer(size_t nLenInSamples)
{
	if (m_pWave == nullptr || m_nWaveLength < nLenInSamples)
	{
		SAFE_DELETE_MEMORY(m_pWave);
		m_pWave = reinterpret_cast<SAMPLE_INT *>(malloc(nLenInSamples * m_WorkingWFX.nBlockAlign));
		m_nWaveLength = 0;

		if (!m_pWave)
		{
			m_nWaveMaxLen = 0;
			return false;
		}
	}

	m_nWaveMaxLen = nLenInSamples;
	m_nAverage = MIN_SAMPLE;
	size_t nSize = nLenInSamples * m_WorkingWFX.nBlockAlign;
	memset(m_pWave, 0, nSize);
	return true;
}


bool CTape::LoadBuffer(SAMPLE_INT *pBuff, size_t nLenInSamples)
{
	if (AllocWaveBuffer(nLenInSamples))
	{
		size_t nSize = nLenInSamples * m_WorkingWFX.nBlockAlign;
		memcpy(m_pWave, pBuff, nSize);
		CalculateAverage();
		return true;
	}

	return false;
}


/*
вход: pBuff - буфер, куда передаётся tape, формат данных в буфере - double 2 канала
nBufSampleLen - размер буфера в сэмплах.
*/
void CTape::PlayWaveGetBuffer(SAMPLE_INT *pBuff, const size_t nBufSampleLen)
{
	ZeroMemory(pBuff, nBufSampleLen * m_WorkingWFX.nBlockAlign);

	if (m_bPlay)
	{
		// m_nWaveLength и m_nPlayPos - измеряются в сэмплах
		size_t nLength = m_nWaveLength - m_nPlayPos;

		if (nLength > nBufSampleLen)    // если длина больше чем буфер,
		{
			nLength = nBufSampleLen;    // уменьшим до длины буфера
		}

		const size_t nByteLength = nLength * m_WorkingWFX.nBlockAlign;
		memcpy(pBuff, m_pWave + m_nPlayPos * BUFFER_CHANNELS, nByteLength);
		m_nPlayPos += nLength;

		if (m_nPlayPos >= m_nWaveLength)
		{
			StopPlay();
			m_nPlayPos = 0;
			m_bWaveLoaded = false; // больше не воспроизводить
		}
	}
}

/*
вход: pBuff - буфер, откуда берётся tape, формат данных в буфере - 16 бит 2 канала
nBufSampleLen - размер буфера в сэмплах.
*/
void CTape::RecordWaveGetBuffer(SAMPLE_INT *pBuff, const size_t nBufSampleLen)
{
	if (m_bRecord)
	{
		// посчитаем размер буфера в байтах
		size_t nBufLen = nBufSampleLen * m_WorkingWFX.nBlockAlign;

		// если место в буфере кончается, надо сделать новый буфер, побольше, и данные из старого скопировать в новый
		while (m_nRecordPos + nBufSampleLen >= m_nRecordLength)
		{
			m_nRecordLength += RECORD_GROW;
			void *p = realloc(m_pRecord, m_nRecordLength * m_WorkingWFX.nBlockAlign);

			if (p)
			{
				m_pRecord = reinterpret_cast<SAMPLE_INT *>(p);
			}
			else
			{
				// кончилась память
				StopRecord();
			}

			ASSERT(m_pRecord);
		}

		memcpy(m_pRecord + m_nRecordPos * m_WorkingWFX.nChannels, pBuff, nBufLen);
		/*
		Ещё тут надо высчитывать m_fAverage, потому что функция FindRecordBegin использует это значение
		причём, среднее значение в пределах заданного буфера вполне подойдёт.
		*/
		auto pSrcPos = pBuff;
		SAMPLE_INT avg = 0.0;

		for (size_t i = 0; i < nBufSampleLen; ++i)
		{
			avg += GetCurrentSampleMono(pSrcPos, i);
		}

		m_nAverage = SAMPLE_INT(avg / nBufSampleLen);

		if (FindRecordEnd(nBufSampleLen))
		{
			StopRecord();
		}

		if (FindRecordBegin(nBufSampleLen))  // Если нашли точку начала записи
		{
			m_nRecordPos += nBufSampleLen;
		}
	}
}

void CTape::StartRecord(bool bAutoBeginRecord, bool bAutoEndRecord)
{
	m_bAutoBeginRecord = bAutoBeginRecord;
	m_bAutoEndRecord = bAutoEndRecord;
	m_nAverage = MIN_SAMPLE;
	m_nRecordPos = 0;
	SAFE_DELETE_MEMORY(m_pRecord);
	const size_t nBufSize = static_cast<size_t>(RECORD_BUFFER) * m_WorkingWFX.nBlockAlign;
	m_pRecord = reinterpret_cast<SAMPLE_INT *>(malloc(nBufSize)); // создаём новый массив длиной RECORD_BUFFER сэмплов, каждый сэмпл - m_WorkingWFX.nChannels * SAMPLE_INT_SIZE

	if (m_pRecord)
	{
		ZeroMemory(m_pRecord, nBufSize);
		m_nRecordLength = RECORD_BUFFER; // длина массива в сэмплах
		m_bRecord = true;
	}
}


void CTape::StopRecord()
{
	if (m_bRecord)
	{
		m_bRecord = false;
		SAFE_DELETE_MEMORY(m_pWave);
		m_pWave = m_pRecord;
		m_pRecord = nullptr;
		m_nWaveLength = m_nRecordPos;
		m_nRecordPos = 0;
		CalculateAverage();
	}
}


/////////////////////////////////////////////////////////////////////////////
// nSmplBufLen - длина буфера в сэмплах
bool CTape::FindRecordBegin(const size_t nSmplBufLen)
{
	int nLastLen = BAD_LENGTH;

	if (m_bAutoBeginRecord)
	{
		m_nRecordPos = 0;
		int imp_num = 0;

		for (;;)
		{
			int len = CalcImpLength(m_pRecord, m_nRecordPos, nSmplBufLen);

			// тут len - в сэмплах
			if (len == BAD_LENGTH)
			{
				m_nRecordPos = 0;
				return false;
			}

			int a = nLastLen - len;

			if (a < 0) // если новая длина будет больше старой, то получится отрицательное значение
			{
				a = -a; // а нам важен модуль а не величина
			}

			if (a <= 3)
			{
				if (++imp_num >= 50)
				{
					break;
				}
			}
			else
			{
				imp_num = 0;
			}

			nLastLen = len;
			// TRACE ("\nWave pos %06i, Start = %i, Len = %i, Imp_Num = %i", m_nPos, begin_pos, len, imp_num);
			// Sleep (1);
		}

		TRACE("AutoSearch: File Start Found!\n");
		m_nRecordPos = 0;
		m_bAutoBeginRecord = false; // признак, что найдено начало записи при автопоиске
	}

	return true;
}


bool CTape::FindRecordEnd(const size_t nSmplBufLen)
{
	// автоматически ищем конец записи только если было автоматически найдено начало,
	// или вообще не был задан автопоиск начала.
	if (!m_bAutoBeginRecord && m_bAutoEndRecord)
	{
		SAMPLE_INT last_sample = -1;
		int last_length = 0;
		size_t nEnd = m_nRecordPos + nSmplBufLen;

		for (size_t i = m_nRecordPos; i < nEnd; ++i)
		{
			SAMPLE_INT sample = GetCurrentSampleMono(m_pRecord, i);

			if (sample == last_sample)
			{
				last_length++;
			}
			else
			{
				last_sample = sample;
				last_length = 0;
			}

			if (last_length >= 500)
			{
				m_bAutoEndRecord = false;
				TRACE("AutoSearch: File End Found!\n");
				return true;
			}
		}
	}

	return false;
}
