#pragma once

constexpr auto FLOPPY_SIDES = 2;            // две стороны у дискеты
constexpr auto FLOPPY_TRACKS = 80;          // количество дорожек на стороне дискеты
constexpr auto FLOPPY_SECTOR_TYPE = 2;      // размер сектора в байтах 512
constexpr auto FLOPPY_TRACK_LEN = 10;       // количество секторов на дорожке
constexpr auto FLOPPY_TRKLEN_DD = 6250;
constexpr auto FLOPPY_TRKLEN_HD = 17700;
constexpr auto FLOPPY_INDEXLENGTH = 32;

class CFloppyDrive
{
		CFile       m_File;
		fs::path    m_strFileName;              // имя файла образа, который примонтирован в привод, чтобы повторно не перепримонтировывать
		bool        m_bReadOnly;                // Флаг защиты от записи
		bool        m_bTrackChanged;            // true = m_data было изменено - надо сохранить его в файл
		int         m_nDataTrack;               // Номер трека данных в массиве data
		int         m_nDataSide;                // Сторона диска данных в массиве data
		size_t      m_nDataPtr;                 // Смещение данных внутри data - позиция заголовка
		std::unique_ptr<uint8_t[]> m_pData;     // Raw track image for the current track
		std::unique_ptr<uint8_t[]> m_pMarker;   // Позиции маркеров

		int         m_nTracks;                  // количество дорожек у дискеты
		int         m_nSides;                   // количество сторон у дискеты
		int         m_nSectorsPerTrk;           // количество секторов на дорожке
		int         m_nSectorSize;              // номер размера сектора 0=128,1=256,2=512,3=1024
		size_t      m_nTrackSize;               // размер логической дорожки в байтах = m_nSectorsPerTrk * m_nSectorSizes[m_nSectorSize]
		size_t      m_nRawTrackSize;            // размер сырой дорожки в байтах = m_nSectorsPerTrk * 650
		size_t      m_nRawMarkerSize;           // размер таблицы позиций маркера = m_nRawTrackSize / 2

		bool        SetArrays();
		void        EncodeTrackData(uint8_t *pSrc);
		bool        DecodeTrackData(uint8_t *pDest) const;

		uint16_t    getCrc(uint8_t *ptr, size_t len) const;

	public:
		CFloppyDrive();
		~CFloppyDrive();

		static const int m_nSectorSizes[4];     // массив размеров секторов
		void        Reset();

		inline bool isReadOnly() const
		{
			return m_bReadOnly;
		}

		inline bool isAttached() const
		{
			return (m_File.m_hFile != CFile::hFileNull);
		}

		bool        Attach(const fs::path &strFile, bool bExclusive);
		void        Detach();
		void        SetGeometry(int nTracks, int nSides, int nSectPerTrk, int nSectSz);
		void        PrepareTrack(int nTrack, int nSide);
		void        FlushChanges();  // Если текущая дорожка была изменена, сохраним её

		// функции для работы с эмуляцией дисковых операций

		LONGLONG    FDSeek(LONGLONG pos, UINT from);    // позиционирование в файле образа
		UINT        FDRead(void *lpBuf, UINT nCount);   // чтение данных из образа
		void        FDWrite(void *lpBuf, UINT nCount);  // запись данных в образ

		// функции для работы с эмуляцией вращения

		void        MovePtr();          // сдвиг указателя на следующее слово
		bool        isIndex() const     // проверка, находится ли указатель в зоне индексного отверстия
		{
			return (m_nDataPtr < FLOPPY_INDEXLENGTH);
		}
		void        WrData(uint16_t w); // запись данных на текущую позицию указателя
		uint16_t    RdData() const;     // чтение данных по текущей позиции указателя
		void        setMarker(bool m)   // задание позиции маркера
		{
			m_pMarker[m_nDataPtr / 2] = m;
		}
		bool        getMarker() const   // получение позиции маркера
		{
			return m_pMarker[m_nDataPtr / 2];
		}
};

