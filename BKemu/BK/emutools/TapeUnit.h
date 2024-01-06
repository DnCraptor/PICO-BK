// TapeUnit.h: interface for the CTapeUnit class.
//


#pragma once


class CTapeUnit
{
	public:
		enum : int      {UNKNOWN = -1};
		enum class TAPETYPE : int { TYPE_NONE = -1, TYPE_BIN = 0, TYPE_WAV, TYPE_MSF, TYPE_TMP };

	protected:
		TAPETYPE        m_nType;
		int             m_nAddress;     // БКшный адрес загрузки
		int             m_nLength;      // БКшная длина файла
		int             m_nMSTime;
		DWORD           m_nWaveLength;  // размер wav файла
		bool            m_bCRC;         // флаг наличия КС
		CString         m_strName;      // БКшное имя файла
		fs::path        m_strPath;      // полное имя файла с путём.

		bool            IsFileWave(const fs::path &strPath) const;
		bool            IsFileMSF(const fs::path &strPath) const;
		bool            IsFileBin(const fs::path &strPath) const;

	public:

		CTapeUnit();
		CTapeUnit(CTapeUnit &tapeUnit);
		virtual ~CTapeUnit();

		CTapeUnit       &operator= (CTapeUnit &tapeUnit);

		bool            SetFile(const fs::path &strPath);
		bool            SetTmpFile(const fs::path &strPath);

		inline TAPETYPE GetType() const
		{
			return m_nType;
		}
		inline int      GetAddress() const
		{
			return m_nAddress;
		}
		inline int      GetLength() const
		{
			return m_nLength;
		}
		inline int      GetTime() const
		{
			return m_nMSTime;
		}
		inline DWORD    GetWaveLength() const
		{
			return m_nWaveLength;
		}
		inline bool     GetCRC() const
		{
			return m_bCRC;
		}
		inline const CString  GetName() const
		{
			return m_strName;
		}
		inline const fs::path &GetPath() const
		{
			return m_strPath;
		}

		bool            RetrieveInfo();
		bool            SaveAs(const fs::path &strPath, TAPETYPE type);
};

