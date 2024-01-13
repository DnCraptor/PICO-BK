#include "pch.h"
#include "BKFloppyImage_HCDos.h"
#include "StringUtil.h"

#include <ctime>

#pragma warning(disable:4996)

CBKFloppyImage_HCDos::CBKFloppyImage_HCDos(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_nFilesNum(0)
	, m_pDiskCat(nullptr)
	, m_pDiskMap(nullptr)
{
	m_pKoi8tbl = imgUtil::koi8tbl11M;
	m_nBlockSize = 1024;
	m_nCatSize = 010000; // размер буфера - 8 секторов
	m_pCatBuffer = std::make_unique<uint8_t[]>(m_nCatSize);

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<NCdosFileRecord *>(m_pCatBuffer.get() + CATALOG_OFFSET);
		m_pDiskMap = m_pCatBuffer.get() + DISKMAP_OFFSET;
	}

	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_HCDos::~CBKFloppyImage_HCDos()
{
}

int CBKFloppyImage_HCDos::FindRecord(NCdosFileRecord *pRec, bool bFull)
{
	for (int i = 1; i < m_nFilesNum; ++i)
	{
		// сравниваем имя
		if (memcmp(m_pDiskCat[i].name, pRec->name, 16) == 0)
		{
			// если полное сравнение
			if (bFull)
			{
				// сравним ещё и другие параметры
				if (m_pDiskCat[i].address == pRec->address && m_pDiskCat[i].length == pRec->length)
				{
					return i;
				}
			}
			else
			{
				return i;
			}
		}
	}

	return -1;
}

void CBKFloppyImage_HCDos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<NCdosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(NCdosFileRecord);
			memset(pRec, 0, sizeof(NCdosFileRecord));
		}

		// теперь сформируем HCшную запись из абстрактной
		std::string name = strUtil::strToUpper(strUtil::trim(pFR->strName.stem().string()));
		std::string ext =  strUtil::strToUpper(strUtil::trim(pFR->strName.extension().string()));

		if (!ext.empty())
		{
			name = strUtil::CropStr(name, 12);
			ext = strUtil::CropStr(ext, 4);
			imgUtil::UNICODEtoBK(name, pRec->name, 12, true);
			imgUtil::UNICODEtoBK(ext, pRec->ext, 4, true);
		}
		else
		{
			name = strUtil::CropStr(name, 16);
			imgUtil::UNICODEtoBK(name, pRec->name, 16, true);
		}

		pRec->address = pFR->nAddress;

		if (pFR->nAttr & FR_ATTR::PROTECTED)
		{
			pRec->address |= 1;
		}

		if (!bRenameOnly)
		{
			pRec->length = pFR->nSize;
			tm ctm;
			gmtime_s(&ctm, &pFR->timeCreation);
			int year = (ctm.tm_year + 1900) > 1972 ? ctm.tm_year + 1900 - 1972 : 0;
			pRec->date = (((ctm.tm_mon & 0xf) + 1) << 10) | ((ctm.tm_mday & 0x1f) << 5) | (year & 0x1f) | ((year & 0x60) << 9);
		}
	}
}

void CBKFloppyImage_HCDos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<NCdosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		if (pRec->ext[0] == '.') // если есть расширение
		{
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 12, m_pKoi8tbl));
			std::string ext = strUtil::trim(imgUtil::BKToUNICODE(pRec->ext, 4, m_pKoi8tbl));
			pFR->strName += ext;
		}
		else
		{
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 16, m_pKoi8tbl));
		}

		pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
		pFR->nDirBelong = 0;
		pFR->nDirNum = 0;
		pFR->nAddress = pRec->address & 0177776;
		pFR->nSize = pRec->length;
		pFR->nBlkSize = ByteSizeToBlockSize_l(pFR->nSize);

		if (pRec->address & 1)
		{
			pFR->nAttr |= FR_ATTR::PROTECTED;
		}

		// обратная операция для времени
		tm ctm;
		memset(&ctm, 0, sizeof(tm));
		ctm.tm_mday = (pRec->date >> 5) & 0x1f;
		ctm.tm_mon = ((pRec->date >> 10) & 0x0f) - 1;
		ctm.tm_year = ((pRec->date >> 9) & 0x60) | (pRec->date & 0x1f) + 1972 - 1900;

		if (ctm.tm_year < 0)
		{
			ctm.tm_year = 0;
		}

		pFR->timeCreation = mktime(&ctm);
	}
}


bool CBKFloppyImage_HCDos::ReadCurrentDir()
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

	m_nFilesNum = 0;
	m_sDiskCat.nDataBegin = 4; // блок начала данных
	m_sDiskCat.nTotalRecs = CATALOG_SIZE; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	int used_size = 0;
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pRec = reinterpret_cast<NCdosFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи

	for (int i = 1; i < CATALOG_SIZE; ++i) // самую первую запись игнорируем, это метка диска
	{
		m_nFilesNum = i + 1; // нумерация файлов в каталоге начинается с 1, т.е. первая проигнорированная запись - №1

		// первый реальный файл - №2
		if (m_pDiskCat[i].name[0] == 0)
		{
			break; // теоретически это вообще конец каталога
		}

		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = sizeof(NCdosFileRecord);
		*pRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);
		used_size += AFR.nBlkSize;
		// в качестве начального сектора будем передавать номер файла
		AFR.nStartBlock = m_nFilesNum;
		// вот эту фигню надо как-то в конвертер перенести.
		int n = GetStartBlock(m_nFilesNum);

		if (m_pDiskMap[n] & 0x80)
		{
			AFR.nAttr |= FR_ATTR::HIDDEN;
		}

		m_sDiskCat.vecFC.push_back(AFR);
	}

	m_sDiskCat.nTotalRecs = CATALOG_SIZE;
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - m_nFilesNum;
	int imgSize = max(800, (int)m_sParseImgResult.nImageSize / m_nBlockSize); // неизвестно как определяется размер диска в блоках,
	m_sDiskCat.nTotalBlocks = imgSize - 4; // будем высчитывать это по размеру образа.
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return true;
}

bool CBKFloppyImage_HCDos::WriteCurrentDir()
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

	if (!SeektoBlkWriteData(0, m_pCatBuffer.get(), m_nCatSize)) // сохраняем каталог как есть
	{
		return false;
	}

	return true;
}

bool CBKFloppyImage_HCDos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;

	// если /*плохой или удалённый или*/ директория - ничего не делаем
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // если это не файл - выход с ошибкой
	}

	ConvertAbstractToRealRecord(pFR);
	int nCurBlock = GetStartBlock(pFR->nStartBlock);

	if (nCurBlock == -1)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
		return false;
	}

	int nLen = pFR->nSize;
	// ФС допускает фрагментирование
	uint8_t *bufp = pBuffer;

	while (nLen > 0)
	{
		if (nCurBlock == -1)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
			return false;
		}

		int nReaded = (nLen >= static_cast<int>(m_nBlockSize)) ? static_cast<int>(m_nBlockSize) : nLen;
		bRet = SeektoBlkReadData(nCurBlock, bufp, nReaded);

		if (!bRet)
		{
			break;
		}

		bufp += nReaded;
		nLen -= nReaded;
		nCurBlock = GetNextBlock(nCurBlock);
	}

	return bRet;
}

bool CBKFloppyImage_HCDos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

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
	auto pRec = reinterpret_cast<NCdosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
	pFR->nDirBelong = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

	if (m_sDiskCat.nFreeBlocks < ByteSizeToBlockSize_l(pRec->length))
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
	int nIndex = FindRecord(pRec, true);

	if (nIndex >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST; // если файл существует - выходим с ошибкой
		*pRec = m_pDiskCat[nIndex];
		ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
		return false;
	}

	// попадая сюда, мы гарантированно имеем в конце каталога свободное место, там и создадим запись
	nIndex = m_nFilesNum; // это номер нашего добавляемого файла
	m_pDiskCat[m_nFilesNum - 1] = *pRec;
	memset(&m_pDiskCat[m_nFilesNum++], 0, sizeof(NCdosFileRecord)); // на всякий случай - обозначим конец каталога
	// теперь сохраним файл
	int nCurBlock = GetFreeBlock(0);

	if (nCurBlock == -1)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
		return false;
	}

	// ФС допускает фрагментирование
	uint8_t *bufp = pBuffer;
	int nLen = pFR->nSize;

	if (nLen == 0)
	{
		nLen++; // запись файла нулевой длины не допускается, поэтому скорректируем.
	}

	while (nLen > 0)
	{
		int nReaded = (nLen >= static_cast<int>(m_nBlockSize)) ? static_cast<int>(m_nBlockSize) : nLen;
		bRet = SeektoBlkWriteData(nCurBlock, bufp, nReaded);

		if (!bRet)
		{
			break;
		}

		bufp += nReaded;
		nLen -= nReaded;
		MarkBlock(nCurBlock, nIndex);
		nCurBlock = GetFreeBlock(nCurBlock);

		if (nCurBlock == -1)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
			return false;
		}
	}

	if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
	{
		// наконец сохраняем каталог
		bRet = WriteCurrentDir();
		// чтобы каталог заново не перечитывать, корректируем данные
		m_sDiskCat.nFreeBlocks -= ByteSizeToBlockSize_l(pRec->length);
		m_sDiskCat.nFreeRecs--;
	}

	return bRet;
}

bool CBKFloppyImage_HCDos::DeleteFile(BKDirDataItem *pFR, bool bForce)
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
	auto pRec = reinterpret_cast<NCdosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
	int nIndex = FindRecord(pRec);

	if (nIndex > 0) // Если нашли
	{
		if ((m_pDiskCat[nIndex].address & 1) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			// в карте диска пометим блоки, занятые файлом как свободные
			for (int n = 0; n < DISKMAP_SIZE; ++n)
			{
				if (m_pDiskMap[n] == 0)
				{
					break;
				}

				if ((m_pDiskMap[n] & 0x7f) == nIndex + 1)
				{
					m_pDiskMap[n] = 0;
				}
			}

			// а заданную запись в каталоге просто удалим, сократив каталог.
			while (nIndex < m_nFilesNum - 1)
			{
				m_pDiskCat[nIndex] = m_pDiskCat[nIndex + 1];
				nIndex++;
			}

			memset(&m_pDiskCat[nIndex], 0, sizeof(NCdosFileRecord));
			m_nFilesNum--;
			bRet = WriteCurrentDir(); // сохраним директорию
			m_sDiskCat.nFreeRecs++;
			m_sDiskCat.nFreeBlocks += ByteSizeToBlockSize_l(pRec->length);
		}
	}
	else
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
	}

	return bRet;
}

bool CBKFloppyImage_HCDos::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<NCdosFileRecord *>(pFR->pSpecificData); // оригинальная запись
	int nIndex = FindRecord(pRec, true); // сперва найдём её

	if (nIndex > 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
		m_pDiskCat[nIndex] = *pRec; // теперь скорректируем в каталоге
		return WriteCurrentDir(); // сохраним каталог
	}

	// что-то не так
	return false;
}

// находим первый свободный блок в карте диска от заданной позиции.
// вход: blk - текущая позиция в карте
// выход: -1 - ничего не найдено
//      или номер блока
int CBKFloppyImage_HCDos::GetFreeBlock(int blk)
{
	while (blk < DISKMAP_SIZE)
	{
		if (m_pDiskMap[blk] == 0)
		{
			return blk;
		}

		blk++;
	}

	return 0;
}

// помечаем блок как занятый файлом
bool CBKFloppyImage_HCDos::MarkBlock(int blk, int fn)
{
	if (blk < DISKMAP_SIZE)
	{
		m_pDiskMap[blk] = fn;
		return true;
	}

	return false;
}

// находим первый блок заданного файла
// вход: fn - номер файла, 1..0176, не может быть 0
// выход: -1 - ничего не найдено
//      или номер блока
int CBKFloppyImage_HCDos::GetStartBlock(int fn)
{
	for (int n = 0; n < DISKMAP_SIZE; ++n)
	{
		if ((m_pDiskMap[n] & 0x7f) == fn)
		{
			return n;
		}
	}

	return -1;
}

// находим следующий блок заданного файла
// вход: blk - текущий найденный блок заданного файла
// выход: -1 - ничего не найдено
//      или номер блока
int CBKFloppyImage_HCDos::GetNextBlock(int blk)
{
	int fn = m_pDiskMap[blk++] & 0x7f;

	while (blk < DISKMAP_SIZE)
	{
		if ((m_pDiskMap[blk] & 0x7f) == fn)
		{
			return blk;
		}

		blk++;
	}

	return -1;
}
