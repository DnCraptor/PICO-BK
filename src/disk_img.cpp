#include "disk_img.h"
#include "debug.h"
#include "stdint.h"
extern "C" {
#include "string.h"
}
static PARSE_RESULT parse_result;

/* определение типа образа, на выходе номер, соответствующий определённой ОС
    0 - образ не опознан, -1 - ошибка
*/
static PARSE_RESULT ParseImage(const char* fname, unsigned long nBaseOffset)
{
	union
	{
		uint8_t pSector[BLOCK_SIZE];
		uint16_t wSector[BLOCK_SIZE / 2];
	};
	PARSE_RESULT ret;
	strncpy(ret.strName, fname, 256);
	ret.nBaseOffset = nBaseOffset;
	ret.imageOSType = IMAGE_TYPE::ERROR_NOIMAGE; // предполагаем изначально такое

	if (!BKFloppyImgFile.Open(fname, false))
	{
		DBGM_PRINT(("Unable to open %s", fname.c_str()));
		return ret;
	}

	ret.nImageSize = BKFloppyImgFile.GetFileSize(); // получим размер файла
	BKFloppyImgFile.SeekTo00();
	nBaseOffset /= BLOCK_SIZE; // смещение в LBA
    DBGM_PRINT(("%s nImageSize: %d nBaseOffset: %d", fname.c_str(), ret.nImageSize, nBaseOffset));
	if (BKFloppyImgFile.ReadLBA(pSector, nBaseOffset + 0, 1))
	{
		bool bAC1 = false; // условие андос 1 (метка диска)
		bool bAC2 = false; // условие андос 2 (параметры диска)

		// Получим признак системного диска
		if (wSector[0] == 0240 /*NOP*/) {
			if (wSector[1] != 5 /*RESET*/)
			{
				ret.bImageBootable = true;
			}
		}
		DBGM_PRINT(("bImageBootable: %d", ret.bImageBootable));

		// проверим тут, а то потом, прямо в условии присваивания не работают из-за оптимизации проверки условий
		bAC1 = (memcmp(strID_Andos, reinterpret_cast<char *>(pSector + 04), strlen(strID_Andos)) == 0);
		//      секторов в кластере                   число файлов в корне                          медиадескриптор                   число секторов в одной фат
		bAC2 = ((pSector[015] == 4) && (*(reinterpret_cast<uint16_t *>(pSector + 021)) == 112) && (pSector[025] == 0371) && (*(reinterpret_cast<uint16_t *>(pSector + 026)) == 2));
        DBGM_PRINT(("bAC1: %d bAC2: %d", bAC1, bAC2));
		// Узнаем формат образа
		if (wSector[0400 / 2] == 0123456) // если микродос
		{
			DBGM_PRINT(("MKDOS"));
			// определяем с определённой долей вероятности систему аодос
			if (pSector[4] == 032 && (wSector[6 / 2] == 256 || wSector[6 / 2] == 512))
			{
				// либо расширенный формат
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::AODOS;
			}
			else if (memcmp(strID_Aodos, reinterpret_cast<char *>(pSector + 4), strlen(strID_Aodos)) == 0)
			{
				// либо по метке диска
				// аодос 1.77 так не распознаётся. его будем считать просто микродосом. т.к. там нету директорий
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::AODOS;
			}
			else if (memcmp(strID_Nord, reinterpret_cast<char *>(pSector + 4), strlen(strID_Nord)) == 0)
			{
				// предполагаем, что это метка для системы
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::NORD;
			}
			else if (memcmp(strID_Nord_Voland, reinterpret_cast<char *>(pSector + 0xa0), strlen(strID_Nord_Voland)) == 0)
			{
				// мультизагрузочные диски - тоже будем опознавать как норд
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::NORD;
			}
			/* обнаружилась нехорошая привычка AODOS и возможно NORD мимикрировать под MKDOS,
			так что проверка на него - последней, при этом, т.к. MKDOSу похрен на ячейки 04..012
			то иногда, если там были признаки аодоса, мкдосный диск принимается за аодосный.
			но это лучше, чем наоборот, ибо в первом случае просто в имени директории будет глючный символ,
			а во втором - вообще все директории пропадают.
			*/
			else if (wSector[0402 / 2] == 051414)
			{
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::MKDOS;
			}
			// тут надо найти способ как определить прочую экзотику
			else
			{
				int nRet = AnalyseMicrodos(&BKFloppyImgFile, nBaseOffset, pSector);

				switch (nRet)
				{
					case 0:
						ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
						ret.imageOSType = IMAGE_TYPE::MIKRODOS;
						break;

					case 1:
						ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
						ret.imageOSType = IMAGE_TYPE::AODOS;
						break;

					case 2:
						ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
						ret.imageOSType = IMAGE_TYPE::NORD;
						break;
				}
			}
		}
		// проверка на формат FAT12, если надпись "FAT12"
		// однако, в некоторых андосах, нету идентификатора фс, зато есть метка андос,
		// но бывает, что и метки нету, зато параметры BPB специфические
		else if ( // если хоть что-то выполняется, то скорее всего это FAT12, но возможны ложные срабатывания из-за мусора в этой области
		    bAC1 || bAC2 ||
		    (memcmp(strID_FAT12, reinterpret_cast<char *>(pSector + 066), strlen(strID_FAT12)) == 0)
		)
		{
			DBGM_PRINT(("FAT12"));
			uint8_t nMediaDescriptor = pSector[025];
			int nBootSize = *(reinterpret_cast<uint16_t *>(pSector + 016)) * (*(reinterpret_cast<uint16_t *>(pSector + 013)));
			uint8_t b[BLOCK_SIZE];
			BKFloppyImgFile.ReadLBA(b, nBaseOffset + nBootSize / BLOCK_SIZE, 1);

			if (b[1] == 0xff && b[0] == nMediaDescriptor) // если медиадескриптор в BPB и фат совпадают, то это точно FAT12
			{
				if (bAC1 || bAC2) // если одно из этих условий - то это скорее всего ANDOS
				{
					ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 023));
					ret.imageOSType = IMAGE_TYPE::ANDOS;
				}
				else if (memcmp(strID_DXDOS, reinterpret_cast<char *>(pSector + 04), strlen(strID_DXDOS)) == 0) // если метка диска DX-DOS, то это скорее всего DX-DOS, других вариантов пока нет.
				{
					ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 023));
					ret.imageOSType = IMAGE_TYPE::DXDOS;
				}
				else
				{
					ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 023));
					ret.imageOSType = IMAGE_TYPE::MSDOS; // тут только MS DOS остался
				}
			}
			else
			{
				ret.imageOSType = IMAGE_TYPE::UNKNOWN;
				DBGM_PRINT(("UNKNOWN"));
			}
		}
		// проверка на предположительно возможно НС-ДОС
		else if (wSector[2 / 2] == 012700 && wSector[4 / 2] == 0404 && wSector[6 / 2] == 0104012)
		{
			DBGM_PRINT(("NCDOS"));
			// нс-дос никак себя не выделяет среди других ос, поэтому будем её определять по коду.
			ret.nPartLen = 1600;
			ret.imageOSType = IMAGE_TYPE::NCDOS;
		}
		else
		{
			// дальше сложные проверки
			// выход из функций: 1 - да, это проверяемая ОС
			//                   0 - нет, это какая-то другая ОС
			//                  -1 - ошибка ввода вывода
			// проверим, а не рт11 ли у нас
			int t = IsRT11_old(&BKFloppyImgFile, nBaseOffset, pSector);
            DBGM_PRINT(("IsRT11_old: %d", t));
			switch (t)
			{
				case 0:
					t = IsRT11(&BKFloppyImgFile, nBaseOffset, pSector);
                    DBGM_PRINT(("IsRT11: %d", t));
					switch (t)
					{
						case 0:
						{
							// нет, не рт11
							// проверим, а не ксидос ли у нас
							t = IsCSIDOS3(&BKFloppyImgFile, nBaseOffset, pSector);
                            DBGM_PRINT(("IsCSIDOS3: %d", t));
							switch (t)
							{
								case 0:
									// нет, не ксидос
									// проверим на ОПТОК
									t = IsOPTOK(&BKFloppyImgFile, nBaseOffset, pSector);
									DBGM_PRINT(("IsOPTOK: %d", t));
									switch (t)
									{
										case 0:
											// нет, не опток
											// проверим на Holography
											t = IsHolography(&BKFloppyImgFile, nBaseOffset, pSector);
											DBGM_PRINT(("IsHolography: %d", t));
											switch (t)
											{
												case 0:
													// нет, не Holography
													// проверим на Dale OS
													t = IsDaleOS(&BKFloppyImgFile, nBaseOffset, pSector);
													DBGM_PRINT(("IsDaleOS: %d", t));
													switch (t)
													{
														case 0:
															ret.imageOSType = IMAGE_TYPE::UNKNOWN;
															break;

														case 1:
															ret.nPartLen = 1600;
															ret.imageOSType = IMAGE_TYPE::DALE;
															break;
													}
													break;
												case 1:
													ret.nPartLen = 800;
													ret.imageOSType = IMAGE_TYPE::HOLOGRAPHY;
													break;
											}
											break;
										case 1:
											ret.nPartLen = 720;
											ret.imageOSType = IMAGE_TYPE::OPTOK;
											break;
									}

									break;

								case 1:
									ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 2));
									ret.imageOSType = IMAGE_TYPE::CSIDOS3;
									break;
							}

							break;
						}

						case 1:
							ret.nPartLen = (ret.nImageSize + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
							ret.imageOSType = IMAGE_TYPE::RT11;
							break;
					}

					break;

				case 1:
					ret.nPartLen = (ret.nImageSize + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
					ret.imageOSType = IMAGE_TYPE::RT11;
					break;
			}
		}
	}
	DBGM_PRINT(("nPartLen: %d imageOSType: %d", ret.nPartLen, ret.imageOSType))
	BKFloppyImgFile.Close();
	return ret;
}

bool is_browse_os_supported() {
   return parse_result.imageOSType == IMAGE_TYPE::MKDOS;
}

void detect_os_type(const char* path, char* os_type, size_t sz) {
    try {
        parse_result = parserImage->ParseImage(path, 0);
        delete parserImage;
        auto s = std::to_string(parse_result.nImageSize >> 10) + " KB " + CBKParseImage::GetOSName(parse_result.imageOSType);
        if (parse_result.bImageBootable) {
            s += " [bootable]";
        }
        DBGM_PRINT(("detect_os_type: %s %s", path, s.c_str()));
        strncpy(os_type, s.c_str(), sz);
    } catch(...) {
        DBGM_PRINT(("detect_os_type: %s FAILED", path));
        strncpy(os_type, "DETECT OS TYPE FAILED", sz);
    }
}


#if EXT_DRIVES_MOUNT
bool mount_img(const char* path) {
    DBGM_PRINT(("mount_img: %s", path));
    if ( !is_browse_os_supported() ) {
        DBGM_PRINT(("mount_img: %s unsupported file type (resources)", path));
        return false;
    }
    m_cleanup_ext();
    try {
        CBKImage* BKImage = new CBKImage();
        BKImage->Open(parse_result);
        BKImage->ReadCurrentDir(BKImage->GetTopItemIndex());
        BKImage->Close();
        DBGM_PRINT(("mount_img: %s done", path));
    } catch(...) {
        DBGM_PRINT(("mount_img: %s FAILED", path));
        return false;
    }
    return true;
}
#endif
