#include "pch.h"
#include "BKFloppyImage_NORD.h"
#include "StringUtil.h"


CBKFloppyImage_Nord::CBKFloppyImage_Nord(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pDiskCat(nullptr)
{
	m_nVersion = 0;
	m_nCatSize = 10 * m_nBlockSize; // размер каталога - нижняя сторона нулевой дорожки
	m_pCatBuffer = std::make_unique<uint8_t[]>(m_nCatSize); // необязательно выделять массив во время чтения каталога, размер-то его известен

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<NordFileRecord *>(m_pCatBuffer.get() + FMT_NORD_CAT_BEGIN); // каталог диска
	}

	m_nMKCatSize = (static_cast<size_t>(012000) - FMT_NORD_CAT_BEGIN) / sizeof(NordFileRecord);
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_Nord::~CBKFloppyImage_Nord()
{
}


bool CBKFloppyImage_Nord::IsEndOfCat(NordFileRecord *pRec)
{
	// для норд3.5 надо проверку на выход делать как в мкдосе, тут возможен глюк.
	// конец каталога необязательно 0, там может быть мусор, в этом случае будет мусор и прочитан.
	if (pRec->status == 0 && pRec->name[0] == 0)
	{
		return true; // теоретически это конец каталога
	}

	// вот ещё одна проверка на конец каталога.
	if (pRec->status == 0377 && pRec->dir_num == 0377 && pRec->name[0] == 055 && pRec->name[1] == 077)
	{
		return true; // теоретически это конец каталога
	}

	// и ещё одна проверка на конец каталога, в норде с этим бардак
	if (pRec->status == 0377 && pRec->len_blk == 0177777 && pRec->address == 0177777 && pRec->length == 0177777)
	{
		return true; // теоретически это конец каталога
	}

	return false;
}


int CBKFloppyImage_Nord::FindRecord(NordFileRecord *pRec)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) // цикл по всему каталогу
	{
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if (m_pDiskCat[i].dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			if (memcmp(&m_pDiskCat[i], pRec, sizeof(NordFileRecord)) == 0)
			{
				nIndex = static_cast<int>(i);
				break;
			}
		}
	}

	return nIndex;
}

int CBKFloppyImage_Nord::FindRecord2(NordFileRecord *pRec, bool bFull)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if (m_pDiskCat[i].status & 0200)
		{
			// если удалённый файл - дырка, то игнорируем
			continue;
		}

		if (m_pDiskCat[i].dir_num == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
		{
			if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // проверим имя
			{
				if (bFull)
				{
					if (pRec->name[0] == 0177) // если директория
					{
						if (m_pDiskCat[i].status == pRec->status) // то проверяем номер директории
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
					else // если файл - то проверяем параметры файла
					{
						if (m_pDiskCat[i].dir_num == pRec->dir_num)
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
				}
				else
				{
					nIndex = static_cast<int>(i);
					break;
				}
			}
		}
	}

	return nIndex;
}

void CBKFloppyImage_Nord::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(NordFileRecord);
			memset(pRec, 0, sizeof(NordFileRecord));
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

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			std::string strDir = strUtil::CropStr(pFR->strName, 13);
			imgUtil::UNICODEtoBK(strDir, pRec->name + 1, 13, true);
			pRec->name[0] = 0177; // признак каталога

			if (!bRenameOnly)
			{
				pRec->status = pFR->nDirNum;
				pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
			}
		}
		else
		{
			imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
			pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда

			if (!bRenameOnly)
			{
				pRec->status = 0;

				if (pFR->nAttr & FR_ATTR::PROTECTED)
				{
					pRec->status |= 1;
				}

				if (pFR->nAttr & FR_ATTR::READONLY)
				{
					pRec->status |= 4;
				}

				pRec->dir_num = pFR->nDirBelong;
				pRec->len_blk = ByteSizeToBlockSize_l(pFR->nSize); // размер в блоках
				pRec->length = pFR->nSize % 0200000; // остаток от длины по модулю 65535
			}
		}
	}
}

// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_Nord::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->name[0] == 0177)
		{
			// если директория
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name + 1, 13, m_pKoi8tbl));
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = pRec->status;
			pFR->nBlkSize = 0;

			// добавим на всякий случай, вдруг такое тоже есть
			if (pRec->address && pRec->address < 0200)
			{
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}
		}
		else
		{
			// если файл
			if (pRec->status & 0200)
			{
				pFR->nAttr |= FR_ATTR::DELETED;
			}
			else if (pRec->status & 2)
			{
				pFR->nAttr |= FR_ATTR::BAD;

				if (pRec->name[0] == 0)
				{
					pRec->name[0] = 'B';
					pRec->name[1] = 'A';
					pRec->name[2] = 'D';
				}
			}

			if (pRec->status & 0111)
			{
				pFR->nAttr |= FR_ATTR::PROTECTED;
			}

			if (pRec->status & 4)
			{
				pFR->nAttr |= FR_ATTR::READONLY;
			}

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
			pFR->nDirBelong = pRec->dir_num;
			pFR->nDirNum = 0;
			pFR->nBlkSize = pRec->len_blk;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;

			if (pFR->nAttr & FR_ATTR::DELETED)
			{
				pFR->nDirBelong = 1; // а то удалённые файлы вообще не видны,
				// а так они будут находиться в специальной директории DEL
				// ну и дырки так же там
			}

			if ((pRec->name[1] == ':') && (pRec->name[0] >= 'A' && pRec->name[0] <= 'Z'))
			{
				pFR->nAttr |= FR_ATTR::LOGDISK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LOGDSK;
			}
		}

		pFR->nAddress = pRec->address;

		if (pFR->nAttr & FR_ATTR::LOGDISK) // если у нас логический диск, то размер надо считать по блокам.
		{
			pFR->nSize = pRec->len_blk * m_nBlockSize;
		}
		else
		{
			/*  правильный расчёт длины файла.
			т.к. в curr_record.length хранится остаток длины по модулю 0200000, нам из длины в блоках надо узнать
			сколько частей по 0200000 т.е. сколько в блоках частей по 128 блоков.(128 блоков == 65536.== 0200000)

			hw = (curr_record.len_blk >>7 ) << 16; // это старшее слово двойного слова
			hw = (curr_record.len_blk / 128) * 65536 = curr_record.len_blk * m_nSectorSize
			*/
			uint32_t hw = (pRec->len_blk << 9) & 0xffff0000; // это старшее слово двойного слова
			pFR->nSize = hw + pRec->length; // это ст.слово + мл.слово
		}

		pFR->nStartBlock = pRec->start_block;
	}
}


void CBKFloppyImage_Nord::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 0177;
}

bool CBKFloppyImage_Nord::ReadCurrentDir()
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

	if (!SeektoBlkReadData(0, m_pCatBuffer.get(), m_nCatSize)) // читаем каталог
	{
		return false;
	}

	// пробуем определить версию. если по смещениям 012 и 014 будут цифры '3' и '5'
	// то будем считать, что это норд 3.x, иначе - 2.х
	m_nVersion = (m_pCatBuffer[012] == '3') ? 30 : 20;
	int recs_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов. (НЕ записей!)
	m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[0470])); // блок начала данных

	if (m_sDiskCat.nDataBegin <= 0 || m_sDiskCat.nDataBegin > 40) // если там некорректная информация
	{
		m_sDiskCat.nDataBegin = 024; // берём данные по умолчанию.
	}

	m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	int used_size = 0;
	int disk_size = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_DISK_SIZE]));

	if ((disk_size == 0) || (disk_size >= 32767))
	{
		// если указан какой-то неправильный размер, то размер берём из расчёта типа диска
		switch (*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_DISK_TYPE])))
		{
			case 0:
				disk_size = 80 * 2 * 10; // 0 - 80 дор. 2 стороны
				break;

			case 1:
				disk_size = 40 * 2 * 10; // 1 - 40 дор. 2 стороны
				break;

			case 2:
				disk_size = 80 * 1 * 10; // 2 - 80 дор. 1 сторона
				break;

			case 3:
				disk_size = 40 * 1 * 10; // 3 - 40 дор. 1 сторона
				break;
		}
	}

	if (*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_BEGIN_BLOCK])))
	{
		// если у нас логический диск, то берём размер логического диска
		disk_size = (int)m_sParseImgResult.nImageSize / BLOCK_SIZE;
	}
	else
	{
		// если у нас образ диска. то вычисляем посложнее
		disk_size = max(disk_size, (int)m_sParseImgResult.nImageSize / BLOCK_SIZE); // норд 3.5 может быть больше 800кб, сильно больше.
	}

	int cat_status = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_NUMBER])); // читаем кол-во и состояние копий каталогов на диске

	switch (cat_status)
	{
		case 0:
		case 1:
			disk_size -= (m_nVersion == 30) ? 2 * 10 : 10; // 0,1 - одна копия каталога, начальный сектор 0
			break;

		case 2: // 2 - одна копия, начальный сектор 012

			// это означает что на диске есть только вторая копия каталога, все остальные числа
			// означают, что есть как минимум одна копия, и она в начале диска
			if (!SeektoBlkReadData(012, m_pCatBuffer.get(), m_nCatSize)) // читаем каталог
			{
				return false;
			}

		case 3: // 3 - две копии. 0 и 012 секторы
			disk_size -= 2 * 10; // т.е. по любому от свободного размера диска надо вычесть размер обеих копий каталога.
			break;
	}

	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<NordFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи

	for (unsigned int i = 0; i < m_nMKCatSize; ++i) // цикл по всему каталогу
	{
		m_nMKLastCatRecord = i;

		if (m_nVersion == 30 && recs_count >= files_total) // файлы кончились, выходим
		{
			break;
		}

		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = sizeof(NordFileRecord);
		*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);

		if ((m_pDiskCat[i].status & 0200) || (m_pDiskCat[i].status & 2))
		{
			// в норд 3.5 удалённые не считаются
			// плохие наверное тоже не считаются, но проверить не на чём
			if (m_nVersion != 30) // а в старых - считаются
			{
				recs_count++;
			}
		}
		else
		{
			recs_count++;
		}

		if (!(AFR.nAttr & FR_ATTR::DELETED))
		{
			if (m_pDiskCat[i].name[0] == 0177)
			{
				// если директория
				if (!AppendDirNum(m_pDiskCat[i].status))
				{
					// встретили дублирование номеров директорий
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			}
			else
			{
				// если файл
				used_size += m_pDiskCat[i].len_blk;
			}
		}

		// выбираем только те записи, которые к нужной директории принадлежат.
		if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(AFR);
		}
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - recs_count;
	m_sDiskCat.nTotalBlocks = disk_size;
	m_sDiskCat.nFreeBlocks = disk_size - used_size;
	return true;
}

bool CBKFloppyImage_Nord::WriteCurrentDir()
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
	int cat_status = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_NUMBER]));

	// если есть первая копия, сохраняем её
	if ((cat_status & 1) || (cat_status == 0))
	{
		if (!SeektoBlkWriteData(0, m_pCatBuffer.get(), m_nCatSize)) // сохраняем каталог как есть
		{
			return false;
		}
	}

	// если есть вторая копия, сохраняем её
	if (cat_status & 2)
	{
		if (!SeektoBlkWriteData(012, m_pCatBuffer.get(), m_nCatSize))
		{
			return false;
		}
	}

	return true;
}

bool CBKFloppyImage_Nord::ChangeDir(BKDirDataItem *pFR)
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

//  m_nLastErrorNumber = IMAGE_ERROR_IS_NOT_DIR;
	return false;
}

bool CBKFloppyImage_Nord::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		// проверим, вдруг такая директория уже есть
		int nIndex = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBKFloppyImage_Nord::CreateDir(BKDirDataItem *pFR)
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
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
		pFR->nDirBelong = pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
		// проверим, вдруг такая директория уже есть
		int nInd = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

		if (nInd >= 0)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_pDiskCat[nInd].status; // и заодно узнаем номер директории
		}
		else
		{
			unsigned int nIndex = 0;
			// найдём свободное место в каталоге.
			bool bHole = false;
			bool bFound = false;

			for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
			{
				if (IsEndOfCat(&m_pDiskCat[i]))
				{
					break;
				}

				if ((m_pDiskCat[i].status & 0200) && (m_pDiskCat[i].name[0] == 0177))
				{
					// если нашли дырку в которой было имя директории
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
				pFR->nDirNum = pRec->status = AssignNewDirNum(); // назначаем номер директории.

				if (pFR->nDirNum == 0)
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
				}

				// если ошибок не произошло, сохраним результаты
				if (bHole)
				{
					// сохраняем нашу запись вместо удалённой директории
					m_pDiskCat[nIndex] = pRec;
				}
				else
				{
					// если нашли свободную область в конце каталога
					int nHoleSize = m_pDiskCat[nIndex].len_blk;
					int nStartBlock = m_pDiskCat[nIndex].start_block;
					// сохраняем нашу запись
					m_pDiskCat[nIndex] = pRec;
					// сохраняем признак конца каталога
					NordFileRecord hole;
					hole.start_block = nStartBlock + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					// и запись с инфой о свободной области
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;
				}

				// сохраняем каталог
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_RECORD_NUMBER])) += 1; // поправим параметры - количество файлов
				bRet = WriteCurrentDir();
				m_sDiskCat.nFreeRecs--;
			}
			else
			{
				m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
			}
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
	}

	return bRet;
}

bool CBKFloppyImage_Nord::DeleteDir(BKDirDataItem *pFR)
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
		// узнать есть ли файлы в директории.
		bool bExist = false;

		for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
		{
			if (IsEndOfCat(&m_pDiskCat[i]))
			{
				break;
			}

			if (m_pDiskCat[i].status & 0200)
			{
				// если удалённый файл - дырка, то игнорируем
				continue;
			}

			if (m_pDiskCat[i].dir_num == pFR->nDirNum)
			{
				bExist = true; // нашли файл, принадлежащий этой директории
				break;
			}
		}

		// если есть - не удалять.
		if (bExist)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EMPTY;
		}
		else
		{
			// удалить можно пустую директорию.
			auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
			int nIndex = FindRecord(pRec);

			if (nIndex >= 0) // если нашли
			{
				m_pDiskCat[nIndex].status = 0375; // пометим как удалённую
				m_pDiskCat[nIndex].dir_num = 1;
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
				bRet = WriteCurrentDir(); // сохраним директорию
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

bool CBKFloppyImage_Nord::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // оригинальная запись
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	// и выводим всё что найдём. но по одному.
	while (m_nCatPos < m_nMKLastCatRecord) // цикл по всему каталогу
	{
		if (IsEndOfCat(&m_pDiskCat[m_nCatPos]))
		{
			break;
		}

		if (m_pDiskCat[m_nCatPos].dir_num == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
		{
			*pRec = m_pDiskCat[m_nCatPos];
			pFR->nSpecificDataLength = sizeof(NordFileRecord);
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

bool CBKFloppyImage_Nord::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // оригинальная запись
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


bool CBKFloppyImage_Nord::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
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

bool CBKFloppyImage_Nord::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
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
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = pRec->dir_num = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

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
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if ((m_pDiskCat[i].status & 0200) && (m_pDiskCat[i].len_blk >= pRec->len_blk))
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
					NordFileRecord hole;
					hole.status = 0375;
					hole.dir_num = 1;
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
			NordFileRecord hole;
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
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_RECORD_NUMBER])) += 1;
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


bool CBKFloppyImage_Nord::DeleteFile(BKDirDataItem *pFR, bool bForce)
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
	auto pRec = reinterpret_cast<NordFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	// найдём её в каталоге
	int nIndex = FindRecord(pRec);

	if (nIndex >= 0) // если нашли
	{
		if ((m_pDiskCat[nIndex].status == 2) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			m_pDiskCat[nIndex].status = 0375; // пометим как удалённую
			m_pDiskCat[nIndex].dir_num = 1;
			*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_NORD_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
			bRet = WriteCurrentDir(); // сохраним директорию
			m_sDiskCat.nFreeRecs++;
			m_sDiskCat.nFreeBlocks += m_pDiskCat[nIndex].len_blk;
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

// сквизирование диска
bool CBKFloppyImage_Nord::Squeeze()
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	unsigned int p = 0;

	while (p <= m_nMKLastCatRecord)
	{
		if (m_pDiskCat[p].status & 0200) // если нашли дырку
		{
			unsigned int n = p + 1; // индекс следующей записи

			if (n >= m_nMKLastCatRecord) // если p - последняя запись
			{
				// Тут надо обработать выход
				m_pDiskCat[p].status = m_pDiskCat[p].dir_num = 0;
				memset(m_pDiskCat[p].name, 0, 14);
				m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
				m_nMKLastCatRecord--;
				break;
			}

			if (m_pDiskCat[n].status & 0200) // и за дыркой снова дырка
			{
				m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // первую - укрупним

				// а вторую удалим.
				while (n < m_nMKLastCatRecord) // сдвигаем каталог
				{
					m_pDiskCat[n] = m_pDiskCat[n + 1];
					n++;
				}

				memset(&m_pDiskCat[m_nMKLastCatRecord--], 0, sizeof(NordFileRecord));
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
void CBKFloppyImage_Nord::DebugOutCatalog(NordFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Stat.Dir.Name           start  lenblk  address length\n");
	uint8_t name[15];

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		memcpy(name, pRec[i].name, 14);
		name[14] = 0;
		fprintf(log, "%03d %03d  %03d %-14s %06o %06o  %06o  %06o\n", i, pRec[i].status, pRec[i].dir_num, name, pRec[i].start_block, pRec[i].len_blk, pRec[i].address, pRec[i].length);
	}

	fclose(log);
}
#endif
