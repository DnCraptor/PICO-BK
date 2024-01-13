#include "pch.h"
#include "BKFloppyImage_RT11.h"
#include "StringUtil.h"

#include <ctime>

#pragma warning(disable:4996)

const char_t CBKFloppyImage_RT11::RADIX50[050] =
{
	// 000..007
	' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	// 010..017
	'H', 'I', 'J', 'K', ' ', 'M', 'N', 'O',
	// 020..027
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	// 030..037
	'X', 'Y', 'Z', '$', '.', ' ', '0', '1',
	// 040..047
	'2', '3', '4', '5', '6', '7', '8', '9'
};

CBKFloppyImage_RT11::CBKFloppyImage_RT11(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_nBeginBlock(6)
	, m_nTotalSegments(0)
	, m_nExtraBytes(0)
{
	m_pKoi8tbl = imgUtil::koi8tbl11M;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
}


CBKFloppyImage_RT11::~CBKFloppyImage_RT11()
{
	m_RT11Catalog.clear();
}

const std::string CBKFloppyImage_RT11::GetSpecificData(BKDirDataItem *fr) const
{
	// поле под месяц - 4 бита. верные значения: 1..12, остальные - ошибочные
	// чтобы не усложнять метод вывода, просто добавим в массив несуществующие месяцы,
	// при ошибке всё будет хорошо видно
	static const char *strMons[16] =
	{
		"Nul",
		"Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug",
		"Sen", "Oct", "Nov", "Dec",
		"?13", "?14", "?15"
	};
	auto rt11fr = reinterpret_cast<RT11FileRecord *>(fr->pSpecificData);
	int nDay = (rt11fr->nFileDate >> 5) & 0x1f;
	int nMon = (rt11fr->nFileDate >> 10) & 0xf;
	int nYear = (rt11fr->nFileDate & 0x1f) + 1972 + ((rt11fr->nFileDate >> 14) & 3) * 32;
	std::string strDate;

	if (nDay && nMon)
	{
		// если день != 0 и месяц != 0, то будем считать, что поле даты верно.
		strDate = imgUtil::string_format("%02d-%s-%4d\0", nDay, strMons[nMon], nYear);
	}

	return strDate;
}

/*
декодирование строки в radix50.
вход: buf - массив слов,
len - размер буфера в словах
*/
std::string CBKFloppyImage_RT11::DecodeRadix50(uint16_t *buf, int len)
{
	std::string ret;

	for (int i = 0; i < len; ++i)
	{
		uint16_t w = buf[i];
		// будем решать в лоб, не будем делать изящный алгоритм
		char_t tch3 = RADIX50[w % 050];
		w /= 050;
		char_t tch2 = RADIX50[w % 050];
		w /= 050;
		char_t tch1 = RADIX50[w % 050];
		ret.push_back(tch1);
		ret.push_back(tch2);
		ret.push_back(tch3);
	}

	return ret;
}
/*
 * Закодирование строки в код radix50
 * вход: buf - массив слов, куда сохраняется результат,
 * len - размер буфера в словах
 * strSrc - строка, которую закодировываем
   выход: размер закодированного результата в словах, по любому не больше len
   если строка не влазит в буфер, тем хуже для неё, всё, что не влазит - отбрасывается
 **/
int CBKFloppyImage_RT11::EncodeRadix50(uint16_t *buf, int len, std::string &strSrc)
{
	int nRes = 0;   // результат
	uint16_t w = 0; // текущее слово в radix50
	int nCnt = 0;   // счётчик символов в слове

	// сперва очистим буфер от старого имени.
	for (int i = 0; i < len; ++i)
	{
		buf[i] = 0;
	}

	for (auto ch : strSrc)
	{
		// теперь поищем символ в таблице
		uint16_t nChCode = 0;   // код текущего символа по умолчанию, если символ не будет найден в таблице, будет значение по умолчанию

		for (int n = 0; n < 050; ++n)
		{
			if (RADIX50[n] == ch)
			{
				nChCode = n;    // нашли
				break;
			}
		}

		w = w * 050 + nChCode;  // упаковываем очередной символ

		if (++nCnt == 3)        // если пора сохранять полное слово
		{
			nCnt = 0;

			if (nRes < len)     // если есть ещё куда сохранять
			{
				buf[nRes++] = w; // сохраним
			}
			else
			{
				break;  // иначе выйдем из цикла
			}

			w = 0;
		}
	}

	// строка кончилась, и если есть что-то несохранённое, сохраним
	if (nCnt)
	{
		while (nCnt < 3)
		{
			w = w * 050;
			nCnt++;
		}

		if (nRes < len) // если есть ещё куда сохранять
		{
			buf[nRes++] = w; // сохраним
		}
	}

	// чё-то какой-то слишком сложный алгоритм получился
	return nRes;
}

/* читаем каталог
заполняем переменные каталога.
*/
bool CBKFloppyImage_RT11::ReadRT11Catalog()
{
	uint16_t Segment[SEGMENT_SIZE] {}; // буфер сегмента в словах
	m_RT11Catalog.clear();

	// Перемещаемся к 1му сектору и читаем первый сектор, хотя нам надо оттуда всего 1 слово
	if (!SeektoBlkReadData(1, m_nSector, sizeof(m_nSector)))
	{
		return false;
	}

	m_nBeginBlock = *(reinterpret_cast<uint16_t *>(m_nSector + 0724)); // блок, с которого начинается каталог берём из служебной области,
	// и если там неправильное значение, тем хуже для образа

	// !!! фикс для АДОС
	if ((m_nBeginBlock == 0) || (m_nBeginBlock > 255))
	{
		m_nBeginBlock = 6;
	}

	int nUsedSegments = 0;  // занято сегментов в каталоге
	int nSegment = 1;       // текущий сегмент каталога, отсчёт ведётся с 1, т.к. 0 - признак конца цепочки сегментов
	auto pSegHdr = reinterpret_cast<RT11SegmentHdr *>(Segment); // заголовок сегмента
	RT11FileRecordEx RT11rec;
	int nRecSizeWord = 7 + ((m_nExtraBytes + 1) >> 1);      // размер записи в словах, предполагается, что количество дополнительных слов везде одинаково
	int nSpecificDataLength = nRecSizeWord * 2; // размер записи в байтах
	int nRecsNum = (SEGMENT_SIZE - 5) / nRecSizeWord;    // общее количество записей, которое помещается в сегмент (все числа - кол-во слов)
	uint16_t nStartFileBlock = 0;

	// можно приступать к чтению каталога
	while (nSegment > 0)
	{
		int nBlock = m_nBeginBlock + (nSegment - 1) * 2;

		// переместимся к нужному сегменту
		// прочитаем весь сегмент
		if (!SeektoBlkReadData(nBlock, Segment, sizeof(Segment)))
		{
			return false;
		}

		if (nSegment == 1) // берём данные только из первого сегмента
		{
			m_nExtraBytes = pSegHdr->filerecord_add_bytes;
			m_nTotalSegments = pSegHdr->segments_num; // всего возможных сегментов в каталоге

			if (m_nTotalSegments > 31)
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
				return false;
			}

			m_sDiskCat.nDataBegin = nStartFileBlock = pSegHdr->file_block;
		}

		// теперь заполним вектор записями

		for (int i = 0; i < nRecsNum; ++i)
		{
			if (Segment[5 + i * nRecSizeWord] & (RT11_FILESTATUS_EOS << 8))
			{
				break; // если конец сегмента, то запись обрабатывать не надо
			}

			RT11rec.clear();
			memcpy(&RT11rec, &Segment[5 + i * nRecSizeWord], nSpecificDataLength);
			RT11rec.BeginBlock = nStartFileBlock;
			m_RT11Catalog.push_back(RT11rec);
			nStartFileBlock += RT11rec.Record.nBlkSize;
		}

		nSegment = pSegHdr->next_segment;
		nUsedSegments++;
	}

	return true;
}
/*
А теперь наоборот. Сохраняем каталог.
причём сохраняем заполняя сегменты по максимуму
*/
bool CBKFloppyImage_RT11::WriteRT11Catalog()
{
	uint16_t Segment[SEGMENT_SIZE] {}; // буфер сегмента в словах
	unsigned int nSegment = 1;      // текущий сегмент каталога, отсчёт ведётся с 1, т.к. 0 - признак конца цепочки сегментов
	auto pSegHdr = reinterpret_cast<RT11SegmentHdr *>(Segment); // заголовок сегмента
	int nRecSizeWord = 7 + ((m_nExtraBytes + 1) >> 1);      // размер записи в словах, предполагается, что количество дополнительных слов везде одинаково
	int nRecsNum = (SEGMENT_SIZE - 5) / nRecSizeWord - 2;   // общее количество записей, которое помещается в сегмент (все числа - кол-во слов)- 2
	// - это одна запись empty.fil в конце каталога, и одна запись - признак конца сегмента
	int nSpecificDataLength = nRecSizeWord * 2; // размер записи в байтах
	// надо бы рассчитать, сколько сегментов будет занимать каталог.
	unsigned int nUsedSegments = static_cast<unsigned int>(m_RT11Catalog.size()) / nRecsNum;    // занято сегментов в каталоге

	if (m_RT11Catalog.size() % nRecsNum) // если ещё есть остаток
	{
		nUsedSegments++; // то он в следующий сегмент помещается.
	}

	int nCurrRecNum = 0; // счётчик записей в сегменте
	uint16_t nFileBlock = m_nBeginBlock + m_nTotalSegments * 2; // это будет начальный блок сегмента
	bool bBeginPrepare = true; // флаг, что надо подготовить новый сегмент

	for (auto &p : m_RT11Catalog)
	{
		if (bBeginPrepare)
		{
			bBeginPrepare = false;
			memset(Segment, 0, sizeof(Segment));
			// заполним заголовок, тем что известно
			pSegHdr->segments_num = m_nTotalSegments; // количество выделенных сегментов под каталог
			pSegHdr->next_segment = (nSegment >= nUsedSegments) ? 0 : nSegment + 1; // следующий сегмент
			pSegHdr->used_segments = nUsedSegments; // количество используемых сегментов
			pSegHdr->filerecord_add_bytes = m_nExtraBytes; // количество дополнительных байтов
			pSegHdr->file_block = nFileBlock;
			nCurrRecNum = 0;
		}

		memcpy(&Segment[5 + nCurrRecNum * nRecSizeWord], std::addressof(p), nSpecificDataLength);
		nFileBlock += p.Record.nBlkSize;

		if (++nCurrRecNum >= (nRecsNum + 1)) // самой последней записью, или словом в сегменте будет признак конца сегмента
		{
			Segment[5 + nCurrRecNum * nRecSizeWord] = (RT11_FILESTATUS_EOS << 8);
			// и сохраним сегмент.
			unsigned int nBlock = m_nBeginBlock + (nSegment - 1) * 2;

			// переместимся к нужному сегменту
			// и запишем весь сегмент
			if (!SeektoBlkWriteData(nBlock, Segment, sizeof(Segment)))
			{
				return false;
			}

			nSegment++;
			bBeginPrepare = true;
		}
	}

	// если вышли из цикла for не сохранив сегмент, а такое всегда случается,
	// если последний сегмент заполнен не полностью.
	if (!bBeginPrepare)
	{
		Segment[5 + nCurrRecNum * nRecSizeWord] = (RT11_FILESTATUS_EOS << 8);
		// и сохраним сегмент.
		unsigned int nBlock = m_nBeginBlock + (nSegment - 1) * 2;

		// переместимся к нужному сегменту
		// и запишем весь сегмент
		if (!SeektoBlkWriteData(nBlock, Segment, sizeof(Segment)))
		{
			return false;
		}
	}

	return true;
}


int CBKFloppyImage_RT11::FindRecord(RT11FileRecord *pRec, bool bFull)
{
	size_t sz = m_RT11Catalog.size();

	for (size_t i = 0; i < sz; ++i)
	{
		auto pRT11Record = reinterpret_cast<RT11FileRecord *>(&m_RT11Catalog[i]);

		if (pRT11Record->nStatus & (RT11_FILESTATUS_UNUSED | RT11_FILESTATUS_TEMPORARY))
		{
			// неиспользуемые и временные игнорируем
			continue;
		}

		if ((pRT11Record->FileName[0] == pRec->FileName[0]) && (pRT11Record->FileName[1] == pRec->FileName[1]) // сравниваем имя
		        && (pRT11Record->FileExt == pRec->FileExt)) // расширение
		{
			// если имя с расширением совпало
			if (bFull) // если полное сравнение
			{
				// то ещё и другие параметры проверим
				if ((pRT11Record->nBlkSize == pRec->nBlkSize) // начальный блок
				        && (pRT11Record->nFileDate == pRec->nFileDate)) // дату создания)
				{
					return static_cast<int>(i); // если это та запись что нам нужна
				}
			}
			else
			{
				return static_cast<int>(i); // если нам важно только имя
			}
		}
	}

	return -1;
}

void CBKFloppyImage_RT11::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<RT11FileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(RT11FileRecord);
			memset(pRec, 0, sizeof(RT11FileRecord));
		}

		// теперь сформируем РТ11шную запись из абстрактной
		std::string str = strUtil::strToUpper(pFR->strName.stem().string());
		str = strUtil::CropStr(str, 6);
		EncodeRadix50(pRec->FileName, 2, str);
		str = pFR->strName.extension().string();

		if (!str.empty())
		{
			str = strUtil::CropStr(str.substr(1), 3); // без точки
		}

		str = strUtil::strToUpper(str);
		EncodeRadix50(&pRec->FileExt, 1, str);

		if (!bRenameOnly)
		{
			pRec->nBlkSize = ByteSizeToBlockSize_l(pFR->nSize); // размер проги в блоках
			tm ctm{};
			gmtime_s(&ctm, &pFR->timeCreation);
			int year = (ctm.tm_year + 1900) > 1972 ? ctm.tm_year + 1900 - 1972 : 0;
			pRec->nFileDate = (((ctm.tm_mon & 0xf) + 1) << 10) | ((ctm.tm_mday & 0x1f) << 5) | (year & 0x1f) | ((year & 0x60) << 9);
		}
	}
}

void CBKFloppyImage_RT11::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<RT11FileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовываем, только если есть реальные данные
	if (pFR->nSpecificDataLength)
	{
		pFR->strName = strUtil::trim(DecodeRadix50(pRec->FileName, 2));
		std::string ext = strUtil::trim(DecodeRadix50(&(pRec->FileExt), 1));

		if (!ext.empty())
		{
			pFR->strName += "." + ext;
		}

		if (pFR->strName.empty())
		{
			pFR->strName = "empty.fi";
		}

		// разберёмся с атрибутами.
		if (pRec->nStatus & RT11_FILESTATUS_TEMPORARY)
		{
			pFR->nAttr |= FR_ATTR::TEMPORARY;
		}

		if (pRec->nStatus & RT11_FILESTATUS_PROTECTED)
		{
			pFR->nAttr |= FR_ATTR::PROTECTED;
		}

		if (pRec->nStatus & RT11_FILESTATUS_READONLY)
		{
			pFR->nAttr |= FR_ATTR::READONLY;
		}

		if (pRec->nStatus & RT11_FILESTATUS_UNUSED)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
			std::string t = pFR->strName.string();
			t[0] = 'x';
			pFR->strName = fs::path(t);
		}

		if (ext == "DSK")
		{
			pFR->nAttr |= FR_ATTR::LOGDISK;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::LOGDSK;
		}
		else
		{
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
		}

		pFR->nDirBelong = 0; // директорий нет
		pFR->nDirNum = 0;
		pFR->nAddress = 0; // в рт11 у файла нету адреса загрузки, т.е. есть, но он не в каталоге хранится.
		pFR->nSize = pRec->nBlkSize * m_nBlockSize;
		pFR->nBlkSize = pRec->nBlkSize;
		// обратная операция для времени
		tm ctm{};
		memset(&ctm, 0, sizeof(tm));
		ctm.tm_mday = (pRec->nFileDate >> 5) & 0x1f;
		ctm.tm_mon = ((pRec->nFileDate >> 10) & 0x0f) - 1;
		ctm.tm_year = ((pRec->nFileDate >> 9) & 0x60) | (pRec->nFileDate & 0x1f) + 1972 - 1900;

		if (ctm.tm_year < 0)
		{
			ctm.tm_year = 0;
		}

		pFR->timeCreation = mktime(&ctm);
	}
}


bool CBKFloppyImage_RT11::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	if (!ReadRT11Catalog())
	{
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	BKDirDataItem AFR;
	auto pRT11Record = reinterpret_cast<RT11FileRecordEx *>(AFR.pSpecificData);
	unsigned int nUsedRecords = 0;  // занято записей в каталоге
	unsigned int nUsedSize = 0;     // занято блоков файлами в каталоге
	int nRecSizeWord = 7 + ((m_nExtraBytes + 1) >> 1);      // размер записи в словах, предполагается, что количество дополнительных слов везде одинаково
	int nSpecificDataLength = nRecSizeWord * 2; // размер записи в байтах

	for (auto &p : m_RT11Catalog)
	{
		AFR.clear();
		AFR.nSpecificDataLength = nSpecificDataLength;
		// тут как-то не очень эффективно. мы храним копию записи в абстрактной записи.
		memcpy(pRT11Record, std::addressof(p), nSpecificDataLength);
		ConvertRealToAbstractRecord(&AFR); // здесь блок начала файла не вычисляется, т.к. его нет в записи о файле
		nUsedRecords++;
		AFR.nStartBlock = p.BeginBlock;

		if (!(AFR.nAttr & FR_ATTR::DELETED))
		{
			nUsedSize += pRT11Record->Record.nBlkSize;
		}

		m_sDiskCat.vecFC.push_back(AFR);
	}

	// рассчитаем по официальной формуле
	m_sDiskCat.nTotalRecs = m_nTotalSegments * ((SEGMENT_SIZE - 5) / nRecSizeWord - 3);  // всего возможных записей в каталоге
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - nUsedRecords;    // сколько записей в каталоге свободно

	// (при полностью заполненном каталоге ОСБК, может случиться казус)
	if (m_sDiskCat.nFreeRecs < 0)
	{
		m_sDiskCat.nTotalRecs -= m_sDiskCat.nFreeRecs; // к общему количеству записей прибавим переизбыток
		m_sDiskCat.nFreeRecs = 0; // а свободных - укажем 0
	}

	int nBlkSize = EvenSizeByBlock_l((int)m_sParseImgResult.nImageSize); // выровняем по границе блока
	nBlkSize >>= 9; // получим размер образа в блоках
	nBlkSize -= (m_nBeginBlock + m_nTotalSegments * 2);
	// будем верить в то, что если каталог читался, то за каталогом данные всё-таки есть
	m_sDiskCat.nTotalBlocks = nBlkSize;
	m_sDiskCat.nFreeBlocks = nBlkSize - nUsedSize;  // сколько блоков на диске свободно.
	// поскольку размер логического диска нигде в нём самом не задаётся, то он
	// вычисляется по размеру файла образа, и если файл оборван - то и размер будет неправильный.
	return true;
}

bool CBKFloppyImage_RT11::WriteCurrentDir()
{
	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	// предполагается, что мы не меняем абстрактные записи, а работаем с каталогом,
	// а абстрактные записи - оперативное зеркало реального каталога
	return WriteRT11Catalog();
}



bool CBKFloppyImage_RT11::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;

	// если директория - ничего не делаем,  плохой или удалённый файл - читаем как обычно
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		return bRet;
	}

	ConvertAbstractToRealRecord(pFR);
	size_t len = pFR->nSize;
	size_t sector = pFR->nStartBlock;

	if (SeekToBlock(sector)) // перемещаемся к началу файла
	{
		// поскольку файл располагается единым куском, прочитаем его целиком
		if (len > 0)
		{
			if (!ReadData(pBuffer, len))
			{
				m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
				bRet = false;
			}
		}
	}
	else
	{
		bRet = false;
	}

	return bRet;
}

bool CBKFloppyImage_RT11::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	// если попадаем сюда с такой ошибкой - то мы в цикле пытаемся просквизировать диск
	if (bNeedSqueeze)
	{
		if (!Squeeze()) // если сквизирование неудачное
		{
			bNeedSqueeze = false;
			return false; // то выходим с ошибкой
		}
	}

	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return false; // то записать в него мы ничего не сможем.
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<RT11FileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pRec->nStatus |= RT11_FILESTATUS_PERMANENT; // нужно установить флаг, что это постоянный файл
	pRec->nStatus &= ~(RT11_FILESTATUS_UNUSED | RT11_FILESTATUS_TEMPORARY); // а это на всякий случай удалить

	if (m_sDiskCat.nFreeBlocks < pRec->nBlkSize)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
		return false;
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	// найти запись с таким именем в каталоге
	int nIndex = FindRecord(pRec, false);

	if (nIndex >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST; // если файл существует - выходим с ошибкой
		*pRec = m_RT11Catalog[nIndex].Record;
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	// запись файла.
	bool bRet = false;
	// стратегия такая: ищем место в конце, если есть - пишем туда.
	auto last = m_RT11Catalog.end() - 1; // берём последнюю запись

	if (last->Record.nBlkSize >= pRec->nBlkSize)
	{
		RT11FileRecordEx nr = *last; // скопируем последнюю запись
		nr.Record.nBlkSize -= pRec->nBlkSize; // размер свободной области уменьшим на размер файла
		nr.BeginBlock += pRec->nBlkSize;
		last->Record = *pRec;

		if (m_nExtraBytes)
		{
			memset(last->ExtraWord, 0, m_nExtraBytes);
		}

		size_t nBlock = last->BeginBlock;
		m_RT11Catalog.push_back(nr); // добавим новое неиспользуемое пространство

		if (SeekToBlock(nBlock))
		{
			int nReaded;
			int nLen = pFR->nSize; // размер файла

			while (nLen > 0)
			{
				memset(m_mBlock, 0, COPY_BLOCK_SIZE);
				nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
				memcpy(m_mBlock, pBuffer, nReaded);
				pBuffer += nReaded;

				// пишем с выравниванием по секторам.
				if (nReaded > 0)
				{
					if (!WriteData(m_mBlock, EvenSizeByBlock_l(nReaded)))
					{
						return false;
					}
				}

				nLen -= nReaded;
			}

			if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
			{
				// наконец сохраняем каталог
				bRet = WriteCurrentDir();
				// чтобы каталог заново не перечитывать, корректируем данные
				m_sDiskCat.nFreeBlocks -= pRec->nBlkSize;
				m_sDiskCat.nFreeRecs--;
			}
		}

		return bRet;
	}

	// если нет - ищем дырку подходящего размера, если есть - пишем туда.

	for (auto p = m_RT11Catalog.begin(); p != m_RT11Catalog.end(); ++p)
	{
		if (p->Record.nStatus & RT11_FILESTATUS_UNUSED)
		{
			if (p->Record.nBlkSize >= pRec->nBlkSize)
			{
				RT11FileRecordEx nr = *p; // скопируем последнюю запись
				nr.Record.nBlkSize -= pRec->nBlkSize; // размер свободной области уменьшим на размер файла
				nr.BeginBlock += pRec->nBlkSize;
				p->Record = *pRec;

				if (m_nExtraBytes)
				{
					memset(p->ExtraWord, 0, m_nExtraBytes);
				}

				size_t nBlock = p->BeginBlock;

				if (nr.Record.nBlkSize > 0)
				{
					int nRecSizeWord = 7 + ((m_nExtraBytes + 1) >> 1);          // размер записи в словах, предполагается, что количество дополнительных слов везде одинаково
					size_t nRecsNum = static_cast<size_t>(SEGMENT_SIZE - 5) / nRecSizeWord - 2;    // общее количество записей, которое помещается в сегмент (все числа - кол-во слов)

					if (m_RT11Catalog.size() > nRecsNum * m_nTotalSegments)
					{
						m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
						goto l_sqeeze;
					}
					else
					{
						m_RT11Catalog.insert(p + 1, nr); // добавим новое неиспользуемое пространство
					}
				}

				if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
				{
					if (SeekToBlock(nBlock))
					{
						int nReaded;
						int nLen = pFR->nSize; // размер файла

						while (nLen > 0)
						{
							memset(m_mBlock, 0, COPY_BLOCK_SIZE);
							nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
							memcpy(m_mBlock, pBuffer, nReaded);
							pBuffer += nReaded;

							// пишем с выравниванием по секторам.
							if (nReaded > 0)
							{
								if (!WriteData(m_mBlock, EvenSizeByBlock_l(nReaded)))
								{
									return false;
								}
							}

							nLen -= nReaded;
						}

						if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
						{
							// наконец сохраняем каталог
							bRet = WriteCurrentDir();
							// чтобы каталог заново не перечитывать, корректируем данные
							m_sDiskCat.nFreeBlocks -= pRec->nBlkSize;
							m_sDiskCat.nFreeRecs--;
						}
					}
				}

				return bRet;
			}
		}
	}

	m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
l_sqeeze:
	// если нет дырки, но суммарное свободное место на диске есть - делаем сквизирование и пишем в конец.
	bNeedSqueeze = true;
	return false;
}

bool CBKFloppyImage_RT11::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	// если директория - ничего не делаем
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		return bRet;
	}

	ConvertAbstractToRealRecord(pFR);
	// найти запись в каталоге
	int nPos = FindRecord(reinterpret_cast<RT11FileRecord *>(pFR->pSpecificData), true);

	if (nPos >= 0)
	{
		// идея такая:
		// Сперва пытаемся удалить файл просто так. Если выходим с ошибкой IMAGE_ERROR_FS_FILE_PROTECTED
		// запрашиваем подтверждение на удаление и если да, то вызываем эту функцию с флагом bForce
		// чтобы удалить защищённый файл
		if ((m_RT11Catalog[nPos].Record.nStatus & RT11_FILESTATUS_PROTECTED) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			// пометить как удалённый.
			m_RT11Catalog[nPos].Record.nStatus &= ~RT11_FILESTATUS_PERMANENT; // этот атрибут надо удалять обязательно
			m_RT11Catalog[nPos].Record.nStatus |= RT11_FILESTATUS_UNUSED; // потому что установка этого - недостаточна
			// сохранить каталог.
			bRet = WriteCurrentDir();
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

bool CBKFloppyImage_RT11::ChangeDir(BKDirDataItem *pFR)
{
	if (CBKFloppyImage_Prototype::ChangeDir(pFR))
	{
		return true;
	}

	if ((pFR->nAttr & FR_ATTR::LOGDISK) && (pFR->nRecType == BKDirDataItem::RECORD_TYPE::LOGDSK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
		return true;
	}

	m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	return false;
}

bool CBKFloppyImage_RT11::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<RT11FileRecord *>(pFR->pSpecificData); // оригинальная запись
	int nIndex = FindRecord(pRec, true); // сперва найдём её

	if (nIndex >= 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
		m_RT11Catalog[nIndex].Record = *pRec; // теперь скорректируем в каталоге
		return WriteCurrentDir(); // сохраним каталог
	}

	// что-то не так
	return false;
}

// сквизирование диска
bool CBKFloppyImage_RT11::Squeeze()
{
	ReadRT11Catalog();  // перечитаем заново каталог
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	auto p = m_RT11Catalog.begin();

	while (p != m_RT11Catalog.end())
	{
#ifdef _DEBUG
		DebugOutCatalog();
#endif

		if (p->Record.nStatus & RT11_FILESTATUS_UNUSED) // если нашли дырку
		{
			auto n = p + 1;

			if (n == m_RT11Catalog.end())
			{
				// если p указывает на последнюю запись, то n == end, и конец работы
				// а имя последней записи надо скорректировать
				std::string str = "EMPTY ";
				EncodeRadix50(p->Record.FileName, 2, str);
				str = "FI";
				EncodeRadix50(&p->Record.FileExt, 1, str);
				break;
			}

			if (n->Record.nStatus & RT11_FILESTATUS_UNUSED) // и за дыркой снова дырка
			{
				p->Record.nBlkSize += n->Record.nBlkSize; // первую - укрупним
				auto i = std::distance(m_RT11Catalog.begin(), p);
				m_RT11Catalog.erase(n); // а вторую дырку удалим.
				// после этого p станет невалидным, и его надо заново переполучить
				p = m_RT11Catalog.begin();
				advance(p, i);
				continue; // и всё сначала
			}

			size_t nBufSize = size_t(n->Record.nBlkSize) * m_nBlockSize;
			auto pBuf = std::vector<uint8_t>(nBufSize);

			if (pBuf.data())
			{
				if (SeekToBlock(n->BeginBlock))
				{
					if (!ReadData(pBuf.data(), nBufSize))
					{
						m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
						bRet = false;
						break;
					}

					if (SeekToBlock(p->BeginBlock))
					{
						if (!WriteData(pBuf.data(), nBufSize))
						{
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
							bRet = false;
							break;
						}

						// теперь надо записи местами поменять.
						std::swap(*p, *n); // обменяем записи целиком
						std::swap(p->BeginBlock, n->BeginBlock); // начальные блоки вернём как было.
						n->BeginBlock = p->BeginBlock + p->Record.nBlkSize;
					}
					else
					{
						bRet = false;
					}
				}
				else
				{
					bRet = false;
				}
			}
			else
			{
				m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
				bRet = false;
				break;
			}
		}

		p++;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	WriteCurrentDir(); // сохраним
	return bRet;
}


#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_RT11::DebugOutCatalog()
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Type Attr Name         Size    Date        Offset, Extra:%d\n", m_nExtraBytes);
	auto sz = int(m_RT11Catalog.size());
	int offset = m_sDiskCat.nDataBegin * BLOCK_SIZE;

	for (int i = 0; i < sz; ++i)
	{
		std::string name = DecodeRadix50(m_RT11Catalog[i].Record.FileName, 2);
		std::string ext = DecodeRadix50(&(m_RT11Catalog[i].Record.FileExt), 1);

		if (!ext.empty())
		{
			name += "." + ext;
		}

		fwprintf(log, "%03d %03o  %03o  %-12s %06o  %06o  0x%08x\n", i, m_RT11Catalog[i].Record.nFileClass, m_RT11Catalog[i].Record.nStatus, name.c_str(), m_RT11Catalog[i].Record.nBlkSize, m_RT11Catalog[i].Record.nFileDate, offset);
		offset += m_RT11Catalog[i].Record.nBlkSize * BLOCK_SIZE;
	}

	fclose(log);
}
#endif
