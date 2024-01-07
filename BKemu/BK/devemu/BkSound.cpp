/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// SoundGen.cpp
//

#include "pch.h"
#include "BKSound.h"
#include "BKMessageBox.h"
#include "Tape.h"

// сколько будет вообще буферов
constexpr auto BKSOUND_BLOCK_COUNT = 8;

/*
* сколько звуковых фреймов в секунду
* 100 - это количество звуковых фреймов в секунду, которое может нормально обрабатываться waveOut;
* если оставить 50 - буфер получается слишком большой, и латентность заметна;
* если делать больше 100 - буфер получается слишком маленький, и очень много ресурсов уходит на заполнение
* и отправку данных в waveOut.
*/
constexpr auto SOUND_FRAMERATE = 100;

// коэффициент увеличения длины буфера для устройств, работающих со звуком
constexpr auto FRAMESIZE_EXP = 4;

CBkSound::CBkSound()
	: m_bSoundGenInitialized(false)
	, m_dSampleL(0.0)
	, m_dSampleR(0.0)
	, m_dFeedbackL(0.0)
	, m_dFeedbackR(0.0)
	, m_nWaveCurrentBlock(0)
	, m_nBufCurPos(0)
	, m_pWaveBlocks(nullptr)
	, m_mBufferIO(nullptr)
	, m_bCaptureProcessed(false)
	, m_bCaptureFlag(false)
	, m_nWaveLength(0)
#if (BKSYNCHRO_SEMAPHORE)
	, m_hSem(CreateSemaphore(nullptr, BKSOUND_BLOCK_COUNT, BKSOUND_BLOCK_COUNT, nullptr))
#endif
{
	Init(0x7fff, g_Config.m_nSoundSampleRate);
}

CBkSound::~CBkSound()
{
	UnInit();
#if (BKSYNCHRO_SEMAPHORE)
	CloseHandle(m_hSem);
#endif
}

int CBkSound::ReInit(int nSSR)
{
	if (m_nSoundSampleRate != nSSR)
	{
		uint16_t vol = SoundGen_GetVolume();
		UnInit();
		Init(vol, nSSR);
	}

// для остальных устройств не обязательно иметь очень мелкий буфер, можно и побольше.
// главное, соблюдать кратность.
// и на выходе - длина буфера в сэмплах.
	return m_nSoundSampleRate * FRAMESIZE_EXP / SOUND_FRAMERATE;
}

void CBkSound::Init(uint16_t volume, int nSSR)
{
	m_nSoundSampleRate = nSSR;   // текущая частота дискретизации
	m_nBufSize = (m_nSoundSampleRate * static_cast<int>(SAMPLE_IO_BLOCKALIGN)) / SOUND_FRAMERATE;  // размер звукового буфера в байтах
	m_nBufSizeInSamples = m_nBufSize / SAMPLE_IO_SIZE; // размер буфера в сэмплах (в сумме для всех каналов)
	SoundGen_Initialize(volume);
}

void CBkSound::UnInit()
{
	SoundGen_Finalize();
}

void CBkSound::SoundGen_SetVolume(uint16_t volume)
{
	if (m_bSoundGenInitialized)
	{
		waveOutSetVolume(m_hWaveOut, MAKELONG(volume, volume));
	}
}

static WORD max(WORD a, WORD b) {
	return a > b ? a : b;
}

uint16_t CBkSound::SoundGen_GetVolume()
{
	if (m_bSoundGenInitialized)
	{
		DWORD vol = 0;
		waveOutGetVolume(m_hWaveOut, &vol);
		return max(LOWORD(vol), HIWORD(vol));
	}

	return 0;
}


void CBkSound::SoundGen_Initialize(uint16_t volume)
{ /*** 
	if (!m_bSoundGenInitialized)
	{
		size_t totalBufferSize = (m_nBufSize + sizeof(WAVEHDR)) * BKSOUND_BLOCK_COUNT;
		auto mbuffer = reinterpret_cast<uint8_t *>(HeapAlloc(
		                                               GetProcessHeap(),
		                                               HEAP_ZERO_MEMORY,
		                                               totalBufferSize));

		if (mbuffer)
		{
			m_pWaveBlocks = reinterpret_cast<WAVEHDR *>(mbuffer);
			mbuffer += sizeof(WAVEHDR) * BKSOUND_BLOCK_COUNT;
#if (!BKSYNCHRO_SEMAPHORE)
			m_nWaveFreeBlockCount = BKSOUND_BLOCK_COUNT;
#endif
			m_nWaveCurrentBlock = 0;
			m_wfx.cbSize = 0; // extra size. sizeof (WAVEFORMATEX);
			m_wfx.wFormatTag = WAVE_FORMAT_PCM;
			m_wfx.nSamplesPerSec = m_nSoundSampleRate;
			m_wfx.wBitsPerSample = SAMPLE_IO_BPS;
			m_wfx.nChannels = BUFFER_CHANNELS;
			m_wfx.nBlockAlign = (m_wfx.wBitsPerSample >> 3) * m_wfx.nChannels;
			m_wfx.nAvgBytesPerSec = m_wfx.nSamplesPerSec * m_wfx.nBlockAlign;
			MMRESULT result = waveOutOpen(
			                      &m_hWaveOut, WAVE_MAPPER, &m_wfx,
			                      reinterpret_cast<DWORD_PTR>(WaveCallback),
#if (BKSYNCHRO_SEMAPHORE)
			                      reinterpret_cast<DWORD_PTR>(m_hSem),
#else
			                      reinterpret_cast<DWORD_PTR>(&m_nWaveFreeBlockCount),
#endif
			                      CALLBACK_FUNCTION);

			if (result == MMSYSERR_NOERROR)
			{
				bool bOK = true;
				waveOutSetVolume(m_hWaveOut, MAKELONG(volume, volume));
				m_nBufCurPos = 0;

				for (int i = 0; i < BKSOUND_BLOCK_COUNT; ++i)
				{
					m_pWaveBlocks[i].dwBufferLength = m_nBufSize;
					m_pWaveBlocks[i].lpData = reinterpret_cast<LPSTR>(mbuffer);
					mbuffer += m_nBufSize;
					result = waveOutPrepareHeader(m_hWaveOut, &m_pWaveBlocks[i], sizeof(WAVEHDR));

					if (result != MMSYSERR_NOERROR)
					{
						bOK = false;
						break;
					}
				}

				// инициализируем указатель на текущий заполняемый блок
				m_mBufferIO = reinterpret_cast<SAMPLE_IO *>(m_pWaveBlocks[m_nWaveCurrentBlock].lpData);
				m_bSoundGenInitialized = bOK;
			}
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}
	} **/
}

void CBkSound::SoundGen_Finalize()
{ /***
	if (m_bSoundGenInitialized)
	{
#if (BKSYNCHRO_SEMAPHORE)

		for (int i = 0; i < BKSOUND_BLOCK_COUNT; ++i)
		{
			WaitForSingleObject(m_hSem, INFINITE);
		}

		LONG lPrevCount;
		ReleaseSemaphore(m_hSem, BKSOUND_BLOCK_COUNT, &lPrevCount);
#else

		while (m_nWaveFreeBlockCount < BKSOUND_BLOCK_COUNT)
		{
			Sleep(10);
		}

#endif

		for (int i = 0; i < BKSOUND_BLOCK_COUNT; ++i)
		{
			if (m_pWaveBlocks[i].dwFlags & WHDR_PREPARED)
			{
				waveOutUnprepareHeader(m_hWaveOut, &m_pWaveBlocks[i], sizeof(WAVEHDR));
			}
		}

		waveOutClose(m_hWaveOut);
	}

	HeapFree(GetProcessHeap(), 0, m_pWaveBlocks);***/
	m_pWaveBlocks = nullptr;
	m_bSoundGenInitialized = false;
}

#if (!BKSYNCHRO_SEMAPHORE)
// статические переменные для каллбака
std::mutex      CBkSound::m_mutCS;
volatile int    CBkSound::m_nWaveFreeBlockCount = 0;
#endif

void CALLBACK CBkSound::WaveCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{/****
	if (uMsg == WOM_DONE)
	{
#if (BKSYNCHRO_SEMAPHORE)
		ReleaseSemaphore(reinterpret_cast<HANDLE>(dwInstance), 1, nullptr);
#else
		auto freeBlockCounter = reinterpret_cast<int *>(dwInstance);
		m_mutCS.lock();
		(*freeBlockCounter)++;
		// свободный блок появился
		m_mutCS.unlock();
#endif
	}***/
}

void CBkSound::SoundGen_FeedDAC_Mixer(sOneSample *pSm)
{ /***
	pSm->s[OSL] = m_dSampleL;
	pSm->s[OSR] = m_dSampleR;

	if (m_bSoundGenInitialized)
	{
		m_mBufferIO[m_nBufCurPos++] = static_cast<SAMPLE_IO>(pSm->s[OSL] * FLOAT_BASE);
		m_mBufferIO[m_nBufCurPos++] = static_cast<SAMPLE_IO>(pSm->s[OSR] * FLOAT_BASE);

		if (m_nBufCurPos >= m_nBufSizeInSamples)
		{
			m_nBufCurPos = 0;
			// если свободных блоков нет, подождём.
#if (BKSYNCHRO_SEMAPHORE)
			WaitForSingleObject(m_hSem, INFINITE);
#else

			while (m_nWaveFreeBlockCount <= 0)
			{
				SleepEx(0, TRUE);
			}

			// резервируем очередной блок
			{
				m_mutCS.lock();
				m_nWaveFreeBlockCount--;
				m_mutCS.unlock();
			}
#endif
			// отправляем звучать заполненный блок
			m_pWaveBlocks[m_nWaveCurrentBlock].dwFlags = WHDR_PREPARED; // оставляем только один флаг
			waveOutWrite(m_hWaveOut, &m_pWaveBlocks[m_nWaveCurrentBlock], sizeof(WAVEHDR));

			if (m_bCaptureFlag)
			{
				WriteToCapture();
			}

			// переходим циклически к следующему блоку
			if (++m_nWaveCurrentBlock >= BKSOUND_BLOCK_COUNT)
			{
				m_nWaveCurrentBlock = 0;
			}

			// и инициализируем указатель на заполняемый блок.
			m_mBufferIO = reinterpret_cast<SAMPLE_IO *>(m_pWaveBlocks[m_nWaveCurrentBlock].lpData);
		}
	}***/
}


void CBkSound::SoundGen_ResetSample(SAMPLE_INT L, SAMPLE_INT R)
{
	m_dSampleL = L;
	m_dSampleR = R;
}

void CBkSound::SoundGen_SetSample(SAMPLE_INT &L, SAMPLE_INT &R)
{
	m_dSampleL = L;
	m_dSampleR = R;
}

void CBkSound::SoundGen_MixSample(sOneSample *pSm)
{
	SAMPLE_INT s = pSm->s[OSL];
	SAMPLE_INT t = s * m_dSampleL;
	m_dSampleL += s;

	if (t > 0.0) // если множители были одного знака
	{
		if (s > 0.0) // если оба положительные
		{
			m_dSampleL -= t;
		}
		else // если оба отрицательные
		{
			m_dSampleL += t;
		}
	}

	// если разного или 0, то просто сумма
	s = pSm->s[OSR];
	t = s * m_dSampleR;
	m_dSampleR += s;

	if (t > 0.0) // если множители были одного знака
	{
		if (s > 0.0) // если оба положительные
		{
			m_dSampleR -= t;
		}
		else // если оба отрицательные
		{
			m_dSampleR += t;
		}
	}

	// если разного или 0, то просто сумма
}


void CBkSound::SetCaptureStatus(bool bCapture, const CString &strUniq)
{
	if (bCapture)
	{
		PrepareCapture(strUniq);
	}
	else
	{
		CancelCapture();
	}
}

void CBkSound::PrepareCapture(const CString &strUniq)
{
	if (m_bCaptureProcessed)
	{
		CancelCapture();
	}
/***
	CString strName = CString(MAKEINTRESOURCE(IDS_FILEEXT_WAVE));
	strName = _T("capture_") + strUniq + strName;
	fs::path strPathName = g_Config.m_strScreenShotsPath / strName.GetString();

	if (!m_waveFile.Open(strPathName.c_str(), CFile::modeCreate | CFile::modeReadWrite))
	{
		return;
	}

	m_nWaveLength = 0;
	DataHeader dataHeader
	{
		DATA_TAG,
		static_cast<DWORD>(m_nWaveLength) *m_wfx.nBlockAlign
	};
	WaveHeader waveHeader
	{
		RIFF_TAG,
		sizeof(WaveHeader) + sizeof(WAVEFORMATEX) + sizeof(DataHeader) + dataHeader.dataSize,
		WAVE_TAG,
		FMT_TAG,
		sizeof(WAVEFORMATEX)
	};
	m_waveFile.Write(&waveHeader, sizeof(WaveHeader));
	m_waveFile.Write(&m_wfx, waveHeader.fmtSize);
	m_waveFile.Write(&dataHeader, sizeof(DataHeader));
	m_bCaptureProcessed = true;
	m_bCaptureFlag = true;***/
}

void CBkSound::CancelCapture()
{
	if (m_bCaptureProcessed)
	{
	///	std::lock_guard<std::mutex> lk(m_mutCapture);
		m_waveFile.SeekToBegin();
		DataHeader dataHeader
		{
			DATA_TAG,
			static_cast<DWORD>(m_nWaveLength) *m_wfx.nBlockAlign
		};
		WaveHeader waveHeader
		{
			RIFF_TAG,
			sizeof(WaveHeader) + sizeof(WAVEFORMATEX) + sizeof(DataHeader) + dataHeader.dataSize,
			WAVE_TAG,
			FMT_TAG,
			sizeof(WAVEFORMATEX)
		};
		m_waveFile.Write(&waveHeader, sizeof(WaveHeader));
		m_waveFile.Write(&m_wfx, waveHeader.fmtSize);
		m_waveFile.Write(&dataHeader, sizeof(DataHeader));
		m_waveFile.Close();
		m_bCaptureProcessed = false;
		m_bCaptureFlag = false;
	}
}

void CBkSound::WriteToCapture()
{
///	std::lock_guard<std::mutex> lk(m_mutCapture);

	if (m_bCaptureProcessed)
	{
		m_waveFile.Write(m_pWaveBlocks[m_nWaveCurrentBlock].lpData, m_nBufSize);
		m_nWaveLength += m_nBufSize;
	}
}

