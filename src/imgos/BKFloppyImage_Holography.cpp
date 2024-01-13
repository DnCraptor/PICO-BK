#include "pch.h"
#include "BKFloppyImage_Holography.h"
#include "StringUtil.h"

#pragma warning(disable:4996)

CBKFloppyImage_Holography::CBKFloppyImage_Holography(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pCatBuffer(nullptr)
	, m_pDiskCat(nullptr)
	, m_nRecsNum(0)
	, m_nFreeBlock(HOLO_DATA_BLK)
{
	m_pCatBuffer = std::make_unique<uint8_t[]>(HOLO_CATALOG_SIZE);

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<HolographyFileRecord *>(m_pCatBuffer.get()); // каталог диска
	}
}

CBKFloppyImage_Holography::~CBKFloppyImage_Holography()
{
}

int CBKFloppyImage_Holography::FindRecord(HolographyFileRecord *pPec)
{
	for (int i = 0; i < m_nRecsNum; ++i) // цикл по всему каталогу
	{
		if (memcmp(&m_pDiskCat[i], pPec, HOLO_REC_SIZE) == 0)
		{
			return i;
		}
	}

	return -1;
}


int CBKFloppyImage_Holography::FindRecord2(HolographyFileRecord *pPec, bool bFull /*= true*/)
{
	for (int i = 1; i < m_nRecsNum; ++i)
	{
		// сравниваем имя
		if (memcmp(m_pDiskCat[i].name, pPec->name, 16) == 0)
		{
			// если полное сравнение
			if (bFull)
			{
				// сравним ещё и другие параметры
				if (m_pDiskCat[i].length_blk == pPec->length_blk
				        && m_pDiskCat[i].load_address == pPec->load_address
				        && m_pDiskCat[i].start_address == pPec->start_address)
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

void CBKFloppyImage_Holography::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly /*= false*/)
{
	auto pRec = reinterpret_cast<HolographyFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = HOLO_REC_SIZE;
			memset(pRec, 0, HOLO_REC_SIZE);
		}

		std::string strName = strUtil::CropStr(pFR->strName, 16);
		imgUtil::UNICODEtoBK(strName, pRec->name, 16, true);
		pRec->load_address = pFR->nAddress;
		pRec->length_blk = pFR->nBlkSize;
		pRec->start_block = pFR->nStartBlock;
		pRec->start_address = pFR->timeCreation; //тут будем хранить адрес запуска
	}
}

void CBKFloppyImage_Holography::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<HolographyFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 16, m_pKoi8tbl));
		pFR->nAttr = 0;
		pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
		pFR->nDirBelong = 0;
		pFR->nDirNum = 0;
		pFR->nAddress = pRec->load_address;
		pFR->nSize = pRec->length_blk * BLOCK_SIZE;
		pFR->nBlkSize = pRec->length_blk;
		pFR->nStartBlock = pRec->start_block;
		pFR->timeCreation = pRec->start_address; //тут будем хранить адрес запуска
	}
}



const std::string CBKFloppyImage_Holography::GetSpecificData(BKDirDataItem *fr) const
{
	auto pRec = reinterpret_cast<HolographyFileRecord *>(fr->pSpecificData);
	return imgUtil::string_format("%06o\0", pRec->start_address);
}

bool CBKFloppyImage_Holography::ReadCurrentDir()
{
	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	memset(m_pCatBuffer.get(), 0, HOLO_CATALOG_SIZE);

	if (!SeektoBlkReadData(HOLO_CATALOG_BLK, m_pCatBuffer.get(), HOLO_CATALOG_SIZE)) // читаем каталог
	{
		return false;
	}

	m_nRecsNum = 0;
	m_nFreeBlock = HOLO_DATA_BLK;
	BKDirDataItem AFR; // экземпляр абстрактной записи
	auto pHoloRec = reinterpret_cast<HolographyFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи
	int used_size = 0;

	for (int i = 0; i < HOLO_CATALOG_DIMENSION; ++i)
	{
		if (m_pDiskCat[i].length_blk == 0)
		{
			break; // конец каталога
		}

		m_nRecsNum++;
		// преобразуем запись и поместим в массив
		AFR.clear();
		AFR.nSpecificDataLength = HOLO_REC_SIZE;
		*pHoloRec = m_pDiskCat[i]; // копируем текущую запись как есть
		ConvertRealToAbstractRecord(&AFR);
		used_size += AFR.nBlkSize; // удалённые файлы тоже считаются.
		m_nFreeBlock += AFR.nBlkSize; // с какого места начинается свободная область
		m_sDiskCat.vecFC.push_back(AFR);
	}

	m_sDiskCat.nTotalRecs = HOLO_CATALOG_DIMENSION;
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - m_nRecsNum;
	int imgSize = max(1600, (int)m_sParseImgResult.nImageSize / m_nBlockSize); // NB! ОС работает только с 40 дорожками.
	m_sDiskCat.nTotalBlocks = imgSize - HOLO_DATA_BLK; // будем высчитывать это по размеру образа.
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	return true;
}



bool CBKFloppyImage_Holography::WriteCurrentDir()
{
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	// если памяти под массив не выделилось - то ошибка, дальше работать невозможно
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	if (!SeektoBlkWriteData(HOLO_CATALOG_BLK, m_pCatBuffer.get(), HOLO_CATALOG_SIZE)) // сохраняем каталог как есть
	{
		return false;
	}

	return true;
}

bool CBKFloppyImage_Holography::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
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


#ifdef _DEBUG
// отладочный вывод каталога
#pragma warning(disable:4996)
void CBKFloppyImage_Holography::DebugOutCatalog(HolographyFileRecord *pRec)
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  FileName         block  length load.ad stаrt.ad\n");
	uint8_t name[17];

	for (int i = 0; i < m_nRecsNum; ++i)
	{
		memcpy(name, pRec[i].name, 16);
		name[16] = 0;
		fprintf(log, "%03d %-16s %06o %06o %06o  %06o\n", i, name, pRec[i].start_block, pRec[i].length_blk, pRec[i].load_address, pRec[i].start_address);
	}

	fclose(log);
}
#endif