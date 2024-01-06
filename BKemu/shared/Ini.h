#pragma once
#include "pch.h"
#include <vector>

constexpr auto INI_BUFFER_LENGTH = 1024;

class CIni
{
	public:
		enum class IniStatus { NOTREADED = 0, READED, FLUSHED };
		enum class IniError { OK_NOERROR = 0, READ_ERROR, WRITE_ERROR, PARSE_ERROR };
		struct INI_KEY_STRUCT
		{
			DWORD Hash;
			CString Key;
			CString Value;
			CString Comment;
			INI_KEY_STRUCT() noexcept : Hash(0) {}
			INI_KEY_STRUCT(const DWORD h, const CString &k, const CString &v, const CString &c)
				: Hash(h)
				, Key(k)
				, Value(v)
				,Comment(c)
			{}
		};

		struct INI_SECTION_STRUCT
		{
			DWORD Hash;
			CString Name;
			std::vector<INI_KEY_STRUCT> Keys;
			INI_SECTION_STRUCT() noexcept : Hash(0) {}
		};

	protected:
		fs::path        m_strFileName; // Имя ини файла
		IniStatus       m_iniStatus;
		IniError        m_iniError;
		int             m_LineNum;
		int             m_Encoding; // 0 - utf8, 1 - ansi -- сейчас не используется. Только АНСИ

		std::vector<INI_SECTION_STRUCT> SetOfSections;

		// переменные для поиска параметров секции
		int             m_nFindSectIdx;     //текущая секция, в которой ищем.
		int             m_nFindKeyIdx;     // текущий ключ, который выдаём.

		int             CheckComment(const CString &str, int &clen) const;

		bool            ReadIni();
		DWORD           GetHash(const CString &str) const;
		bool            Add(const CString &strSection, const CString &strKey, const CString &strVal, const CString &strComm, bool bChangeComment = false);

	//	std::unique_ptr<CHAR> ConvertUnicodetoStr(UINT type, const CString &ustr) const; // type = CP_ACP - ansi; CP_UTF8 - utf8
	//	CString         ConvertStrtoUnicode(UINT type, LPCSTR astr) const; // type = CP_ACP - ansi; CP_UTF8 - utf8

		bool           _intGetValueString(const CString &strSection, const CString &strKey, CString &strValue);

		bool            DeleteKey(const CString &strSection, const CString &strKey);

	public:
		CIni();
		virtual ~CIni();

		inline void     SetIniFileName(const fs::path &strFileName)
		{
			m_strFileName = strFileName;
		}
		inline const fs::path &GetIniFileName() noexcept
		{
			return m_strFileName;
		}
		inline IniStatus GetIniStatus() const noexcept
		{
			return m_iniStatus;
		}
		inline IniError GetIniError() const noexcept
		{
			return m_iniError;
		}
		inline void     SetEncoding(int e) noexcept
		{
			m_Encoding = e;
		}
		inline int      GetEncoding() const noexcept
		{
			return m_Encoding;
		}

		void            Clear();
		bool            FlushIni();

		ULONGLONG       FlushIniToMemory(uint8_t *pBuff, UINT nSize);
		bool            ReadIniFromMemory(uint8_t *pBuff, UINT nSize);

		CString         GetValueString(const CString &strSection, const CString &strKey, const CString &strDefault);
		CString         GetValueString(int nSection, const CString &strKey, const CString &strDefault);
		CString         GetValueString(const int nSection, const int nKey, const CString &strDefault);
		int             GetValueInt(const int nSection, const int nKey, const int nDefault);
		double          GetValueFloat(const int nSection, const int nKey, const double fDefault);
		bool            GetValueBool(const int nSection, const int nKey, const bool bDefault);

		bool            SetValueString(const CString &strSection, const CString &strKey, const CString &strVal);
		bool            SetValueString(int nSection, const CString &strKey, const CString &strVal);
		bool            SetValueString(const int nSection, const int nKey, const CString &strVal);
		bool            SetValueInt(const int nSection, const int nKey, const int iVal);
		bool            SetValueFloat(const int nSection, const int nKey, const double fVal);
		bool            SetValueBool(const int nSection, const int nKey, const bool bVal);

		CString         GetValueStringEx(const CString &strCustomName, const int nSection, const int nKey, const CString &strDefault);
		int             GetValueIntEx(const CString &strCustomName, const int nSection, const int nKey, const int nDefault);
		double          GetValueFloatEx(const CString &strCustomName, const int nSection, const int nKey, const double fDefault);
		bool            GetValueBoolEx(const CString &strCustomName, const int nSection, const int nKey, const bool bDefault);

		bool            SetValueStringEx(const CString &strCustomName, const int nSection, const int nKey, const CString &strVal, bool bForce = false);
		bool            SetValueIntEx(const CString &strCustomName, const int nSection, const int nKey, const int iVal, bool bForce = false);
		bool            SetValueFloatEx(const CString &strCustomName, const int nSection, const int nKey, const double fVal, bool bForce = false);
		bool            SetValueBoolEx(const CString &strCustomName, const int nSection, const int nKey, const bool bVal, bool bForce = false);

		// копирование секции из другого ини файла в этот
		bool            CopySection(CIni *pSrcIni, const CString &strSectionName);

// ----------------------------------------------------------------------
		// работа с параметрами секции
		// последовательная выдача всех параметров секции
		// вход: strSection - имя секции
		// выход: strKey - ключ
		//        strValue - значение
		//  true - результат успешен
		//  false - неудача, или конец поиска. strKey, strValue - не валидны
		bool            FindKeyStart(const CString &strSection, CString &strKey, CString &strValue);
		bool            FindKeyNext(CString &strKey, CString &strValue);
};

