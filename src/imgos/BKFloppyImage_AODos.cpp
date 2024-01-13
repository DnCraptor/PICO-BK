#include "pch.h"
#include "BKFloppyImage_AODos.h"
#include "StringUtil.h"

// TODO: ����� ���-�� ��������� � ���-�� ��������� ����� ����� ������������� ������� ���������
// �������������� � �������������. ��� ������ ��������� ��������� ������� ��������, ��� ����� ��������
// � ����� �������� �� ��� �������, ���� ��� ������������ ���������� ������ �������

CBKFloppyImage_AODos::CBKFloppyImage_AODos(const PARSE_RESULT &image)
	: CBKFloppyImage_Prototype(image)
	, m_pDiskCat(nullptr)
{
	m_nCatSize = 10 * m_nBlockSize; // ������ �������� - ���� ������� ������� �������.
	m_pCatBuffer = std::make_unique<uint8_t[]>(m_nCatSize); // ������������� �������� ������ �� ����� ������ ��������, ������-�� ��� ��������

	if (m_pCatBuffer)
	{
		m_pDiskCat = reinterpret_cast<AodosFileRecord *>(m_pCatBuffer.get() + FMT_AODOS_CAT_BEGIN); // ������� �����
	}

	m_nMKCatSize = (static_cast<size_t>(012000) - FMT_AODOS_CAT_BEGIN) / sizeof(AodosFileRecord);
	m_nMKLastCatRecord = m_nMKCatSize - 1;
	m_bMakeAdd = true;
	m_bMakeDel = true;
	m_bMakeRename = true;
	m_bChangeAddr = true;
}


CBKFloppyImage_AODos::~CBKFloppyImage_AODos()
{
}

int CBKFloppyImage_AODos::FindRecord(AodosFileRecord *pRec)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) // ���� �� ����� ��������
	{
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if ((m_pDiskCat[i].status2 & 0177) == m_sDiskCat.nCurrDirNum) // ���� ������ � ������� ����������
		{
			if (memcmp(&m_pDiskCat[i], pRec, sizeof(AodosFileRecord)) == 0)
			{
				nIndex = static_cast<int>(i);
				break;
			}
		}
	}

	return nIndex;
}

int CBKFloppyImage_AODos::FindRecord2(AodosFileRecord *pRec, bool bFull)
{
	int nIndex = -1;

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if ((m_pDiskCat[i].status2 & 0200) || (m_pDiskCat[i].status1 & 0200))
		{
			// ���� �������� ���� ��� ������, �� ����������
			continue;
		}

		if ((m_pDiskCat[i].status2 & 0177) == m_sDiskCat.nCurrDirNum) // ��������� ������ � ������� ����������
		{
			if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // �������� ���
			{
				if (bFull)
				{
					if (pRec->len_blk == 0) // ���� ����������
					{
						if ((m_pDiskCat[i].status1 & 0177) == (pRec->status1 & 0177)) // �� ��������� ����� ����������
						{
							nIndex = static_cast<int>(i);
							break;
						}
					}
					else // ���� ���� - �� ��������� ��������� �����
					{
						if ((m_pDiskCat[i].status2 & 0177) == (pRec->status2 & 0177))
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

void CBKFloppyImage_AODos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� ��������

	// ��������������� ����� ������ ���� ��� ��� �� ������������ �������� ������.
	if (pFR->nSpecificDataLength == 0 || bRenameOnly)
	{
		if (!bRenameOnly)
		{
			pFR->nSpecificDataLength = sizeof(AodosFileRecord);
			memset(pRec, 0, sizeof(AodosFileRecord));
		}

		// ���� ������������ �������� ������ �� �����������
		std::string strMKName = pFR->strName.stem().string();
		std::string strMKExt = strUtil::CropStr(pFR->strName.extension().string(), 4); // ������� �����
		size_t nNameLen = (14 - strMKExt.length()); // ���������� ����� �����

		if (strMKName.length() > nNameLen) // ���� ��� �������
		{
			strMKName = strUtil::CropStr(strMKName, nNameLen); // ��������
		}

		strMKName += strMKExt; // ���������� ����������

		if (pFR->nAttr & FR_ATTR::DIRECTORY)
		{
			std::string strDir = strUtil::CropStr(pFR->strName, 14);
			imgUtil::UNICODEtoBK(strDir, pRec->name, 14, true);

			if (!bRenameOnly)
			{
				pRec->len_blk = 0; // ������� ��������
				pRec->status1 = pFR->nDirNum;
				pRec->status2 = m_sDiskCat.nCurrDirNum; // ����� �������� ��������� �����������
			}
		}
		else
		{
			imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
			pRec->address = pFR->nAddress; // ��������, ���� �� ��������� ��� ����, ����� ����� ������� ������

			if (!bRenameOnly)
			{
				pRec->status1 = 0;

				if (pFR->nAttr & FR_ATTR::HIDDEN)
				{
					pRec->status1 |= 2;
				}

				if (pFR->nAttr & FR_ATTR::PROTECTED)
				{
					pRec->status1 |= 1;
				}

				pRec->status2 = pFR->nDirBelong;
				pRec->len_blk = ByteSizeToBlockSize_l(pFR->nSize); // ������ � ������
				pRec->length = pFR->nSize % 0200000; // ������� �� ����� �� ������ 65535
			}
		}
	}
}

// �� ����� ��������� �� ����������� ������.
// � ��� ��������� ����� �������� ������, �� ��� ��������� �����������
void CBKFloppyImage_AODos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� �������������

	if (pFR->nSpecificDataLength) // ���������������, ������ ���� ���� �������� ������
	{
		if (pRec->status2 & 0200)
		{
			pFR->nAttr |= FR_ATTR::DELETED;
		}
		else if (pRec->status1 & 0200)
		{
			pFR->nAttr |= FR_ATTR::BAD;

			if (pRec->name[0] == 0)
			{
				pRec->name[0] = 'B';
				pRec->name[1] = 'A';
				pRec->name[2] = 'D';
			}
		}

		if (pRec->len_blk == 0)
		{
			// ���� ����������
			pFR->nAttr |= FR_ATTR::DIRECTORY;
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
			pFR->strName = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
			pFR->nDirBelong = pRec->status2 & 0177;
			pFR->nDirNum = pRec->status1 & 0177;
			pFR->nBlkSize = 0;

			// ������� �� ������ ������, ����� ����� ���� ����
			if (pRec->address && pRec->address < 0200)
			{
				pFR->nAttr |= FR_ATTR::LINK;
				pFR->nRecType = BKDirDataItem::RECORD_TYPE::LINK;
			}

//          pFR->nAddress = 0; // ���� ���������� address, �� ���������� �������� ������������ �������,
			// � � ��������� �� �� ��������� ��� ������, �� ������ ����
		}
		else
		{
			// ���� ����
			pFR->nRecType = BKDirDataItem::RECORD_TYPE::FILE;
			std::string name = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
			std::string ext;
			size_t l = name.length();
			size_t t = name.rfind('.');

			if (t != std::string::npos) // ���� ���������� ����
			{
				ext = name.substr(t, l);
				name = strUtil::trim(name.substr(0, t));
			}

			if (!ext.empty())
			{
				name += ext;
			}

			pFR->strName = name;
			pFR->nDirBelong = pRec->status2 & 0177;
			pFR->nDirNum = 0;
			pFR->nBlkSize = pRec->len_blk;

			// if (pFR->nAttr & FR_ATTR::DELETED)
			// {
			//  pFR->nDirBelong = 0; // � �� �������� ����� ������ �� �����. � �� ������, ��� � �������� dir_num == 255 ����.
			// }

			if (pRec->status1 & 2)
			{
				pFR->nAttr |= FR_ATTR::HIDDEN;
			}

			if (pRec->status1 & 1)
			{
				pFR->nAttr |= FR_ATTR::PROTECTED;
			}
		}

		pFR->nAddress = pRec->address;
		/*  ���������� ������ ����� �����.
		�.�. � curr_record.length �������� ������� ����� �� ������ 0200000, ��� �� ����� � ������ ���� ������
		������� ������ �� 0200000 �.�. ������� � ������ ������ �� 128 ������.(128������ == 65536.== 0200000)

		hw = (curr_record.len_blk >>7 ) << 16; // ��� ������� ����� �������� �����
		hw = (curr_record.len_blk / 128) * 65536 = curr_record.len_blk * m_nSectorSize
		*/
		uint32_t hw = (pRec->len_blk << 9) & 0xffff0000; // ��� ������� ����� �������� �����
		pFR->nSize = hw + pRec->length; // ��� ��.����� + ��.�����
		pFR->nStartBlock = pRec->start_block;
	}
}


void CBKFloppyImage_AODos::OnReopenFloppyImage()
{
	m_sDiskCat.bHasDir = true;
	m_sDiskCat.nMaxDirNum = 0177;
}

bool CBKFloppyImage_AODos::IsEndOfCat(AodosFileRecord *pRec)
{
	if ((pRec->status1 | pRec->status2 | pRec->name[0] | pRec->name[1]) == 0)
	{
		return true;    // ����� �������� ��� ����� 1.77
	}

	return false;
}


bool CBKFloppyImage_AODos::ReadCurrentDir()
{
	if (!CBKFloppyImage_Prototype::ReadCurrentDir())
	{
		return false;
	}

	// ���� ������ ��� ������ �� ���������� - �� ������, ������ �������� ����������
	if (!m_pCatBuffer)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

	memset(m_pCatBuffer.get(), 0, m_nCatSize);
	BKDirDataItem AFR; // ��������� ����������� ������
	auto pRec = reinterpret_cast<AodosFileRecord *>(AFR.pSpecificData); // � � ��� ����� ������������ ������

	if (!SeektoBlkReadData(0, m_pCatBuffer.get(), m_nCatSize)) // ������ �������
	{
		return false;
	}

	int files_count = 0;
	int files_total = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])); // ������ ����� ���-�� ������.
	m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_FIRST_FILE_BLOCK])); // ���� ������ ������

	if (m_sDiskCat.nDataBegin <= 0 || m_sDiskCat.nDataBegin > 40) // ���� ��� ������������ ����������
	{
		m_sDiskCat.nDataBegin = 024; // ���� ������ �� ���������.
	}

	m_sDiskCat.nTotalRecs = m_nMKCatSize; // ��� � ��� ����� ��������, �� ���� ���� ����� ������� ����� ���������� �������
	int used_size = 0;

	for (unsigned int i = 0; i < m_nMKCatSize; ++i) // ���� �� ����� ��������
	{
		m_nMKLastCatRecord = i;

		if (files_count >= files_total) // ����� ���������, �������
		{
			break;
		}

		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;    // ����� �������� ��� ����� 1.77
		}

		// ����������� ������ � �������� � ������
		AFR.clear();
		AFR.nSpecificDataLength = sizeof(AodosFileRecord);
		*pRec = m_pDiskCat[i]; // �������� ������� ������ ��� ����
		ConvertRealToAbstractRecord(&AFR);

		if (m_pDiskCat[i].status2 & 0200)
		{
			// �������� �� ���������,
			// ������ �������� ���� �� ���������, �� ��������� �� �� ���
		}
		else
		{
			// ��������! �������� � ����� 2.02 ���� ���������, �� ��� ���������� ������, �� ����
			files_count++;
		}

		if (!(AFR.nAttr & FR_ATTR::DELETED))
		{
			if (m_pDiskCat[i].len_blk == 0)
			{
				// ���� �������
				if (!AppendDirNum(m_pDiskCat[i].status2 & 0177))
				{
					// ��������� ������������ ������� ����������
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
				}
			}
			else
			{
				// ���� ����
				used_size += m_pDiskCat[i].len_blk;
			}
		}

		// �������� ������ �� ������, ������� � ������ ���������� �����������.
		if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
		{
			m_sDiskCat.vecFC.push_back(AFR);
		}
	}

	m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_count;
	int imgSize = *(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_DISK_SIZE]));

	if ((imgSize == 0) || (imgSize > 32767)) // �.�. ���� ��� ����� �������������, ���� ���� ��� ��� ��������, �� ��� ����� ��������� �������� ����� � ������. � ������.
	{
		/*��� ����� ��������.
		800�� ����� ����� ���� ������ 800 ��, �.�. ���������, �������� �������,
		����� ��-�� ����������� �� ������ ������ ������� � ����� �����.
		� � ������ �������, � �����, ������ ����������� ����� ����� ���� ����� ��, � ��� ����� � �����,
		� ������ ����� ����� ���� ������ 800��.
		*/
		imgSize = max(1600, (int)m_sParseImgResult.nImageSize / 512);
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif
	m_sDiskCat.nTotalBlocks = imgSize - m_sDiskCat.nDataBegin;
	m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
	return true;
}

bool CBKFloppyImage_AODos::WriteCurrentDir()
{
	if (!CBKFloppyImage_Prototype::WriteCurrentDir())
	{
		return false;
	}

	// ���� ������ ��� ������ �� ���������� - �� ������, ������ �������� ����������
	if (m_pCatBuffer == nullptr)
	{
		m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
		return false;
	}

#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	if (!SeektoBlkWriteData(0, m_pCatBuffer.get(), m_nCatSize)) // ��������� ������� ��� ����
	{
		return false;
	}

	if (!WriteData(m_pCatBuffer.get(), m_nCatSize)) // � ������ �����.
	{
		return false;
	}

	return true;
}

bool CBKFloppyImage_AODos::ChangeDir(BKDirDataItem *pFR)
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

bool CBKFloppyImage_AODos::VerifyDir(BKDirDataItem *pFR)
{
	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� ��������
		// ��������, ����� ����� ���������� ��� ����
		int nIndex = FindRecord2(pRec, false); // �� ��� �� ����� ����� ����������. �� ����� ������ �� ����� ���������.

		if (nIndex >= 0)
		{
			return true;
		}
	}

	return false;
}

bool CBKFloppyImage_AODos::CreateDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// ���� ����� �������� ������ ��� ������,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // �� �������� � ���� �� ������ �� ������.
	}

	if (m_sDiskCat.nFreeRecs <= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
		return false;
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� ��������
		pFR->nDirBelong = pRec->status2 = m_sDiskCat.nCurrDirNum; // ����� �������� ��������� �����������
		// ��������, ����� ����� ���������� ��� ����
		int nInd = FindRecord2(pRec, false); // �� ��� �� ����� ����� ����������. �� ����� ������ �� ����� ���������.

		if (nInd >= 0)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
			pFR->nDirNum = m_pDiskCat[nInd].status2 & 0177; // � ������ ������ ����� ����������
		}
		else
		{
			unsigned int nIndex = 0;
			// ����� ��������� ����� � ��������.
			bool bHole = false;
			bool bFound = false;

			for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
			{
				if (IsEndOfCat(&m_pDiskCat[i]))
				{
					break;
				}

				if ((m_pDiskCat[i].status2 & 0200) && (m_pDiskCat[i].len_blk == 0))
				{
					// ���� ����� ����� � ������� ���� ��� ����������
					bHole = true;
					nIndex = i;
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				nIndex = m_nMKLastCatRecord;

				if (nIndex < m_nMKCatSize) // ���� � ����� �������� ������ ���� �����
				{
					bFound = true; // �� �����, ����� - ���
				}
			}

			if (bFound)
			{
				pFR->nDirNum = pRec->status1 = AssignNewDirNum(); // ��������� ����� ����������.

				if (pFR->nDirNum == 0)
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
				}

				// ���� ������ �� ���������, �������� ����������
				if (bHole)
				{
					// ��������� ���� ������ ������ �������� ����������
					m_pDiskCat[nIndex] = pRec;
				}
				else
				{
					// ���� ����� ��������� ������� � ����� ��������
					int nHoleSize = m_pDiskCat[nIndex].len_blk;
					int nStartBlock = m_pDiskCat[nIndex].start_block;
					// ��������� ���� ������
					m_pDiskCat[nIndex] = pRec;
					// ��������� ������� ����� ��������
					AodosFileRecord hole;
					hole.start_block = nStartBlock + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					// � ������ � ����� � ��������� �������
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;
				}

				// ��������� �������
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) += 1; // �������� ��������� - ���������� ������
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

bool CBKFloppyImage_AODos::DeleteDir(BKDirDataItem *pFR)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// ���� ����� �������� ������ ��� ������,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // �� �������� � ���� �� ������ �� ������.
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // ��� �������� �� ������� ��������
	}

	if (pFR->nAttr & FR_ATTR::DIRECTORY)
	{
		ConvertAbstractToRealRecord(pFR);
		// ������ ���� �� ����� � ����������.
		bool bExist = false;

		for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
		{
			if (IsEndOfCat(&m_pDiskCat[i]))
			{
				break;
			}

			if (m_pDiskCat[i].status2 & 0200)
			{
				// ���� �������� ���� - �����, �� ����������
				continue;
			}

			if ((m_pDiskCat[i].status2 & 0177) == pFR->nDirNum)
			{
				bExist = true; // ����� ����, ������������� ���� ����������
				break;
			}
		}

		// ���� ���� - �� �������.
		if (bExist)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EMPTY;
		}
		else
		{
			// ������� ����� ������ ����������.
			auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� �������
			int nIndex = FindRecord(pRec);

			if (nIndex >= 0) // ���� �����
			{
				m_pDiskCat[nIndex].status2 |= 0200; // ������� ��� ��������
				*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) -= 1; // �������� ��������� - ���������� ������
				bRet = WriteCurrentDir(); // �������� ����������
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

bool CBKFloppyImage_AODos::GetNextFileName(BKDirDataItem *pFR)
{
	pFR->clear();
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ������������ ������
#ifdef _DEBUG
	DebugOutCatalog(m_pDiskCat);
#endif

	// � ������� �� ��� �����. �� �� ������.
	while (m_nCatPos < m_nMKLastCatRecord) // ���� �� ����� ��������
	{
		if (IsEndOfCat(&m_pDiskCat[m_nCatPos]))
		{
			break;
		}

		if ((m_pDiskCat[m_nCatPos].status2 & 0177) == m_sDiskCat.nCurrDirNum) // ���� ������ � ������� ����������
		{
			*pRec = m_pDiskCat[m_nCatPos];
			pFR->nSpecificDataLength = sizeof(AodosFileRecord);
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

bool CBKFloppyImage_AODos::RenameRecord(BKDirDataItem *pFR)
{
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ������������ ������
	int nIndex = FindRecord(pRec); // ������ ����� �

	if (nIndex >= 0)
	{
		ConvertAbstractToRealRecord(pFR, true); // ������ ������������� ��� �������� ������
		m_pDiskCat[nIndex] = *pRec; // ������ ������������� � ��������
		return WriteCurrentDir(); // �������� �������
	}

	// ���-�� �� ���
	return false;
}


bool CBKFloppyImage_AODos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // ���� ��� �� ���� - ����� � �������
	}

	ConvertAbstractToRealRecord(pFR);
	size_t len = pFR->nSize;
	size_t sector = pFR->nStartBlock;

	if (SeekToBlock(sector)) // ������������ � ������ �����
	{
		// ��������� ���� ������������� ������ ������, ��������� ��� �������
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

bool CBKFloppyImage_AODos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
	// ���� �������� ���� � ����� ������� - �� �� � ����� �������� ��������������� ����
	if (bNeedSqueeze)
	{
		if (!Squeeze()) // ���� ������������� ���������
		{
			bNeedSqueeze = false;
			return false; // �� ������� � �������
		}
	}

	bNeedSqueeze = false;
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

	if (m_bFileROMode)
	{
		// ���� ����� �������� ������ ��� ������,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return false; // �� �������� � ���� �� ������ �� ������.
	}

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // ���� ��� �� ���� - ����� � �������
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� ��������
	pFR->nDirBelong = pRec->status2 = m_sDiskCat.nCurrDirNum; // ����� �������� ��������� �����������

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

	// ������, ����� ����� ��� ����� ��� ����������.
	int nInd = FindRecord2(pRec, true);

	if (nInd >= 0)
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
		*pRec = m_pDiskCat[nInd];
		ConvertRealToAbstractRecord(pFR); // � ������� ��������� ������ ����������.
		return false;
	}

	bool bRet = false;
	unsigned int nIndex = 0;
	// ����� ��������� ����� � ��������.
	bool bFound = false;
	bool bHole = false; // ��� �����, ����� ��� ����� � �����, �.�. ������ ������� ��������� ��������

	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
	{
		if (IsEndOfCat(&m_pDiskCat[i]))
		{
			break;
		}

		if ((m_pDiskCat[i].status2 & 0200) && (m_pDiskCat[i].len_blk >= pRec->len_blk))
		{
			// ���� ����� ����� ����������� �������
			bHole = true;
			nIndex = i;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		nIndex = m_nMKLastCatRecord;

		if (nIndex < m_nMKCatSize) // ���� � ����� �������� ������ ���� �����
		{
			bFound = true; // �� �����, ����� - ���
		}
	}

	if (bFound)
	{
		// ����� ���������
		int nFirstFreeBlock = pRec->start_block = m_pDiskCat[nIndex].start_block;
		int nHoleSize = m_pDiskCat[nIndex].len_blk;

		if (bHole)
		{
			// ���� ����� ����� � ��������, ���� ���������� �������, ��� �� ����������, ���� ������ ����� ���������
			if (nHoleSize > pRec->len_blk)
			{
				// ��������� �������
				unsigned int i = m_nMKLastCatRecord + 1; // ����� ����� ��������

				if (i >= m_nMKCatSize) // ���� ������� ���������� ������
				{
					m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
					goto l_squeeze;
				}
				else
				{
					// ����������
					int cnt = 0; // �������

					while (i > nIndex)
					{
						m_pDiskCat[i] = m_pDiskCat[i - 1];
						i--;
						cnt++;
					}

					// ��������� ���� ������
					m_pDiskCat[nIndex] = pRec;
					AodosFileRecord hole;
					hole.status1 = 0377;
					hole.status2 = 0377;
					hole.start_block = pRec->start_block + pRec->len_blk;
					hole.len_blk = nHoleSize - pRec->len_blk;
					imgUtil::UNICODEtoBK("<HOLE>", hole.name, 14, true);
					hole.length = (hole.len_blk * m_nBlockSize) % 0200000;
					// � ������ � ����� � �����
					m_pDiskCat[nIndex + 1] = hole;
					m_nMKLastCatRecord++;

					// ���� �� � �������� ���������, ���������� ������ �� ���� ������ - �� �� ��������� � ����� ��������
					// ��� ���� ���� ��������� ������ ������� ���������� ����� � ������ 032
					if (cnt == 1)
					{
						*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_FIRST_FREE_BLOCK])) = hole.start_block;
					}
				}
			}
			else
			{
				// ������ ���������� �� ����, ������
				// ��������� ���� ������
				m_pDiskCat[nIndex] = pRec;
			}
		}
		else
		{
			// ���� ����� ��������� ������� � ����� ��������
			// ��������� ���� ������
			m_pDiskCat[nIndex] = pRec;
			// ��������� ������� ����� ��������
			AodosFileRecord hole;
			hole.start_block = pRec->start_block + pRec->len_blk;
			hole.len_blk = nHoleSize - pRec->len_blk;
			// � ������ � ����� � ��������� �������
			m_pDiskCat[nIndex + 1] = hole;
			m_nMKLastCatRecord++;
		}

		if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
		{
			// ������������ � �����
			if (SeekToBlock(nFirstFreeBlock))
			{
				int nReaded;
				int nLen = pFR->nSize; // ������ �����

				if (nLen == 0)
				{
					nLen++; // ������ ����� ������� ����� �������������, ������� �������������.
				}

				while (nLen > 0)
				{
					memset(m_mBlock, 0, COPY_BLOCK_SIZE);
					nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
					memcpy(m_mBlock, pBuffer, nReaded);
					pBuffer += nReaded;

					// ����� � ������������� �� ��������.
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
					// ���� ������ �� ���������, �������� ����������
					*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) += 1;
					// ������� ��������� �������
					bRet = WriteCurrentDir();
					m_sDiskCat.nFreeRecs--;
					m_sDiskCat.nFreeBlocks -= pRec->len_blk;
				}
			}
		}
	}
	else
	{
		// ���� ��� �����, �� ��������� ��������� ����� �� ����� ���� - ������ ������������� � ����� � �����.
		m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_NEED_SQEEZE;
l_squeeze:
		bNeedSqueeze = true;
	}

	return bRet;
}


bool CBKFloppyImage_AODos::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = false;

	if (m_bFileROMode)
	{
		// ���� ����� �������� ������ ��� ������,
		m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
		return bRet; // �� �������� � ���� �� ������ �� ������.
	}

	if (pFR->nAttr & (FR_ATTR::DIRECTORY | FR_ATTR::LINK))
	{
		m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
		return false; // ���� ��� �� ���� - ����� � �������
	}

	if (pFR->nAttr & FR_ATTR::DELETED)
	{
		return true; // ��� �������� �� ������� ��������
	}

	ConvertAbstractToRealRecord(pFR);
	auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // ��� ��� ������ ���� �������
	// ����� � � ��������
	int nIndex = FindRecord(pRec);

	if (nIndex >= 0) // ���� �����
	{
		if ((m_pDiskCat[nIndex].status1 & 2) && !bForce)
		{
			m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
		}
		else
		{
			m_pDiskCat[nIndex].status1 = 0377; // ������� ��� ��������
			m_pDiskCat[nIndex].status2 = 0377;
			*(reinterpret_cast<uint16_t *>(&m_pCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) -= 1; // �������� ��������� - ���������� ������
			bRet = WriteCurrentDir(); // �������� ����������
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

// ������������� �����
bool CBKFloppyImage_AODos::Squeeze()
{
	m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	unsigned int p = 0;

	while (p <= m_nMKLastCatRecord)
	{
		if (m_pDiskCat[p].status2 == 0200) // ���� ����� �����
		{
			unsigned int n = p + 1; // ������ ��������� ������

			if (n >= m_nMKLastCatRecord) // ���� p - ��������� ������
			{
				// ��� ���� ���������� �����
				m_pDiskCat[p].status1 = m_pDiskCat[p].status2 = 0;
				memset(m_pDiskCat[p].name, 0, 14);
				m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
				m_nMKLastCatRecord--;
				break;
			}

			if (m_pDiskCat[n].status2 & 0200) // � �� ������ ����� �����
			{
				m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // ������ - ��������

				// � ������ ������.
				while (n < m_nMKLastCatRecord) // �������� �������
				{
					m_pDiskCat[n] = m_pDiskCat[n + 1];
					n++;
				}

				memset(&m_pDiskCat[m_nMKLastCatRecord--], 0, sizeof(AodosFileRecord));
				continue; // � �� �������
			}

			size_t nBufSize = static_cast<size_t>(m_pDiskCat[n].len_blk) * m_nBlockSize;
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

						// ������ ���� ������ ������� ��������.
						std::swap(m_pDiskCat[p], m_pDiskCat[n]); // �������� ������ �������
						std::swap(m_pDiskCat[p].start_block, m_pDiskCat[n].start_block); // ��������� ����� ������ ��� ����.
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

	WriteCurrentDir(); // ��������
	return bRet;
}

#ifdef _DEBUG
// ���������� ����� ��������
#pragma warning(disable:4996)
void CBKFloppyImage_AODos::DebugOutCatalog(AodosFileRecord *pRec)
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
		fprintf(log, "%03d %03d  %03d %-14s %06o %06o  %06o  %06o\n", i, pRec[i].status1, pRec[i].status2, name, pRec[i].start_block, pRec[i].len_blk, pRec[i].address, pRec[i].length);
	}

	fclose(log);
}
#endif
