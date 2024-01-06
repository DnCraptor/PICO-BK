/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// SoundGen.h
//

#pragma once

#define DIRECTSOUND_VERSION 0x1000

//#include <Mmsystem.h>
#include "Config.h"
#include <mutex>

// способ синхронизации
// 1 - через семафор (нагрузка на процессор меньше)
// 0 - через переменную-счётчик
#define BKSYNCHRO_SEMAPHORE 1

/* wave data block header */
typedef struct wavehdr_tag {
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD_PTR   dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag FAR *lpNext;     /* reserved for driver */
    DWORD_PTR   reserved;               /* reserved for driver */
} WAVEHDR, *PWAVEHDR, NEAR *NPWAVEHDR, FAR *LPWAVEHDR;

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;

class CBkSound
{
	protected:
		SAMPLE_INT      m_dSampleL, m_dSampleR;
		SAMPLE_INT      m_dFeedbackL, m_dFeedbackR;
		uint32_t        m_nWaveCurrentBlock;
		uint32_t        m_nBufCurPos;
		uint32_t        m_nBufSize;             // размер звукового буфера в байтах
		uint32_t        m_nBufSizeInSamples;    // размер звукового буфера в сэмплах
		int             m_nSoundSampleRate;
		SAMPLE_IO      *m_mBufferIO;            // указатель на текущий заполняемый буфер воспроизведения
		WAVEHDR        *m_pWaveBlocks;
		HWAVEOUT        m_hWaveOut;
		WAVEFORMATEX    m_wfx;
		bool            m_bSoundGenInitialized;

#if (BKSYNCHRO_SEMAPHORE)
		const HANDLE    m_hSem;
#else
		static std::mutex       m_mutCS;
		static volatile int     m_nWaveFreeBlockCount;
#endif
		// захват звука
		bool            m_bCaptureProcessed;
		bool            m_bCaptureFlag;
		std::mutex      m_mutCapture;
		CFile           m_waveFile;
		size_t          m_nWaveLength;

	public:
		CBkSound();
		virtual ~CBkSound();
		int             ReInit(int nSSR);
		void            SoundGen_SetVolume(uint16_t volume);
		uint16_t        SoundGen_GetVolume();
		void            SoundGen_ResetSample(SAMPLE_INT L, SAMPLE_INT R);
		void            SoundGen_SetSample(SAMPLE_INT &L, SAMPLE_INT &R);
		void            SoundGen_MixSample(sOneSample *pSm);
		void            SoundGen_FeedDAC_Mixer(sOneSample *pSm);
		void            SetCaptureStatus(bool bCapture, const CString &strUniq);
		bool            IsCapture() const
		{
			return m_bCaptureProcessed;
		}

	protected:
		void            SoundGen_Initialize(uint16_t volume);
		void            SoundGen_Finalize();

		void            Init(uint16_t volume, int nSSR);
		void            UnInit();

		void            PrepareCapture(const CString &strUniq);
		void            CancelCapture();
		void            WriteToCapture();
		static void CALLBACK WaveCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
};
