#include "pch.h"
#include "BKFloppyImage_DaleOS.h"
#include "StringUtil.h"

// атрибуты файла
constexpr uint16_t FA$EXIST = 1;    //1 = файл есть, 0 = файл удалён / дырка
constexpr uint16_t FA$PROTECT = 2;  //1 = защита от удаления
constexpr uint16_t FA$EOC = 4;      //1 = признак конца каталога
constexpr uint16_t FA$SYSTEM = 20;  //1 = системный файл


СBKFloppyImage_DaleOS::СBKFloppyImage_DaleOS(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pCatBuffer(nullptr)
	, m_nDataBlock(0)
	, m_nDiskSizeBlk(0)
{
	m_pCatBuffer = std::make_unique<uint8_t[]>(DALE_CAT_SIZE);
}

СBKFloppyImage_DaleOS::~СBKFloppyImage_DaleOS()
{
}

// int СBKFloppyImage_DaleOS::FindRecord(DaleOSFileRecord *pPec)
// {
//
// }


void СBKFloppyImage_DaleOS::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly /*= false*/)
{
	auto pRec = reinterpret_cast<DaleOSFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

	// преобразовывать будем только если там ещё не сформирована реальная запись.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = DALE_REC_SIZE;
			memset(pRec, 0, DALE_REC_SIZE);
		}

		std::string strName = strUtil::CropStr(pFR->strName, 16);
		imgUtil::UNICODEtoBK(strName, pRec->name, 16, true);
		//if (!bRenameOnly)
		{
			pRec->attr = 0;

			if ((pFR->nAttr & FR_ATTR::DELETED) == 0)
			{
				pRec->attr |= FA$EXIST;
			}

			if (pFR->nAttr & FR_ATTR::READONLY)
			{
				pRec->attr |= FA$PROTECT;
			}

			if (pFR->nAttr & FR_ATTR::SYSTEM)
			{
				pRec->attr |= FA$SYSTEM;
			}

			pRec->address = pFR->nAddress;
			pRec->length = pFR->nSize;
			pRec->start_block = pFR->nStartBlock;
			pRec->reserved = -1;
		}
	}
}


void СBKFloppyImage_DaleOS::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<DaleOSFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

	if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
	{
		pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;

		if ((pRec->attr & FA$EXIST) == 0)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
		}

		if ((pRec->attr & FA$PROTECT))
		{
			pFR->nAttr |= FR_ATTR::READONLY;
		}

		if ((pRec->attr & FA$SYSTEM))
		{
			pFR->nAttr |= FR_ATTR::SYSTEM;
		}

		pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 16, m_pKoi8tbl));
		pFR->nDirBelong = 0;
		pFR->nDirNum = 0;
		pFR->nAddress = pRec->address;
		pFR->nSize = pRec->length;
		pFR->nBlkSize = ByteSizeToBlockSize_l(pFR->nSize);
		pFR->nStartBlock = pRec->start_block;
	}
}

bool СBKFloppyImage_DaleOS::ReadDaleCatalog()
{
	// Перемещаемся к блоку с битовой картой и читаем его
	if (!SeektoBlkReadData(DALE_BITMAP_BLK, m_nSector, sizeof(m_nSector)))
	{
		return false;
	}

	// проверим сигнатуру
	if (*(reinterpret_cast<uint16_t *>(&m_nSector[DALE_BITMAP_SIGNATURE_OFFSET])) != DALE_BITMAP_SIGNATURE)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
		return false;
	}

	m_nDataBlock = *(reinterpret_cast<uint16_t *>(&m_nSector[DALE_BITMAP_DATABLOCK_OFFSET]));
	m_nDiskSizeBlk = *(reinterpret_cast<uint16_t *>(&m_nSector[DALE_BITMAP_DISKDIMENSION_OFFSET]));
	m_DaleCatalog.clear();
	int nUsedBlocks = 0;
	int nRecsCount = 0;

	for (int nCatBlock = DALE_CAT_BLK; nCatBlock < DALE_CAT_BLK + DALE_CAT_SIZEBLK; ++nCatBlock)
	{
		// Перемещаемся к очередному сектору каталога и читаем его
		if (!SeektoBlkReadData(nCatBlock, m_nSector, sizeof(m_nSector)))
		{
			return false;
		}

		// у каждого блока каталога есть сигнатура
		if (*(reinterpret_cast<uint16_t *>(&m_nSector[DALE_CAT_SIGNATURE_OFFSET])) != DALE_CAT_SIGNATURE)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FORMAT_ERROR;
			return false;
		}

		auto DaleCat = reinterpret_cast<DaleOSFileRecord *>(m_nSector);
		bool bEoC = false;

		// теперь заполним вектор записями
		for (int i = 0; i < DALE_CAT_REC_SIZE; ++i)
		{
			if (DaleCat[i].attr & FA$EOC) // если встретили конец каталога, то выход
			{
				bEoC = true;
				break;
			}

			if (DaleCat[i].attr & FA$EXIST)
			{
				nRecsCount++;
				nUsedBlocks += ByteSizeToBlockSize_l(DaleCat[i].length);
			}

			m_DaleCatalog.push_back(DaleCat[i]);
		}

		if (bEoC)
		{
			break; // совсем выход.
		}
	}

	m_sDiskCat.nTotalRecs = DALE_CAT_REC_SIZE * DALE_CAT_SIZEBLK; // это у нас объём каталога, из него надо будет вычесть общее количество записей
	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - nRecsCount;
	m_sDiskCat.nTotalBlocks = m_nDiskSizeBlk - m_nDataBlock;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - nUsedBlocks;
	return true;
}

bool СBKFloppyImage_DaleOS::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	if (!ReadDaleCatalog())
	{
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog();
#endif
	BKDirDataItem AFR;
	auto pRec = reinterpret_cast<DaleOSFileRecord *>(AFR.pSpecificData);

	for (auto &p : m_DaleCatalog)
	{
		AFR.clear();
		AFR.nSpecificDataLength = DALE_REC_SIZE;
		memcpy(pRec, std::addressof(p), DALE_REC_SIZE);
		ConvertRealToAbstractRecord(&AFR);
		m_sDiskCat.vecFC.push_back(AFR);
	}

	return true;
}

bool СBKFloppyImage_DaleOS::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
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
void СBKFloppyImage_DaleOS::DebugOutCatalog()
{
	auto strModuleName = std::vector<char_t>(_MAX_PATH);
	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
	fs::path strName = fs::path(strModuleName.data()).parent_path() / "dirlog.txt";
	FILE *log = _wfopen(strName.c_str(), "wt");
	fprintf(log, "N#  Name            status start  address length\n");
	uint8_t name[17];
	size_t sz = m_DaleCatalog.size();

	for (size_t i = 0; i < sz; ++i)
	{
		memcpy(name, m_DaleCatalog.at(i).name, 16);
		name[16] = 0;
		std::string strAttr;

		if (m_DaleCatalog.at(i).attr & FA$EXIST)
		{
			if (m_DaleCatalog.at(i).attr & FA$SYSTEM)
			{
				strAttr.push_back('S');
			}

			if (m_DaleCatalog.at(i).attr & FA$PROTECT)
			{
				strAttr.push_back('P');
			}
		}
		else
		{
			strAttr = "De";
		}

		fprintf(log, "%03d %-16s  %-6s %06o %06o  %06o\n", static_cast<int>(i), name, strAttr.c_str(),
		        m_DaleCatalog.at(i).start_block, m_DaleCatalog.at(i).address, m_DaleCatalog.at(i).length);
	}

	fclose(log);
}

#endif

