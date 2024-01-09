#include "Ini.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CIni::CIni()
	: m_nFindSectIdx(-1)
	, m_nFindKeyIdx(-1)
	, m_iniStatus(IniStatus::NOTREADED)
	, m_iniError(IniError::OK_NOERROR)
	, m_LineNum(0)
	, m_Encoding(0)
{
}

CIni::~CIni()
{
	Clear();
}

void CIni::Clear()
{
	if (!SetOfSections.empty())
	{
		if ((m_iniError != IniError::WRITE_ERROR) && (m_iniError != IniError::READ_ERROR))
		{
			FlushIni();
		}

		for (auto &p : SetOfSections)
		{
			p.Keys.clear();
		}

		SetOfSections.clear();
	}
}

/*
  получение хеша строки для быстрого поиска, чтобы сравнивать числа, а не строки
*/
DWORD CIni::GetHash(const CString &str) const
{
	DWORD h = 0; // расчёт функцией FAQ6
	DWORD hval = 0x811c9dc5; // расчёт функцией FNV
	constexpr auto FNV_32_PRIME = 0x01000193;

	if (!str.IsEmpty())
	{
		CString strlc = str; // сделаем строки регистронезависимыми.
		strlc.MakeLower(); // а то какая-то вообще ерунда получается.
		const int nLen = strlc.GetLength();

		for (int i = 0; i < nLen; ++i)
		{
			h += strlc[i];
			h += (h << 10);
			h ^= (h >> 6);
			hval ^= DWORD(strlc[i]);
			hval *= FNV_32_PRIME;
		}

		h += (h << 3);
		h ^= (h >> 11);
		h += (h << 15);
	}

	return h ^ ((hval << 13) | (hval >> 19));
}

/*
выход: true - добавили новый элемент
false - не добавили новый элемент, по разным причинам
*/
bool CIni::Add(const CString &strSection, const CString &strKey, const CString &strVal, const CString &strComm, bool bChangeComment)
{
	if (strSection.IsEmpty() || strKey.IsEmpty())
	{
		return false;
	}

	const DWORD hashSection = GetHash(strSection);
	// начинаем поиск

	for (auto &pISS : SetOfSections)
	{
		if (pISS.Hash == hashSection)
		{
#ifdef DEBUG

			if (pISS.Name != strSection)
			{
				TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashSection, pISS.Name, strSection);
				ASSERT(false);
			}

#endif // DEBUG
			// нашли секцию
			const DWORD hashKey = GetHash(strKey);

			for (auto &pIKS : pISS.Keys)
			{
				if (pIKS.Hash == hashKey)
				{
#ifdef DEBUG

					if (pIKS.Key != strKey)
					{
						TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashKey, pIKS.Key, strKey);
						ASSERT(false);
					}

#endif // DEBUG

					// нашли ключ
					if (strVal.Compare(CString(pIKS.Value)) != 0)
					{
						// и старое значение не совпадает с новым
						pIKS.Value = strVal;
						return true;
					}

					return false;    // нечего добавлять
				}
			}

			// в секции ключа не нашли, надо добавить
			INI_KEY_STRUCT pIKS{
				hashKey,
				strKey,
				strVal,
				bChangeComment ? strComm : CString(_T(""))
			};

			pISS.Keys.push_back(pIKS); // добавляем
			return true;
		}
	}

	// даже секции такой не нашли, надо создать
	// и новый ключ тоже создадим
	INI_KEY_STRUCT pIKS{
		GetHash(strKey),
		strKey,
		strVal,
		bChangeComment ? strComm : CString(_T(""))
	};

	// теперь секцию
	INI_SECTION_STRUCT pISS;
	pISS.Hash = hashSection;
	pISS.Name = strSection;
	pISS.Keys.push_back(pIKS);
	SetOfSections.push_back(pISS);
	return true;
}

// проверяем, есть ли в строке комментарий. и если есть
// то какой.
// Вход: str - строка
// выход: позиция символа комментария
//      clen - длина символа комментария, 1 или 2
int CIni::CheckComment(const CString &str, int &clen) const
{
	const int len = str.GetLength();
	clen = 1;

	if (len)
	{
		const int cpos = str.Find(_T(';')); // ищем точку с запятой
		int spos = str.Find(_T('/')); // ищем слеш

		if (spos >= 0) // если слеш нашёлся
		{
			// то ищем второй за ним
			if (spos < len - 1)
			{
				const TCHAR ch = str.GetAt(spos + 1);
				if (ch != '/')
				{
					spos = -1;
				}
			}
			else
			{
				spos = -1;
			}
		}

		// теперь посмотрим, что есть
		if (cpos >= 0 && spos >= 0)
		{
			// есть оба символа
			// надо узнать, который из них раньше встретился.
			if (cpos < spos)
			{
				return cpos;
			}

			++clen;
			return spos;
		}

		// есть что-то одно либо ничего
		if (cpos >= 0)
		{
			return cpos;
		}

		if (spos >= 0)
		{
			++clen;
			return spos;
		}

		//иначе ничего нет
	}

	return -1;
}

bool CIni::ReadIni() {
	m_iniStatus = IniStatus::NOTREADED;
	m_iniError  = IniError::OK_NOERROR;
	if (m_strFileName.empty()) {
		return false;    // если имя файла не задано, то и читать нечего
	}
/* TODO:
	CStdioFile file;

	if (!file.Open(m_strFileName.c_str(), CFile::modeRead | CFile::shareDenyWrite | CFile::typeText))
	{
		m_iniError = IniError::READ_ERROR;
		return false;
	}

	CString strRead;
	CString strCurrentSectionName = _T("");
	m_LineNum = 0;

	while (file.ReadString(strRead))
	{
		m_LineNum++;

		if (strRead.IsEmpty())
		{
			continue;    // если пустая строка - игнорируем
		}

		strRead.Trim(); // уберём пустоту в начале и в конце строки
		const TCHAR chFch = strRead.GetAt(0);

		// пояснительные и философские комменты проигнорируем
		if (chFch == _T(';'))
		{
			continue;    // если комментарий - игнорируем
		}

		if (chFch == _T('/') && strRead.GetAt(1) == _T('/'))
		{
			continue;    // это тоже комментарий, тоже игнорируем
		}

		if (chFch == _T('[')) // мы уже убрали возможную пустоту перед [, поэтому этот символ обязательно должен быть первым, иначе это не имя секции
		{
			// начало секции
			const int pos = strRead.Find(_T(']')); // ищем конец секции

			if (pos > 0) // если этот символ найден
			{
				CString strName = strRead.Mid(1, pos - 1);
				strName.Trim(); // вот. мы даже допускаем многословное имя секции,
				// strName.Replace(_T(' '), _T('_')); // хотя можем и заменить пробелы на _
				strCurrentSectionName = strName; // имя текущей секции
				TRACE(_T("%#08X : %s\n"), GetHash(strCurrentSectionName), strCurrentSectionName);
				continue;
			}

			m_iniError = IniError::PARSE_ERROR;
			return false;
		}

		// сюда попадаем, если у нас простая строка имя=значение ;коммент
		int pos = strRead.Find(_T('='));
		CString strComment;
		CString strValue;
		CString strKey;

		// если строка не начинается с =, и = вообще есть
		if (pos > 0)
		{
			// разберёмся
			strKey = strRead.Left(pos++);
			strKey.Trim();
			strValue = strRead.Mid(pos);
			// проверим, вдруг есть комментарий
			int clen = 0;
			const int commentpos = CheckComment(strValue, clen);

			if (commentpos >= 0)
			{
				// если комментарий таки есть, то
				strComment = strValue.Mid(commentpos + clen); // вот комментарий
				strValue = strValue.Left(commentpos); // вот значение
			}

			strValue.Trim();
			strValue.Trim(_T('"')); // удалим ещё и кавычки (если есть)
		}
		else
		{
			// есть только ключ, без параметра
			// разберёмся
			strKey = strRead;
			// проверим, вдруг есть комментарий
			int clen = 0;
			const int commentpos = CheckComment(strRead, clen);

			if (commentpos >= 0)
			{
				// если комментарий таки есть, то
				strComment = strRead.Mid(commentpos + clen); // вот комментарий
				strKey = strRead.Left(commentpos); // вот значение
			}
		}

		Add(strCurrentSectionName, strKey, strValue, strComment, true);
		TRACE(_T("%#08X : %s\n"), GetHash(strKey), strKey);
	}

	file.Close();
	*/
	m_iniStatus = IniStatus::READED;
	return true;
}

bool CIni::FlushIni()
{
	if (m_iniStatus == IniStatus::NOTREADED // если ини не прочитан, то и записывать нечего
	        || m_iniStatus == IniStatus::FLUSHED   // если ини уже записан, то второй раз - не надо
	        || SetOfSections.empty()               // если инифайл пустой, то и делать нечего
	        || m_strFileName.empty()               // если имя файла не задано, то и записывать нечего
	   )
	{
		return false;
	}
/* TODO:
	CStdioFile file; // CMemFile - для создания файла в памяти

	if (!file.Open(m_strFileName.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeText))
	{
		m_iniError = IniError::WRITE_ERROR;
		return false;
	}

	m_LineNum = 0;
	CString strOut;

	for (auto &pISS : SetOfSections)
	{
		strOut = _T("[") + pISS.Name + _T("]\n");
		file.WriteString(strOut);
		m_LineNum++;

		for (auto &pIKS : pISS.Keys)
		{
			strOut = pIKS.Key;

			if (!pIKS.Value.IsEmpty())
			{
				strOut += _T(" = ") + pIKS.Value;
			}

			// если есть комментарий
			if (!pIKS.Comment.IsEmpty())
			{
				strOut += _T("\t;") + pIKS.Comment;
			}

			strOut += _T("\n");
			file.WriteString(strOut);
			m_LineNum++;
		}

		file.WriteString(_T("\n")); // и для красоты добавим пустую строку
		m_LineNum++;
	}

	file.Close();
	*/
	m_iniStatus = IniStatus::FLUSHED;
	m_iniError = IniError::OK_NOERROR;
	return true;
}

// возвращает размер файла в памяти
ULONGLONG CIni::FlushIniToMemory(uint8_t *pBuff, UINT nSize)
{
	if (m_iniStatus == IniStatus::NOTREADED     // если ини не прочитан, то pIKS.и записывать нечего
	        || SetOfSections.empty()                   // // если инифайл пустой, то и делать нечего
	   )
	{
		return 0;
	}
	const ULONGLONG len = 0;
/*TODO:
	CMemFile file(pBuff, nSize, 1024); // создаём файл в памяти
	m_LineNum = 0;
	CString strOut;

	for (auto &pISS : SetOfSections)
	{
		strOut = _T("[") + pISS.Name + _T("]\n");
		file.Write(strOut, strOut.GetLength() * sizeof(TCHAR));
		m_LineNum++;

		for (auto &pIKS : pISS.Keys)
		{
			strOut = pIKS.Key + _T(" = ") + pIKS.Value;

			// если есть комментарий
			if (!pIKS.Comment.IsEmpty())
			{
				strOut += _T("\t;") + pIKS.Comment;
			}

			strOut += _T("\n");
			file.Write(strOut, strOut.GetLength() * sizeof(TCHAR));
			m_LineNum++;
		}
	}

	const ULONGLONG len = file.GetLength();
	file.Close();
	*/
	m_iniStatus = IniStatus::FLUSHED;
	m_iniError = IniError::OK_NOERROR;
	return len;
}

bool CIni::ReadIniFromMemory(uint8_t *pBuff, UINT nSize)
{
	m_iniStatus = IniStatus::NOTREADED;
	m_iniError = IniError::OK_NOERROR;
	/*todo:
	CMemFile file(pBuff, nSize, 0); // создаём файл в памяти
	CString strRead;
	CString strCurrentSectionName = _T("");
	m_LineNum = 0;
	bool bEof = false;
	TCHAR tch;

	while (!bEof)
	{
		strRead = _T("");

		for (;;)
		{
			file.Read(&tch, sizeof(TCHAR));

			if (file.GetPosition() >= nSize)
			{
				bEof = true;
				break;
			}

			if (tch == L'\n')
			{
				break;
			}

			strRead += tch;
		}

		m_LineNum++;

		if (strRead.IsEmpty())
		{
			continue;    // если пустая строка - игнорируем
		}

		strRead.Trim(); // уберём пустоту в начале и в конце строки
		const TCHAR chFch = strRead.GetAt(0);

		if (chFch == _T('[')) // мы уже убрали возможную пустоту перед [, поэтому этот символ обязательно должен быть первым, иначе это не имя секции
		{
			// начало секции
			const int pos = strRead.Find(_T(']')); // ищем конец секции

			if (pos > 0) // если этот символ найден
			{
				CString strName = strRead.Mid(1, pos - 1);
				strName.Trim(); // вот. мы даже допускаем многословное имя секции,
				// strName.Replace(_T(' '), _T('_')); // хотя можем и заменить пробелы на _
				strCurrentSectionName = strName; // имя текущей секции
				TRACE(_T("%#08X : %s\n"), GetHash(strCurrentSectionName), strCurrentSectionName);
				continue;
			}

			m_iniError = IniError::PARSE_ERROR;
			return false;
		}

		// сюда попадаем, если у нас простая строка имя=значение ;коммент
		int pos = strRead.Find(_T('='));
		CString strComment;
		CString strValue;
		CString strKey;

		// если строка не начинается с =, и = вообще есть
		if (pos > 0)
		{
			// разберёмся
			strKey = strRead.Left(pos++);
			strKey.Trim();
			strValue = strRead.Mid(pos);
			// проверим, вдруг есть комментарий
			int clen = 0;
			const int commentpos = CheckComment(strValue, clen);

			if (commentpos >= 0)
			{
				// если комментарий таки есть, то
				strComment = strValue.Mid(commentpos + clen); // вот комментарий
				strValue = strValue.Left(commentpos); // вот значение
			}

			strValue.Trim();
			strValue.Trim(_T('"')); // удалим ещё и кавычки (если есть)
		}
		else
		{
			// есть только ключ, без параметра
			// разберёмся
			strKey = strRead;
			// проверим, вдруг есть комментарий
			int clen = 0;
			const int commentpos = CheckComment(strRead, clen);

			if (commentpos >= 0)
			{
				// если комментарий таки есть, то
				strComment = strRead.Mid(commentpos + clen); // вот комментарий
				strKey = strRead.Left(commentpos); // вот значение
			}
		}

		Add(strCurrentSectionName, strKey, strValue, strComment, true);
		TRACE(_T("%#08X : %s\n"), GetHash(strKey), strKey);
	}

	file.Close();
	*/
	m_iniStatus = IniStatus::READED;
	return true;
}

/* Внутренняя функция получения значения
Вход:   strSection - строковое значение имени секции
        strKey - строковое значение имени ключа
        strValue - переменная, куда выдаётся результат
Выход:  true - значение нашлось
        false - не нашлось
*/
bool CIni::_intGetValueString(const CString &strSection, const CString &strKey, CString &strValue)
{
	if (m_iniStatus == IniStatus::NOTREADED)
	{
		ReadIni();
	}

	const DWORD hashSection = GetHash(strSection);

	for (auto &pISS : SetOfSections)
	{
		if (pISS.Hash == hashSection)
		{
#ifdef DEBUG

			if (pISS.Name != strSection)
			{
				TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashSection, pISS.Name, strSection);
				ASSERT(false);
			}

#endif // DEBUG
			// нашли секцию
			const DWORD hashKey = GetHash(strKey);

			for (auto &pIKS : pISS.Keys)
			{
				if (pIKS.Hash == hashKey)
				{
#ifdef DEBUG

					if (pIKS.Key != strKey)
					{
						TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashKey, pIKS.Key, strKey);
						ASSERT(false);
					}

#endif // DEBUG
					// нашли ключ
					strValue = pIKS.Value;
					return true;
				}
			}

			// не нашли ключ в заданной секции - выходим
			return false;
		}
	}

	// не нашли нужную секцию - выходим
	return false;
}

bool CIni::DeleteKey(const CString &strSection, const CString &strKey)
{
	if (m_iniStatus == IniStatus::NOTREADED)
	{
		ReadIni();
	}

	const DWORD hashSection = GetHash(strSection);
	int nSectIndex = 0;

	for (auto &pISS : SetOfSections)
	{
		if (pISS.Hash == hashSection)
		{
#ifdef DEBUG

			if (pISS.Name != strSection)
			{
				TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashSection, pISS.Name, strSection);
				ASSERT(false);
			}

#endif // DEBUG
			// нашли секцию
			const DWORD hashKey = GetHash(strKey);
			int nKeyIndex = 0;

			for (auto &pIKS : pISS.Keys)
			{
				if (pIKS.Hash == hashKey)
				{
#ifdef DEBUG

					if (pIKS.Key != strKey)
					{
						TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashKey, pIKS.Key, strKey);
						ASSERT(false);
					}

#endif // DEBUG
					// нашли ключ, нужно его удалить
					pISS.Keys.erase(pISS.Keys.begin() + nKeyIndex);

					// если при этом это был единственный ключ
					if (pISS.Keys.empty())
					{
						// то нужно и саму секцию удалить
						SetOfSections.erase(SetOfSections.begin() + nSectIndex);
					}

					return true;
				}

				nKeyIndex++;
			}
		}

		nSectIndex++;
	}

	return false;
}

CString CIni::GetValueString(const CString &strSection, const CString &strKey, const CString &strDefault)
{
	CString strValue;

	if (_intGetValueString(strSection, strKey, strValue))
	{
		return strValue;
	}

	// не нашли. тогда надо его добавить
	SetValueString(strSection, strKey, strDefault);
	return strDefault;
}

CString CIni::GetValueString(int nSection, const CString &strKey, const CString &strDefault)
{
	CString strSection;

	if (strSection.LoadString(nSection))
	{
		return GetValueString(strSection, strKey, strDefault);
	}

	return strDefault;
}

CString CIni::GetValueString(const int nSection, const int nKey, const CString &strDefault)
{
	CString strSection, strKey;

	if (strSection.LoadString(nSection) && strKey.LoadString(nKey))
	{
		return GetValueString(strSection, strKey, strDefault);
	}

	return strDefault;
}


int CIni::GetValueInt(const int nSection, const int nKey, const int nDefault)
{
	int nReturn = nDefault;
	CString strReturn = GetValueString(nSection, nKey, CString(_T("")));

	if (!strReturn.IsEmpty())
	{
		nReturn = _ttoi(strReturn);
	}

	return nReturn;
}

double CIni::GetValueFloat(const int nSection, const int nKey, const double fDefault)
{
	double fReturn = fDefault;
	CString strReturn = GetValueString(nSection, nKey, CString(_T("")));

	if (!strReturn.IsEmpty())
	{
		fReturn = _ttof(strReturn);
	}

	return fReturn;
}

bool CIni::GetValueBool(const int nSection, const int nKey, const bool bDefault)
{
	bool bReturn = bDefault;
	CString strReturn = GetValueString(nSection, nKey, CString(_T("")));

	if (!strReturn.IsEmpty())
	{
		if ((!strReturn.CollateNoCase(_T("yes"))) || (!strReturn.Compare(_T("1"))))
		{
			bReturn = true;
		}
		else
		{
			bReturn = false;
		}
	}

	return bReturn;
}


bool CIni::SetValueString(const CString &strSection, const CString &strKey, const CString &strVal)
{
	if (Add(strSection, strKey, strVal, _T(""), false))
	{
		m_iniStatus = IniStatus::READED;
		return true;
	}

	return false;
}

bool CIni::SetValueString(int nSection, const CString &strKey, const CString &strVal)
{
	CString strSection;

	if (strSection.LoadString(nSection))
	{
		return SetValueString(strSection, strKey, strVal);
	}

	return false;
}

bool CIni::SetValueString(const int nSection, const int nKey, const CString &strVal)
{
	CString strSection, strKey;

	if (strSection.LoadString(nSection) && strKey.LoadString(nKey))
	{
		return SetValueString(strSection, strKey, strVal);
	}

	return false;
}


bool CIni::SetValueInt(const int nSection, const int nKey, const int iVal)
{
	CString str;
	str.Format(_T("%d"), iVal);
	return SetValueString(nSection, nKey, str);
}

bool CIni::SetValueFloat(const int nSection, const int nKey, const double fVal)
{
	CString str;
	str.Format(_T("%.16f"), fVal);
	return SetValueString(nSection, nKey, str);
}

bool CIni::SetValueBool(const int nSection, const int nKey, const bool bVal)
{
	CString str = bVal ? _T("Yes") : _T("No");
	return SetValueString(nSection, nKey, str);
}
/* TODO: ??
std::unique_ptr<CHAR> CIni::ConvertUnicodetoStr(UINT type, const CString &ustr) const
{
	// сперва узнаем, какого размера будет новая строка
	int t_len = WideCharToMultiByte(type, 0, ustr, -1, nullptr, 0, nullptr, nullptr);
	const int sizeOfString = t_len + 1; // и +1 для завершающего нуля
	auto lpszAscii = std::make_unique<CHAR>(sizeOfString); // а потом, необходимо самим делать delete [] использованных строк

	if (lpszAscii)
	{
		t_len = WideCharToMultiByte(type, 0, ustr, -1, lpszAscii.get(), sizeOfString, nullptr, nullptr);
	}

	return lpszAscii;
}

CString CIni::ConvertStrtoUnicode(UINT type, LPCSTR astr) const
{
	// сперва узнаем, какого размера будет новая строка
	int t_len = MultiByteToWideChar(type, 0, astr, -1, nullptr, 0);
	const int sizeOfString = t_len + 1; // и +1 для завершающего нуля
	// вот тут будет строка
	CString sret;
	// конвертируем
	t_len = MultiByteToWideChar(type, 0, astr, -1, sret.GetBufferSetLength(sizeOfString), sizeOfString);
	sret.ReleaseBuffer();
	return sret;
}
*/

// TODO: .rc into flash
#include "resource.h"
bool CString :: LoadString(unsigned int nID) { // A Windows string resource ID.
  ///  DBGM_PRINT(("CString :: LoadString(%d (%Xh))", nID, nID));
    switch(nID) {
		case 0:
		  _in("0100000");
		  return true;
		case IDS_INI_CPU_RUN_ADDR:
		  return true;
		default:
		  DBGM_PRINT(("CString :: LoadString(%d (%Xh)) not found", nID, nID));
		  return false;
	}
}

// расширенный функционал. С кастомизацией
CString CIni::GetValueStringEx(const CString &strCustomName, const int nSection, const int nKey, const CString &strDefault)
{
	DBGM_PRINT(("CIni::GetValueStringEx(strCustomName = %s, nSection = %d, nKey = %d, strDefault = %s)",
                                        strCustomName.GetString(), nSection,    nKey, strDefault.GetString()));
	CString strSection, strKey, strValue;
	strSection.LoadString(nSection);
	strKey.LoadString(nKey);
	CString strCustomSection = strSection + _T(".") + strCustomName;

	// сперва ищем значение в кастомной секции
	if (_intGetValueString(strCustomSection, strKey, strValue))
	{
		// если нашли
		return strValue; // вернём что нашли
	}

	// если не нашли, поищем в базовой секции
	if (_intGetValueString(strSection, strKey, strValue))
	{
		// если нашли
		return strValue; // вернём что нашли
	}

	// если и там не нашли, тогда надо его добавить
	if (Add(strSection, strKey, strDefault, _T(""), false))
	{
		m_iniStatus = IniStatus::READED;
	}

	return strDefault;
}

int CIni::GetValueIntEx(const CString &strCustomName, const int nSection, const int nKey, const int nDefault)
{
	int nReturn = nDefault;
	CString strDef;
	strDef.Format(_T("%d"), nDefault);
	CString strReturn = GetValueStringEx(strCustomName, nSection, nKey, strDef);

	if (!strReturn.IsEmpty())
	{
		nReturn = _ttoi(strReturn);
	}

	return nReturn;
}

double CIni::GetValueFloatEx(const CString &strCustomName, const int nSection, const int nKey, const double fDefault)
{
	double fReturn = fDefault;
	CString strDef;
	strDef.Format(_T("%.16f"), fDefault);
	CString strReturn = GetValueStringEx(strCustomName, nSection, nKey, strDef);

	if (!strReturn.IsEmpty())
	{
		fReturn = _ttof(strReturn);
	}

	return fReturn;
}

bool CIni::GetValueBoolEx(const CString &strCustomName, const int nSection, const int nKey, const bool bDefault)
{
	bool bReturn = bDefault;
	CString strReturn = GetValueStringEx(strCustomName, nSection, nKey, (bDefault ? _T("Yes") : _T("No")));

	if (!strReturn.IsEmpty())
	{
		if ((!strReturn.CollateNoCase(_T("yes"))) || (!strReturn.Compare(_T("1"))))
		{
			bReturn = true;
		}
		else
		{
			bReturn = false;
		}
	}

	return bReturn;
}

bool CIni::SetValueStringEx(const CString &strCustomName, const int nSection, const int nKey, const CString &strVal, bool bForce)
{
	// флаг bForce - принудительно писать в кастомную секцию
	CString strSection, strKey;

	if (strSection.LoadString(nSection) && strKey.LoadString(nKey))
	{
		CString strCustomSection = strSection + _T(".") + strCustomName;

		if (!bForce)    // если сохранять в кастомную секцию только по необходимости
		{
			CString strOldValue;
			// читаем значение из стандартной секции
			bool bReaded = _intGetValueString(strSection, strKey, strOldValue);

			// если прочиталось
			if (bReaded)
			{
				// и оно такое, же какое мы хотим сохранить
				if (strOldValue == strVal)
				{
					// то его вообще необязательно сохранять, а из кастомной секции старое
					// значение, какое бы оно ни было, нужно удалить
					// на всякий случай прочитаем значение кастомной секции
					bReaded = _intGetValueString(strCustomSection, strKey, strOldValue);

					if (bReaded)
					{
						// если прочиталось, то значение из кастомной секции можно удалить
						return DeleteKey(strCustomSection, strKey);
					}

					// если не прочиталось, то там и не было ничего
					return true; // значит и не надо, в стандартной секции же есть.
					// тут вообще можно удалять не читая, если такого параметра нет, DeleteKey
					// возвратит false, поэтому нужно делать
					// DeleteKey(strCustomSection, strKey);
					// return true;
				}

				// если не такое, то новое значение сохраним в кастомной секции
			}
			else // если не прочиталось, то надо сохранять.
			{
				if (Add(strSection, strKey, strVal, _T(""), false))
				{
					m_iniStatus = IniStatus::READED;
					return true;
				}

				return false;
			}
		}

		// сюда попадаем, если нужно принудительно сохранять значение в кастомную секцию,
		// или оно не совпадает со значением в основной секции
		if (Add(strCustomSection, strKey, strVal, _T(""), false))
		{
			m_iniStatus = IniStatus::READED;
			return true;
		}
	}

	return false;
}

bool CIni::SetValueIntEx(const CString &strCustomName, const int nSection, const int nKey, const int iVal, bool bForce)
{
	CString str;
	str.Format(_T("%d"), iVal);
	return SetValueStringEx(strCustomName, nSection, nKey, str, bForce);
}

bool CIni::SetValueFloatEx(const CString &strCustomName, const int nSection, const int nKey, const double fVal, bool bForce)
{
	CString str;
	str.Format(_T("%.16f"), fVal);
	return SetValueStringEx(strCustomName, nSection, nKey, str, bForce);
}

bool CIni::SetValueBoolEx(const CString &strCustomName, const int nSection, const int nKey, const bool bVal, bool bForce)
{
	CString str = bVal ? _T("Yes") : _T("No");
	return SetValueStringEx(strCustomName, nSection, nKey, str, bForce);
}

bool CIni::CopySection(CIni *pSrcIni, const CString &strSectionName)
{
	const DWORD hashSection = GetHash(strSectionName);

	for (auto &pISS : pSrcIni->SetOfSections)
	{
		if (pISS.Hash == hashSection)
		{
#ifdef DEBUG

			if (pISS.Name != strSectionName)
			{
				TRACE(_T("HASH_COLLISION:: %#08X : \"%s\" and \"%s\" \n"), hashSection, pISS.Name, strSectionName);
				ASSERT(false);
			}

#endif // DEBUG
			// нашли секцию, теперь добавим все ключи

			for (auto &pIKS : pISS.Keys)
			{
				Add(strSectionName, pIKS.Key, pIKS.Value, pIKS.Comment);
			}

			m_iniStatus = IniStatus::READED;
			return true;
		}
	}

	return false;
}

bool CIni::FindKeyStart(const CString &strSection, CString &strKey, CString &strValue)
{
	if (m_iniStatus == IniStatus::NOTREADED)
	{
		ReadIni();
	}

	const DWORD hashSection = GetHash(strSection);
	int nSectIdx = 0;

	for (auto &pISS : SetOfSections)
	{
		if (pISS.Hash == hashSection)
		{
			// нашли секцию
			m_nFindSectIdx = nSectIdx;
			m_nFindKeyIdx = 0;
			return FindKeyNext(strKey, strValue);
		}

		nSectIdx++;
	}

	strKey.Empty();
	strValue.Empty();
	m_nFindSectIdx = -1;
	m_nFindKeyIdx = -1;
	return false;
}

bool CIni::FindKeyNext(CString &strKey, CString &strValue)
{
	if (m_nFindSectIdx >= 0) // если секция найдена, то можно перебирать ключи
	{
		if (m_nFindKeyIdx >= 0) // если ключи ещё не кончились
		{
			// выдаём очередное значение
			strKey = SetOfSections[m_nFindSectIdx].Keys[m_nFindKeyIdx].Key;
			strValue = SetOfSections[m_nFindSectIdx].Keys[m_nFindKeyIdx].Value;
			m_nFindKeyIdx++; // переходим к следующему

			if (m_nFindKeyIdx >= int(SetOfSections[m_nFindSectIdx].Keys.size()))
			{
				m_nFindKeyIdx = -1; // если закончились, то при следующем вызове будет false
			}

			return true;
		}

		m_nFindSectIdx = -1;
	}

	strKey.Empty();
	strValue.Empty();
	m_nFindKeyIdx = -1;
	return false;
}
