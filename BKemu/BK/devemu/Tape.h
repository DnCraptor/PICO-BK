// Tape.h: interface for the CTape class.
//


#pragma once

#include "BKSound_Defines.h"
#include "BKSound.h"
//#include <mmsystem.h>
//#include "bits/unique_ptr.h"

// "RIFF"
constexpr DWORD RIFF_TAG = 0x46464952;
// "fmt "
constexpr DWORD FMT_TAG = 0x20746d66;
// "data"
constexpr DWORD DATA_TAG = 0x61746164;
// "WAVE"
constexpr DWORD WAVE_TAG = 0x45564157;

#pragma pack(push)
#pragma pack(1) // envelope period (envT, R13-R14) has invalid word alignment
struct TAPE_FILE_INFO
{
	size_t              start_tuning;
	size_t              synchro_start;
	size_t              marker1;
	size_t              synchro_header;
	size_t              header;
	uint16_t            address;
	uint16_t            length;
	uint8_t             name[16];
	size_t              marker2;
	size_t              synchro_data;
	size_t              data;
	uint16_t            crc;
	size_t              synchro6;
	size_t              end_tuning;
	size_t              end;
};
#pragma pack(pop)

struct WaveHeader
{
	DWORD               riffTag;
	DWORD               size;
	DWORD               waveTag;
	DWORD               fmtTag;
	DWORD               fmtSize;
};

struct DataHeader
{
	DWORD               dataTag;
	DWORD               dataSize;
};

constexpr auto MAX_TABLE_SIZE = 65000;

class CTape
{
		WAVEFORMATEX    m_WorkingWFX;       // текущие рабочие параметры wave

		bool            m_bPlay;
		bool            m_bWaveLoaded;
		SAMPLE_INT     *m_pWave;            // собственно массив wave.
		size_t          m_nWaveMaxLen;      // размер массива m_pWave в сэмплах
		size_t          m_nWaveLength;      // размер данных в массиве m_pWave в сэмплах (может быть меньше m_nWaveMaxLen)
		size_t          m_nPlayPos;         // текущая позиция воспроизведения в сэмплах

		bool            m_bRecord;          // флаг записи
		SAMPLE_INT     *m_pRecord;          // массив, куда записывается звук
		size_t          m_nRecordPos;       // текущая позиция в массиве в сэмплах
		size_t          m_nRecordLength;    // длина массива в сэмплах

		bool            m_bAutoBeginRecord;
		bool            m_bAutoEndRecord;

		uint8_t*        m_pBin;  // массив бинарных данных, в которые преобразуются wav и tap
		size_t          m_nPos;
		SAMPLE_INT      m_nAverage;
		int             m_nAvgLength;
		double          m_dAvgLength;
		bool            m_bInverse;

		std::vector<uint16_t> m_pScanTable[MAX_TABLE_SIZE]; // массив для упаковки/распаковки
		int             m_nScanTableSize;

		// функции упаковки/распаковки wav в tap
		void            InitTables();
		void            ClearTables();
		int             TableLookup_Fast(uint8_t *pPackBits, uint16_t prefix, int end_pos) const;
		void            AddWord_Fast(uint8_t *pPackBits, uint16_t prefix, int end_pos);
		bool            UnpackWord(uint8_t *pPackBits, int &nPackBitsLength, uint16_t code) const;
		// функции преобразования модулированного wav в bin
		bool            FindTuning(int nLength, size_t &wave_pos, size_t &wave_length);
		bool            FindMarker();
		bool            FindMarker1();
		bool            FindSyncro6();
		int             CalcImpLength(SAMPLE_INT *pWave, size_t &pos, size_t length) const;
		inline SAMPLE_INT GetCurrentSampleMono(SAMPLE_INT *pWave, const size_t pos) const;
		void            SetCurrentSampleMono(SAMPLE_INT *pWave, const size_t pos, const SAMPLE_INT sample) const;
		int             DefineLength(int length) const;
		bool            ReadBit(bool &bBit);
		bool            ReadByte(uint8_t &byte);
		// функции преобразования bin в модулированный wav
		bool            SaveTuning(int length);
		bool            SaveSyncro6();
		bool            SaveImp(int size);
		bool            SaveBit(bool bBit);
		bool            SaveByte(uint8_t byte);

		size_t          ConvertSamples(WAVEFORMATEX wfx_in, void *inBuf, size_t nBufSize);
		void            ResampleBuffer(int nSrcSSR, int nDstSSR);
		bool            FindRecordBegin(const size_t nSmplBufLen);
		bool            FindRecordEnd(const size_t nSmplBufLen);
		void            CalculateAverage();

	public:
		CTape();
		virtual ~CTape();
		void            SetWaveParam(int nWorkingSSR = DEFAULT_SOUND_SAMPLE_RATE, int nWorkingChn = BUFFER_CHANNELS);
		int             GetWorkingSSR() const
		{
			return m_WorkingWFX.nSamplesPerSec;
		}
		int             GetWorkingChannels() const
		{
			return m_WorkingWFX.nChannels;
		}
		// Wave pack/unpack methods
		void            PackBits(uint8_t *pPackBits, int nPackBitsLength);
		void            UnpackBits(uint8_t *pPackBits, int nPackBitsLength);

		int             PackLZW_Fast(uint8_t *pPackBits, int nPackBitsLength, uint16_t *pPackLZW);
		void            UnpackLZW_Fast(uint8_t *pPackBits, int nPackBitsLength, uint16_t *pPackLZW, int nPackLZWLength);

		// Wave read/write methods
		bool            SetWaveFile(TAPE_FILE_INFO *pTfi);
		bool            GetWaveFile(TAPE_FILE_INFO *pTfi, bool bHeaderOnly = false);
		uint16_t        CalcCRC(TAPE_FILE_INFO *pTfi) const;

		bool            LoadWaveFile(const fs::path &strPath);
		bool            SaveWaveFile(const fs::path &strPath) const;

		bool            LoadBinFile(const fs::path &strPath, TAPE_FILE_INFO *pTfi);
		bool            SaveBinFile(const fs::path &strPath, TAPE_FILE_INFO *pTfi) const;

		bool            LoadMSFFile(const fs::path &strPath, bool bSilent = false);
		bool            SaveMSFFile(const fs::path &strPath);

		bool            LoadTmpFile(const fs::path &strPath);

		// Wave buffer methods
		bool            AllocWaveBuffer(size_t nLenInSamples);
		bool            LoadBuffer(SAMPLE_INT *pBuff, size_t nLenInSamples);
		inline size_t   GetWaveLength() const
		{
			return m_nWaveLength;
		}
		inline SAMPLE_INT *GetWaveBuffer() const
		{
			return m_pWave;
		}

		// Playing methods
		inline size_t   GetPlayWavePos() const
		{
			return m_nPlayPos;
		}

		void            PlayWaveGetBuffer(SAMPLE_INT *pBuff, const size_t nBufSampleLen);

		inline void     ResetPlayWavePos()
		{
			m_nPlayPos = 0;
		}
		inline void     StartPlay()
		{
			m_bPlay = true;
		}
		inline void     StopPlay()
		{
			m_bPlay = false;
		}
		inline bool     IsPlaying() const
		{
			return m_bPlay;
		}
		inline void     SetWaveLoaded(bool b)
		{
			m_bWaveLoaded = b;
		}

		inline bool     IsWaveLoaded() const
		{
			return m_bWaveLoaded;
		}

		void            StartRecord(bool bAutoBeginRecord, bool bAutoEndRecord);
		void            RecordWaveGetBuffer(SAMPLE_INT *pBuff, const size_t nBufSampleLen);
		void            StopRecord();

		inline bool     IsRecording() const
		{
			return m_bRecord;
		}

};
