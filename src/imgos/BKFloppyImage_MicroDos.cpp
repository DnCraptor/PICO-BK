#include "pch.h"
#include "BKFloppyImage_MicroDos.h"
#include "StringUtil.h"


CBKFloppyImage_MicroDos::CBKFloppyImage_MicroDos(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pDiskCat(nullptr)
{
	m_nCatSize = 10 * m_nBlockSize;
	m_pCatBuffer = std::make_unique<uint8_t[]>(m_nCatSize); // необязательно выделять массив во время чтения каталога, размер-то его известен

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<MicrodosFileRecord *>(m_pCatBuffer.get() + FMT_MIKRODOS_CAT_BEGIN); // каталог диска
	}

	m_nMKCatSize = (static_cast<size_t>(012000) - FMT_MIKRODOS_CAT_BEGIN) / sizeof(MicrodosFileRecord);
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_MicroDos::~CBKFloppyImage_MicroDos()
{
}

int CBKFloppyImage_MicroDos::FindRecord(MicrodosFileRecord *pRec)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) // цикл по всему каталогу
	{
		if (memcmp(&m_pDiskCat[i], pRec, sizeof(MicrodosFileRecord)) == 0)
		{
			nIndex = static_cast<int>(i);
			break;
		}
	}

	return nIndex;
}

int CBKFloppyImage_MicroDos::FindRecord2(MicrodosFileRecord *pRec, bool bFull)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if (m_pDiskCat[i].stat1 & 0200)
		{
			// если удалённый файл - дырка, то игнорируем
			continue;
		}

		if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // проверим имя
		{
			nIndex = static_cast<int>(i);
			break;
		}
	}

	return nIndex;
}

void CBKFloppyImage_MicroDos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(MicrodosFileRecord);
			memset(pRec, 0, sizeof(MicrodosFileRecord));
		}

		// надо сформировать мкдосную запись из абстрактной
		std::string strMKName = pFR->strName.stem().string();
		std::string strMKExt = strUtil::CropStr(pFR->strName.extension().string(), 4); // включая точку
		size_t nNameLen = (14 - strMKExt.length()); // допустимая длина имени

		if (strMKName.length() > nNameLen) // если имя длиньше
		{
			strMKName = strUtil::CropStr(strMKName, nNameLen); // обрезаем
		}

		strMKName += strMKExt; // прицепляем расширение
		imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
		pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда

		if (!bRenameOnly)
		{
			pRec->stat1 = pFR->nDirBelong;
			unsigned int nBlkLen = EvenSizeByBlock_l(pFR->nSize); // размер проги выровняем по границе блока
			pRec->len_blk = nBlkLen / m_nBlockSize; // размер в блоках
			pRec->length = pFR->nSize % 0200000; // остаток от длины по модулю 65535
		}
	}
}

// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_MicroDos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->stat1 & 0200)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
		}
		else if (pRec->stat0 & 0200)
		{
			pFR->nAttr |= FR_ATTR::BAD;

			if (pRec->name[0] == 0)
			{
				pRec->name[0] = 'B';
				pRec->name[1] = 'A';
				pRec->name[2] = 'D';
			}
		}

		// если файл
		std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
		std::string ext;
		size_t l = name.length();
		size_t t = name.rfind('.');

		if (t != std::string::npos) // если расширение есть
		{
			ext = name.substr(t, l);
			name = strUtil::trim(name.substr(0, t));
		}

		if (!ext.empty())
		{
			name += ext;
		}

		pFR->strName = name;
		pFR->nDirBelong = pRec->stat1;
		pFR->nDirNum = 0;
		pFR->nBlkSize = pRec->len_blk;
		pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;

		if (pFR->nAttr & FR_ATTR::DELETED)
		{
			pFR->nDirBelong = 0; // а то удалённые файлы вообще не видны. А всё потому, что у удалённых stat1 == 255 тоже.
		}

		pFR->nAddress = pRec->address;
		/*  правильный расчёт длины файла.
		т.к. в curr_record.length хранится остаток длины по модулю 0200000, нам из длины в блоках надо узнать
		сколько частей по 0200000 т.е. сколько в блоках частей по 128 блоков.(128 блоков == 65536.== 0200000)

		hw = (curr_record.len_blk >>7 ) << 16; // это старшее слово двойного слова
		hw = (curr_record.len_blk / 128) * 65536 = curr_record.len_blk * m_nSectorSize
		*/
		uint32_t hw = (pRec->len_blk << 9) & 0xffff0000; // это старшее слово двойного слова
		pFR->nSize = hw + pRec->length; // это ст.слово + мл.слово
		pFR->nStartBlock = pRec->start_block;
	}
}


bool CBKFloppyImage_MicroDos::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	memset(m_pCatBuffer.get(), 0, m_nCatSize);
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи

	if (!SeektoBlkReadData(0, m_pCatBuffer.get(), m_nCatSize)) // читаем каталог
	{
		return false;
	}

	int files_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов. (НЕ записей!)
	m_sDiskCat.nDataBegin = 012; // блок начала данных
	m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	int used_size = 0;

	for (unsigned int i = 0; i < m_nMKCatSize; ++i) // цикл по всему каталогу
	{
		m_nMKLastCatRecord = i;

		if (files_count >= files_total) // файлы кончились, выходим
		{
			break;
		}

		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = sizeof(MicrodosFileRecord);
		*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);
		files_count++;

		if (!(AFR.nAttr & FR_ATTR::DELETED))
		{
			used_size += m_pDiskCat[i].len_blk;
		}

		// выбираем только те записи, которые к нужной директории принадлежат.
		//if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(AFR);
		}
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_total;
	m_sDiskCat.nTotalBlocks = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_DISK_SIZE])) - m_sDiskCat.nDataBegin;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return true;
}

bool CBKFloppyImage_MicroDos::WriteCurrentDir()
{
	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (m_pCatBuffer == nullptr)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	if (!SeektoBlkWriteData(0, m_pCatBuffer.get(), m_nCatSize)) // сохраняем каталог как есть
	{
		return false;
	}

	return true;
}


bool CBKFloppyImage_MicroDos::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // оригинальная запись

	// и выводим всё что найдём. но по одному.
	while (m_nCatPos < m_nMKLastCatRecord) // цикл по всему каталогу
	{
		//if (m_pDiskCat[m_nCatPos].stat1 == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			*pRec = m_pDiskCat[m_nCatPos];
			pFR->nSpecificDataLength = sizeof(MicrodosFileRecord);
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

bool CBKFloppyImage_MicroDos::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // оригинальная запись
	int nIndex = FindRecord(pRec); // сперва найдём её

	if (nIndex >= 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
		m_pDiskCat[nIndex] = *pRec; // теперь скорректируем в каталоге
		return WriteCurrentDir(); // сохраним каталог
	}

	// что-то не так
	return false;
}


bool CBKFloppyImage_MicroDos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
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

bool CBKFloppyImage_MicroDos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
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

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = pRec->stat1 = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

	if (m_sDiskCat.nFreeBlocks < pRec->len_blk)
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
	int nInd = FindRecord2(pRec, true);

	if (nInd >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
		*pRec = m_pDiskCat[nInd];
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	bool bRet = false;
	unsigned int nIndex = 0;
	// найдём свободное место в каталоге.
	bool bFound = false;
	bool bHole = false; // что нашли, дырку или место в конце, т.к. разные способы обработки каталога

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if ((m_pDiskCat[i].stat1 & 0200) && (m_pDiskCat[i].len_blk >= pRec->len_blk))
		{
			// если нашли дырку подходящего размера
			bHole = true;
			nIndex = i;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		nIndex = m_nMKLastCatRecord;

		if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
		{
			bFound = true; // то нашли, иначе - нет
		}
	}

	if (bFound)
	{
		// можно сохранять
		int nFirstFreeBlock = pRec->start_block = m_pDiskCat[nIndex].start_block;
		int nHoleSize = m_pDiskCat[nIndex].len_blk;

		if (bHole)
		{
			// если нашли дырку в каталоге, надо раздвинуть каталог, или не раздвигать, если размер дырки совпадает
			if (nHoleSize > pRec->len_blk)
			{
				// раздвинем каталог
				unsigned int i = m_nMKLastCatRecord + 1; // найдём конец каталога

				if (i >= m_nMKCatSize) // если каталог раздвинуть нельзя
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
					goto l_squeeze;
				}
				else
				{
					// раздвигаем
					while (i > nIndex)
					{
						m_pDiskCat[i] = m_pDiskCat[i - 1];
						i--;
					}

					// сохраняем нашу запись
					m_pDiskCat[nIndex] = pRec;
					MicrodosFileRecord hole;
					hole.stat0 = 0377;
					hole.stat1 = 0377;
					hole.start_block = pRec->start_block + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					imgUtil::UNICODEtoBK("<HOLE>", hole.name, 14, true);
					hole.length = (hole.len_blk * m_nBlockSize) % 0200000;
					// и запись с инфой о дырке
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;
				}
			}
			else
			{
				// ничего раздвигать не надо, просто
				// сохраняем нашу запись
				m_pDiskCat[nIndex] = pRec;
			}
		}
		else
		{
			// если нашли свободную область в конце каталога
			// сохраняем нашу запись
			m_pDiskCat[nIndex] = pRec;
			// сохраняем признак конца каталога
			MicrodosFileRecord hole;
			hole.start_block = pRec->start_block + pRec->len_blk;
			hole.len_blk = nHoleSize - pRec->len_blk;
			// и запись с инфой о свободной области
			m_pDiskCat[nIndex + 1] = hole;
			m_nMKLastCatRecord++;
		}

		if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
		{
			// перемещаемся к месту
			if (SeekToBlock(nFirstFreeBlock))
			{
				int nReaded;
				int nLen = pFR->nSize; // размер файла

				if (nLen == 0)
				{
					nLen++; // запись файла нулевой длины не допускается, поэтому скорректируем.
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
					// если ошибок не произошло, сохраним результаты
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_CAT_RECORD_NUMBER])) += 1;
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_TOTAL_FILES_USED_BLOCKS])) += pRec->len_blk;
					// наконец сохраняем каталог
					bRet = WriteCurrentDir();
					m_sDiskCat.nFreeRecs--;
					m_sDiskCat.nFreeBlocks -= pRec->len_blk;
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


bool CBKFloppyImage_MicroDos::DeleteFile(BKDirDataItem *pFR, bool bForce)
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
	auto pRec = reinterpret_cast<MicrodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	// найдём её в каталоге
	int nIndex = FindRecord(pRec);

	if (nIndex >= 0) // если нашли
	{
		m_pDiskCat[nIndex].stat0 = 0377; // пометим как удалённую
		m_pDiskCat[nIndex].stat1 = 0377;
		*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
		*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_MIKRODOS_TOTAL_FILES_USED_BLOCKS])) -= m_pDiskCat[nIndex].len_blk; // кол-во занятых блоков
		bRet = WriteCurrentDir(); // сохраним директорию
		m_sDiskCat.nFreeRecs++;
		m_sDiskCat.nFreeBlocks += m_pDiskCat[nIndex].len_blk;
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

// сквизирование диска
bool CBKFloppyImage_MicroDos::Squeeze()
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	unsigned int p = 0;

	while (p <= m_nMKLastCatRecord)
	{
		if (m_pDiskCat[p].stat1 & 0200) // если нашли дырку
		{
			unsigned int n = p + 1; // индекс следующей записи

			if (n >= m_nMKLastCatRecord) // если p - последняя запись
			{
				// Тут надо обработать выход
				m_pDiskCat[p].stat0 = m_pDiskCat[p].stat1 = 0;
				memset(m_pDiskCat[p].name, 0, 14);
				m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
				m_nMKLastCatRecord--;
				break;
			}

			if (m_pDiskCat[n].stat1 & 0200) // и за дыркой снова дырка
			{
				m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // первую - укрупним

				// а вторую удалим.
				while (n < m_nMKLastCatRecord) // сдвигаем каталог
				{
					m_pDiskCat[n] = m_pDiskCat[n + 1];
					n++;
				}

				memset(&m_pDiskCat[m_nMKLastCatRecord--], 0, sizeof(MicrodosFileRecord));
				continue; // и всё сначала
			}

			size_t nBufSize = size_t(m_pDiskCat[n].len_blk) * m_nBlockSize;
			auto pBuf = std::vector<uint8_t>(nBufSize);

			if (pBuf.data())
			{
				if (SeekToBlock(m_pDiskCat[n].start_block))
				{
					if (!ReadData(pBuf.data(), nBufSize))
					{
						m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
						bRet = false;
						break;
					}

					if (SeekToBlock(m_pDiskCat[p].start_block))
					{
						if (!WriteData(pBuf.data(), nBufSize))
						{
							m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
							bRet = false;
							break;
						}

						// теперь надо записи местами поменять.
						std::swap(m_pDiskCat[p], m_pDiskCat[n]); // обменяем записи целиком
						std::swap(m_pDiskCat[p].start_block, m_pDiskCat[n].start_block); // начальные блоки вернём как было.
						m_pDiskCat[n].start_block = m_pDiskCat[p].start_block + m_pDiskCat[p].len_blk;
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

	WriteCurrentDir(); // сохраним
	return bRet;
}
#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_MicroDos::DebugOutCatalog(MicrodosFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  St0 St1 Name           start  lenblk  address length\n");
	uint8_t name[15];

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		memcpy(name, pRec[i].name, 14);
		name[14] = 0;
		fprintf(log, "%03d %03o %03o %-14s %06o %06o  %06o  %06o\n", i, pRec[i].stat0, pRec[i].stat1, name, pRec[i].start_block, pRec[i].len_blk, pRec[i].address, pRec[i].length);
	}

	fclose(log);
}
#endif
