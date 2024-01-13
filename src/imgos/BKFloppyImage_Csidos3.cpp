#include "pch.h"
#include "BKFloppyImage_Csidos3.h"
#include "StringUtil.h"

#pragma warning(disable:4996)

static const int PgNumF2L[8] = { 1, 5, 2, 3, 4, 7, 0, 6 }; // перекодировка БКшной кодировки номеров страниц в нормальную.
static const int PgNumL2F[8] = { 6, 0, 2, 3, 4, 1, 7, 5 }; // перекодировка логического номера страницы в БКшную реальную

CBKFloppyImage_Csidos3::CBKFloppyImage_Csidos3(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
{
//  m_pKoi8tbl = imgUtil::koi8tbl11M;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_Csidos3::~CBKFloppyImage_Csidos3()
{
	m_CSICatalog.clear();
}

const std::string CBKFloppyImage_Csidos3::GetSpecificData(BKDirDataItem *fr) const
{
	std::string str;
	auto pRec = reinterpret_cast<CsidosFileRecord *>(fr->pSpecificData);
	uint8_t dd = pRec->status;

	if (dd && (pRec->start_block | pRec->address | pRec->length))
	{
		int p0 = (dd >> 4) & 7;
		int p1 = dd & 7;
		str = imgUtil::string_format("%s; %d:%d\0", ((dd & 010) ? "БК11" : "БК10"), PgNumF2L[p0], PgNumF2L[p1]);
	}

	return str;
}


void CBKFloppyImage_Csidos3::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(CsidosFileRecord);
			memset(pRec, 0, sizeof(CsidosFileRecord));
		}

		// надо сформировать ксидосную запись из абстрактной
		std::string name = strUtil::CropStr(strUtil::strToLower(pFR->strName.stem().string()), 8);
		imgUtil::UNICODEtoBK(name, pRec->name, 8, true); // берём первые 8 символов имени.
		std::string ext = pFR->strName.extension().string();

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			pRec->name[8] = 0;
			pRec->name[9] = 0;
			pRec->name[10] = 0;
		}
		else
		{
			if (!ext.empty())
			{
				ext = strUtil::CropStr(ext.substr(1), 3); // без точки
			}

			ext = strUtil::strToLower(ext);
			imgUtil::UNICODEtoBK(ext, pRec->name + 8, 3, true);
		}

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			pRec->address = 0;
		}
		else
		{
			pRec->address = pFR->nAddress;
		}

		if (!bRenameOnly)
		{
			if (pFR->nAttr & FR_ATTR::PROTECTED)
			{
				pRec->protection = 0200;
			}

			pRec->type = pFR->nDirBelong + 1;

			if (pFR->nAttr & FR_ATTR::DIRECTORY)
			{
				pRec->status = pFR->nDirNum + 1;
				pRec->start_block = 0;
				pRec->length = 0;
			}
			else
			{
				pRec->status = pFR->timeCreation & 0xff; // по умолчанию файл для бк11, страницы: окно0-7 окно1-5
				pRec->start_block = pFR->nStartBlock;
				SetLength(pRec, pFR->nSize);
			}
		}
	}
}

void CBKFloppyImage_Csidos3::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		int nDirBelong = 0;

		if (pRec->type == 0376)
		{
			// дырка
			// игнорируем
			strcpy_s(reinterpret_cast<char *>(pRec->name), 11, "HOLE\0");
		}
		else if (pRec->type == 0312)
		{
			// запись не содержит информации
			// игнорируем
			strcpy_s(reinterpret_cast<char *>(pRec->name), 11, "NONE\0");
		}
		else if (pRec->type == 0377)
		{
			// удалённый файл, который можно восстановить
			pFR->nAttr |= FR_ATTR::DELETED;
		}
		else if (pRec->type == 0311)
		{
			// плохое место
			pFR->nAttr |= FR_ATTR::BAD;

			if (pRec->name[0] == 0)
			{
				strcpy_s(reinterpret_cast<char *>(pRec->name), 11, "BAD\0");
			}
		}
		else if ((1 <= pRec->type) && (pRec->type <= 0310))
		{
			// номер директории
			nDirBelong = pRec->type - 1;
		}

		pFR->nDirBelong = nDirBelong;

		if ((*(reinterpret_cast<uint16_t *>(pRec->name + 8)) == 0) && (pRec->start_block == 0)) // если директория
		{
			// обрабатываем директорию
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 8, m_pKoi8tbl));
			pFR->nDirNum = pRec->status - 1;
			pFR->nBlkSize = 0;
			// в поле времени будем хранить статус файла
			pFR->timeCreation = 0;

			// добавим на всякий случай, вдруг такое тоже есть
			if (pRec->address && pRec->address < 0200)
			{
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}
		}
		else
		{
			// обрабатываем файл
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 8, m_pKoi8tbl));
			std::string ext = strUtil::trim(imgUtil::BKToUNICODE(pRec->name + 8, 3, m_pKoi8tbl));

			if (!ext.empty())
			{
				pFR->strName += "." + ext;
			}

			pFR->nDirNum = 0;
			// в поле времени будем хранить статус файла
			pFR->timeCreation = pRec->status;
		}

		if (pRec->protection & 0200)
		{
			// защита от удаления
			pFR->nAttr |= FR_ATTR::PROTECTED;
		}

		pFR->nAddress = pRec->address;

		if (pRec->status & 0200)
		{
			// признак длины в блоках
			pFR->nSize = pRec->length * m_nBlockSize;
			pFR->nBlkSize = pRec->length;
		}
		else
		{
			pFR->nSize = pRec->length;
			pFR->nBlkSize = ByteSizeToBlockSize_l(pRec->length);
		}

		pFR->nStartBlock = pRec->start_block;
	}
}


void CBKFloppyImage_Csidos3::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 0311;
}

int CBKFloppyImage_Csidos3::FindRecord(CsidosFileRecord *pRec)
{
	size_t sz = m_CSICatalog.size();

	for (size_t i = 0; i < sz; ++i)
	{
		if ((m_CSICatalog.at(i).type - 1) == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
		{
			if (memcmp(pRec, &m_CSICatalog.at(i), sizeof(CsidosFileRecord)) == 0)
			{
				return static_cast<int>(i);
			}
		}
	}

	return -1;
}

int CBKFloppyImage_Csidos3::FindRecord2(CsidosFileRecord *pRec, bool bFull)
{
	size_t sz = m_CSICatalog.size();

	for (size_t i = 0; i < sz; ++i)
	{
		if (m_CSICatalog.at(i).type > 0310)
		{
			continue;
		}

		if ((m_CSICatalog.at(i).type - 1) == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
		{
			if (memcmp(pRec->name, m_CSICatalog.at(i).name, 11) == 0)  // проверим имя
			{
				if (bFull)
				{
					if ((*(reinterpret_cast<uint16_t *>(m_CSICatalog.at(i).name + 8)) == 0) && (m_CSICatalog.at(i).start_block == 0)) // если директория
					{
						if (m_CSICatalog.at(i).status == pRec->status) // то проверяем номер директории
						{
							return static_cast<int>(i);
						}
					}
					else // если файл - то проверяем параметры файла
					{
						if (m_CSICatalog.at(i).type == pRec->type)
						{
							return static_cast<int>(i);
						}
					}
				}
				else
				{
					return static_cast<int>(i); // если нам важно только имя
				}
			}
		}
	}

	return -1;
}

/* читаем каталог
заполняем переменные каталога.
*/
bool CBKFloppyImage_Csidos3::ReadCSICatalog()
{
	m_CSICatalog.clear();
	uint16_t nDiskSize = 012;
	int nUsedBlocks = 0;
	int nRecsCount = 0;

	for (int nCatBlock = 2; nCatBlock < 10; ++nCatBlock)
	{
		// Перемещаемся к очередному сектору каталога и читаем его
		if (!SeektoBlkReadData(nCatBlock, m_nSector, sizeof(m_nSector)))
		{
			return false;
		}

		// у каждого блока каталога есть заголовок
		auto CSIHdr = reinterpret_cast<CsidosCatHeader *>(m_nSector);
		auto CSICat = reinterpret_cast<CsidosFileRecord *>(m_nSector + 12);

		if (nCatBlock == 2) // важен заголовок только у первого блока каталога
		{
			m_FirstBlockHeader = *CSIHdr; // сохраним заголовок первого блока каталога
			nDiskSize = CSIHdr->block_counts;
		}

		// теперь заполним вектор записями
		// каталог занимает 8 секторов. но записи идут не с начала сектора, в каждом секторе есть заголовок.
		// длина заголовка 12. байтов, длина записи 20. байтов. в секторе получается 25. записей.
		bool bEoC = false;

		for (int i = 0; i < 25; ++i)
		{
			if (CSICat[i].type == 0) // если встретили конец каталога, то выход
			{
				bEoC = true;
				break;
			}

			// считаем записи принадлежащие каталогам, плохие.
			// игнорируем удалённые, дырки, и пустые записи
			if ((0 < CSICat[i].type) && (CSICat[i].type <= 0311))
			{
				nRecsCount++;

				if (CSICat[i].start_block)
				{
					if (CSICat[i].status & 0200)
					{
						nUsedBlocks += CSICat[i].length;
					}
					else
					{
						nUsedBlocks += ByteSizeToBlockSize_l(CSICat[i].length);
					}
				}
			}

			m_CSICatalog.push_back(CSICat[i]);
		}

		if (bEoC)
		{
			break; // совсем выход.
		}
	}

	m_sDiskCat.nTotalRecs = 25 * 8; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - nRecsCount;
	m_sDiskCat.nTotalBlocks = nDiskSize - 012;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - nUsedBlocks;
	return true;
}

/*
А теперь наоборот. Сохраняем каталог.
*/
bool CBKFloppyImage_Csidos3::WriteCSICatalog()
{
	bool bBeginPrepare = true; // флаг, что надо подготовить новый блок
	int nCatBlock = 2;
	int nRecNum = 0;
	auto CSIHdr = reinterpret_cast<CsidosCatHeader *>(m_nSector);
	auto CSICat = reinterpret_cast<CsidosFileRecord *>(m_nSector + 12);

	for (auto &p : m_CSICatalog)
	{
		if (bBeginPrepare)
		{
			bBeginPrepare = false;
			memset(m_nSector, 0, BLOCK_SIZE);

			if (nCatBlock == 2)
			{
				*CSIHdr = m_FirstBlockHeader;
			}
			else
			{
				CSIHdr->block_number = nCatBlock;
			}

			nRecNum = 0;
		}

		memcpy(&CSICat[nRecNum], std::addressof(p), sizeof(CsidosFileRecord));

		if (++nRecNum >= 25)
		{
			// переместимся к нужному блоку и запишем его
			if (!SeektoBlkWriteData(nCatBlock, m_nSector, sizeof(m_nSector)))
			{
				return false;
			}

			bBeginPrepare = true;
			nCatBlock++;
		}
	}

	// если вышли из цикла for не сохранив сегмент, а такое всегда случается,
	// если последний сегмент заполнен не полностью.
	if (!bBeginPrepare)
	{
		// переместимся к нужному блоку и запишем его
		if (!SeektoBlkWriteData(nCatBlock, m_nSector, sizeof(m_nSector)))
		{
			return false;
		}
	}

	return true;
}



bool CBKFloppyImage_Csidos3::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	if (!ReadCSICatalog())
	{
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	BKDirDataItem AFR;
	auto pRec = reinterpret_cast<CsidosFileRecord *>(AFR.pSpecificData);

	for (auto &p : m_CSICatalog)
	{
		if (p.type == 0376)
		{
			// дырка
			continue; // игнорируем
		}

		if (p.type == 0312)
		{
			// запись не содержит информации
			continue; // игнорируем
		}

		AFR.clear();
		AFR.nSpecificDataLength = sizeof(CsidosFileRecord);
		memcpy(pRec, std::addressof(p), sizeof(CsidosFileRecord));
		ConvertRealToAbstractRecord(&AFR);

		if ((AFR.nAttr & FR_ATTR::DIRECTORY) && !(AFR.nAttr & FR_ATTR::DELETED))
		{
			// если директория
			if (!AppendDirNum(AFR.nDirNum))
			{
				// встретили дублирование номеров директорий
				m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
			}
		}

		// выбираем только те записи, которые к нужной директории принадлежат.
		if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(AFR);
		}
		else if (m_sDiskCat.nCurrDirNum == 0 && AFR.nDirBelong >= 0300)
		{
			// покажем записи, не принадлежащие ни одному каталогу
			// как скрытые
			AFR.nAttr |= FR_ATTR::HIDDEN;
			m_sDiskCat.vecFC.push_back(AFR);
		}
	}

	return true;
}

bool CBKFloppyImage_Csidos3::WriteCurrentDir()
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
	return WriteCSICatalog();
}

bool CBKFloppyImage_Csidos3::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
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

void CBKFloppyImage_Csidos3::OnExtract(BKDirDataItem *pFR, std::string &strName)
{
	char buff[32] {};
	std::string str;
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData);
	uint8_t dd = pRec->status;
	int p0 = (dd >> 4) & 7;
	int p1 = dd & 7;
	char_t tch = (dd & 010) ? '!' : '-';
	snprintf(buff, 32, "#%d%c%d\0", PgNumF2L[p0], tch, PgNumF2L[p1]);
	strName += std::string(buff);
}

bool CBKFloppyImage_Csidos3::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
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

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	// !!!
	// А вот о выдаче я что-то не подумал. не знаю как сделать так.
	// !!!
	// теперь распарсим расширение.
	// формат такой: ext#n0xn1
	// где  # - признак
	//      n0 - цифра 0..7 страница в окно 0
	//      n1 - цифра 0..7 страница в окно 1
	//      x - символ: - - режим БК10
	//                  ! - режим БК11М
	std::string name = pFR->strName.stem().string();
	std::string ext = pFR->strName.extension().string();
	int p0 = PgNumL2F[7]; // значения по умолчанию БК11, страницы: окно0-7 окно1-5
	int p1 = PgNumL2F[5];
	bool bBK11 = true;
	size_t t = ext.rfind('#'); // поищем, может есть коды страниц ОЗУ

	if (t != std::string::npos)
	{
		size_t l = ext.length() - t - 1;

		if (l >= 3)
		{
			std::string sext = ext.substr(t + 1); // выделим предполагаемые номера страниц
			ext = ext.substr(0, t); // отрежем от расширения эту фигню
			pFR->strName = name + ext;

			// разбор делаем самым примитивным способом
			if (('0' <= sext[0]) && (sext[0] <= '7'))
			{
				p0 = PgNumL2F[sext[0] - '0'];
			}

			if ('-' == sext[1])
			{
				bBK11 = false;
			}
			else if ('!' == sext[1])
			{
				bBK11 = true;
			}

			if (('0' <= sext[2]) && (sext[2] <= '7'))
			{
				p1 = PgNumL2F[sext[2] - '0'];
			}

			// если хоть что-то будет не так - будет фигня, а не результат
		}
	}

	pFR->timeCreation = (p0 << 4) | p1 | (bBK11 ? 010 : 0);
	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
	pRec->type = m_sDiskCat.nCurrDirNum + 1;

	if (m_sDiskCat.nFreeBlocks < (int)ByteSizeToBlockSize_l(pFR->nSize))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
		return false;
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	// поищем, вдруг такое имя файла уже существует.
	int nIndex = FindRecord2(pRec, true);

	if (nIndex >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
		*pRec = m_CSICatalog.at(nIndex);
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	bool bRet = false;
	// найдём свободное место в каталоге.
	std::ptrdiff_t r377 = -1; // смещение для записи 0377
	std::ptrdiff_t nCatOffs = 0;
	bool bFound = false;

	for (auto p = m_CSICatalog.begin(); p != m_CSICatalog.end(); ++p)
	{
		if (p->type == 0377 && CompareSize(std::addressof(*p), pFR->nSize) >= 0)
		{
			// если нашли удалённый файл, который можно восстановить и его размер позволяет
			// записать туда наш файл.
			if (r377 == -1)
			{
				// используем только первую найденную
				r377 = std::distance(m_CSICatalog.begin(), p);
			}
		}
		else if ((p->type == 0376) && CompareSize(std::addressof(*p), pFR->nSize) >= 0)
		{
			// если нашли подходящую дырку
			bFound = true;
			nCatOffs = std::distance(m_CSICatalog.begin(), p);
			break;
		}
	}

	// если дырок не нашлось, но нашёлся файл, который можно восстановить,
	// то файл - фтопку, вместо него будет наш новый файл.
	if (!bFound && (r377 != -1))
	{
		bFound = true;
		nCatOffs = r377;
	}

	if (bFound)
	{
		// можно сохранять
		int nFirstFreeBlock = pRec->start_block = m_CSICatalog.at(nCatOffs).start_block;
		int nHoleSize = EvenSizeByBlock_l(GetLength(&m_CSICatalog.at(nCatOffs)));
		int len = EvenSizeByBlock_l(pFR->nSize);

		// сперва разберёмся с каталогом
		if (nHoleSize > len)
		{
			// раздвинем каталог
			size_t sz = m_CSICatalog.size();

			if ((sz >= 8 * 25 - 1))
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
				goto l_squeeze;
			}
			else
			{
				// раздвигаем
				// сохраняем нашу запись
				m_CSICatalog.at(nCatOffs) = *pRec;
				CsidosFileRecord hole;
				hole.type = 0376;
				hole.start_block = pRec->start_block + ByteSizeToBlockSize_l(len);
				SetLength(&hole, nHoleSize - len);
				// и запись с инфой о дырке
				auto p = m_CSICatalog.begin();
				std::advance(p, nCatOffs + 1);
				m_CSICatalog.insert(p, hole);
			}
		}
		else
		{
			// ничего раздвигать не надо, просто
			// сохраняем нашу запись
			m_CSICatalog.at(nCatOffs) = *pRec;
		}

		if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
		{
			// можно сохранять сам файл

			// перемещаемся к месту
			if (SeekToBlock(nFirstFreeBlock))
			{
				int nReaded;
				int nLen = pFR->nSize; // размер файла

				if (nLen == 0)
				{
					nLen++; // запись файла нулевой длины недопускается, поэтому скорректируем.
				}

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
					m_sDiskCat.nFreeRecs--;
					m_sDiskCat.nFreeBlocks -= ByteSizeToBlockSize_l(pFR->nSize);
				}
			}
		}
	}
	else
	{
		// если нет дырки, но суммарное свободное место на диске есть - делаем сквизирование и пишем в конец.
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_NEED_SQEEZE;
l_squeeze:
		bNeedSqueeze = true;
	}

	return bRet;
}

bool CBKFloppyImage_Csidos3::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // уже удалённое не удаляем повторно
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	int nIndex = FindRecord(pRec);   // найдём её в каталоге

	if (nIndex >= 0) // если нашли
	{
		if ((m_CSICatalog.at(nIndex).protection & 0200) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			m_CSICatalog.at(nIndex).type = 0377; // пометим как удалённую
			bRet = WriteCurrentDir(); // сохраним директорию
			m_sDiskCat.nFreeRecs++;
			m_sDiskCat.nFreeBlocks += ByteSizeToBlockSize_l(pFR->nSize);
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

bool CBKFloppyImage_Csidos3::CreateDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		pFR->timeCreation = 0;
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		pFR->nDirBelong = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
		pRec->type = m_sDiskCat.nCurrDirNum + 1;
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nIndex >= 0)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_CSICatalog.at(nIndex).status - 1; // и заодно узнаем номер директории
		}
		else
		{
			// найдём свободное место в каталоге.
			bool bFound = false;
			std::ptrdiff_t nIndex = 0;

			for (auto p = m_CSICatalog.begin(); p != m_CSICatalog.end(); ++p)
			{
				if (p->type == 0312)
				{
					// если нашли подходящее место
					bFound = true;
					nIndex = std::distance(m_CSICatalog.begin(), p);
					break;
				}
			}

			auto nCatOffs = m_CSICatalog.begin();

			if (!bFound) // если не нашли
			{
				nCatOffs = m_CSICatalog.end() - 1; // то будет предпоследнее, т.к. последнее - это дырка - оставшееся свободное место
			}
			else
			{
				std::advance(nCatOffs, nIndex);
			}

			pFR->nDirNum = AssignNewDirNum(); // назначаем номер директории.

			if (pFR->nDirNum == 0)
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
			}

			pRec->status = pFR->nDirNum + 1;

			if (bFound)
			{
				*nCatOffs = *pRec;
			}
			else
			{
				m_CSICatalog.insert(nCatOffs, *pRec);
			}

			// сохраняем каталог
			bRet = WriteCurrentDir();
			m_sDiskCat.nFreeRecs--;
		}
	}

	return bRet;
}

bool CBKFloppyImage_Csidos3::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBKFloppyImage_Csidos3::DeleteDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// Если образ открылся только для чтения,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // то записать в него мы ничего не сможем.
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // уже удалённое не удаляем повторно
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData);
		// узнать есть ли файлы в директории.
		bool bExist = false;
		size_t sz = m_CSICatalog.size();

		for (size_t i = 0; i < sz; ++i)
		{
			if (m_CSICatalog.at(i).type >= 0311) // удалённые файлы игнорируем
			{
				continue;
			}

			if (m_CSICatalog.at(i).type && (m_CSICatalog.at(i).type == pRec->status))
			{
				bExist = true; // нашли файл, принадлежащий этой директории
				break;
			}
		}

		if (bExist)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EMPTY;
		}
		else
		{
			// удалить можно пустую директорию.
			int nIndex = FindRecord(pRec);

			if (nIndex >= 0) // если нашли
			{
				m_CSICatalog.at(nIndex).type = 0312; // пометим как удалённую
				bRet = WriteCurrentDir(); // сохраним каталог
				m_sDiskCat.nFreeRecs++;
			}
			else
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
			}
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}

	return bRet;
}

bool CBKFloppyImage_Csidos3::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // оригинальная запись
	auto sz = static_cast<unsigned int>(m_CSICatalog.size());
#ifdef _DEBUG
	DebugOutCatalog();
#endif

	while (m_nCatPos < sz)
	{
		if ((m_CSICatalog.at(m_nCatPos).type - 1) == m_sDiskCat.nCurrDirNum)
		{
			*pRec = m_CSICatalog.at(m_nCatPos);
			pFR->nSpecificDataLength = sizeof(CsidosFileRecord);
			ConvertRealToAbstractRecord(pFR);
			m_nCatPos++;
			return true;
		}

		m_nCatPos++;
	}

	if (!m_vecPC.empty())
	{
		m_nCatPos = m_vecPC.back();
		m_vecPC.pop_back();
	}

	return false;
}

bool CBKFloppyImage_Csidos3::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<CsidosFileRecord *>(pFR->pSpecificData); // оригинальная запись
	int nIndex = FindRecord(pRec); // сперва найдём её

	if (nIndex >= 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
		m_CSICatalog.at(nIndex) = *pRec; // теперь скорректируем в каталоге
		return WriteCurrentDir(); // сохраним каталог
	}

	// что-то не так
	return false;
}

// сравниваем размер найденной дырки с размером файла.
// выход:
// -1 - дырка меньше файла
// 0 - дырка равна файлу
// 1 - дырка больше файла
int CBKFloppyImage_Csidos3::CompareSize(CsidosFileRecord *pRec, unsigned int fileLen)
{
	unsigned int fl = (pRec->status & 0200) ? ByteSizeToBlockSize_l(fileLen) : fileLen;

	if (pRec->length > fl)
	{
		return 1;
	}

	if (pRec->length == fl)
	{
		return 0;
	}

	return -1;
}

unsigned int CBKFloppyImage_Csidos3::GetLength(CsidosFileRecord *pRec)
{
	if (pRec->status & 0200)
	{
		// длина в блоках
		return pRec->length * m_nBlockSize;
	}

	return pRec->length;
}

void CBKFloppyImage_Csidos3::SetLength(CsidosFileRecord *pRec, unsigned int fileLen)
{
	if (fileLen > 65535)
	{
		pRec->length = ByteSizeToBlockSize_l(fileLen);
		pRec->status |= 0200;
	}
	else
	{
		pRec->status &= ~0200;
		pRec->length = fileLen;
	}
}


bool CBKFloppyImage_Csidos3::Squeeze()
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	auto p = m_CSICatalog.begin();

	while (p != m_CSICatalog.end())
	{
#ifdef _DEBUG
		DebugOutCatalog();
#endif

		if (p->type == 0311)
		{
			// если нашли сбойное место - игнорируем
		}
		else if (p->type == 0312)
		{
			// если нашли запись, которая не содержит информации
			auto i = std::distance(m_CSICatalog.begin(), p);
			m_CSICatalog.erase(p); // её тупо удалим
			p = m_CSICatalog.begin(); // после этого p станет невалидным, и его надо заново переполучить
			std::advance(p, i);
			continue;
		}
		else if (p->type == 0376 || p->type == 0377)
		{
			// если нашли дырку
			auto n = p + 1;

			if (n == m_CSICatalog.end())
			{
				// если p указывает на последнюю запись, то n == end
				// а последнюю запись оформим как дырку.
				p->type = 0376;
				memset(p->name, 0, 11);
				p->protection = 0;
				p->status &= 0200;
				p->address = 0;
				break;
			}

			if (n->type == 0376 || n->type == 0377 || n->type == 0312) // и за дыркой снова дырка
			{
				if (n->type != 0312) // запись, которая не содержит информации тоже тут может попасться.
				{
					unsigned int nl = EvenSizeByBlock_l(GetLength(std::addressof(*n)));
					unsigned int pl = EvenSizeByBlock_l(GetLength(std::addressof(*p)));
					pl += nl;
					SetLength(std::addressof(*p), pl); // первую - укрупним
				}

				auto i = std::distance(m_CSICatalog.begin(), p);
				m_CSICatalog.erase(n); // а вторую дырку удалим.
				p = m_CSICatalog.begin(); // после этого p станет невалидным, и его надо заново переполучить
				std::advance(p, i);
				continue; // и всё сначала
			}

			unsigned int nBufSize = GetLength(std::addressof(*n));

			if (nBufSize)
			{
				nBufSize = EvenSizeByBlock_l(nBufSize); // размер выровняем по границе блока
				auto pBuf = std::make_unique<uint8_t[]>(nBufSize);

				if (pBuf)
				{
					memset(pBuf.get(), 0, nBufSize);

					if (SeekToBlock(n->start_block))
					{
						if (!ReadData(pBuf.get(), nBufSize))
						{
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
							bRet = false;
							break;
						}

						if (SeekToBlock(p->start_block))
						{
							if (!WriteData(pBuf.get(), nBufSize))
							{
								m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
								bRet = false;
								break;
							}

							// теперь надо записи местами поменять.
							std::swap(*p, *n); // обменяем записи целиком
							std::swap(p->start_block, n->start_block); // начальные блоки вернём как было.
							unsigned int pl = GetLength(std::addressof(*p));
							n->start_block = p->start_block + nBufSize / m_nBlockSize;
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
			else
			{
				std::swap(*p, *n); // обменяем записи целиком
			}
		}

		p++;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	// если диск полностью заполнен, т.е. в конце диска нет дырки, то как быть?
	// !!! тут нужна процедура проверки каталога.
	int nUSedBlocks = 0;
	int nCurrBlock = 012;

	for (auto &p : m_CSICatalog)  // пройдёмся по всем записям
	{
		if ((*(reinterpret_cast<uint16_t *>(p.name + 8)) == 0) && (p.start_block == 0))
		{
			// директории игнорируем
			continue;
		}

		if (nCurrBlock > p.start_block)
		{
			ASSERT(false); // что-то не так
			m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
			return false;
		}

		if (nCurrBlock < p.start_block)
		{
			// бывает такая фигня - потерянное место на диске.
			nCurrBlock = p.start_block;
		}

		int nBlkLen = ByteSizeToBlockSize_l(GetLength(std::addressof(p)));
		nUSedBlocks += nBlkLen;
		nCurrBlock += nBlkLen;
	}

	auto last = m_CSICatalog.end() - 1; // берём последнюю запись

	if (last->type != 0376)
	{
		CsidosFileRecord hole;
		hole.type = 0376;
		hole.start_block = nCurrBlock;
		SetLength(&hole, m_sDiskCat.nTotalBlocks - nUSedBlocks);
	}

	WriteCurrentDir(); // сохраним
	return bRet;
}

#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_Csidos3::DebugOutCatalog()
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Type Prot Name        status start  address length\n");
	uint8_t name[12];
	size_t sz = m_CSICatalog.size();

	for (size_t i = 0; i < sz; ++i)
	{
		memcpy(name, m_CSICatalog.at(i).name, 11);
		name[11] = 0;
		fprintf(log, "%03d %03o  %03o  %-11s  %03o   %06o %06o  %06o\n", static_cast<int>(i), m_CSICatalog.at(i).type, m_CSICatalog.at(i).protection, name, m_CSICatalog.at(i).status, m_CSICatalog.at(i).start_block, m_CSICatalog.at(i).address, m_CSICatalog.at(i).length);
	}

	fclose(log);
}
#endif
