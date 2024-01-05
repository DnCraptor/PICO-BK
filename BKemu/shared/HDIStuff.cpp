#include "pch.h"
#include "HDIStuff.h"
#include <codecvt>

std::wstring HDIStuff::GenerateSerialNumber()
{
	// тут будет генерация серийного номера Серийный номер такого типа
	// BK-AAAADDDDDDDD, где A - буква, D - цифра.
	std::wstring strSerialNumber = L"BK-AAAADDDDDDDD";
	wchar_t ch;

	for (int i = 3; i < 7; ++i)
	{
		ch = L'A' + wchar_t(float(rand()) / float(RAND_MAX + 1) * (L'Z' - L'A'));
		strSerialNumber[i] = ch;
	}

	for (int i = 7; i < 15; ++i)
	{
		ch = L'0' + wchar_t(float(rand()) / float(RAND_MAX + 1) * (L'9' - L'0'));
		strSerialNumber[i] = ch;
	}

	return strSerialNumber;
}

// копирование строки в системный сектор.
void HDIStuff::CopyString2Sys(uint8_t *dst, const std::wstring &str, const size_t max_len)
{
#pragma warning(disable:4996)
	const size_t sizeOfString = str.length();
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	std::string lpszAscii = converterX.to_bytes(str);

	for (size_t i = 0; i < max_len; ++i)
	{
		dst[i] = (i < sizeOfString) ? lpszAscii[i] : ' ';
	}

	// а теперь по традиции переставим байты в словах
	for (size_t i = 0; i < max_len; i += 2)
	{
		std::swap(dst[i], dst[i + 1]);
	}
}

void HDIStuff::InverseSector(USector &sector) noexcept
{
	for (uint16_t &i : sector.w)
	{
		i = ~i;
	}
}

// подсчёт КС сектора
bool HDIStuff::CheckCS(const USector *sector) noexcept
{
	uint8_t cs = 0;

	for (int i = 0; i < SECTOR_SIZEB - 1; ++i)
	{
		cs += sector->b[i];
	}

	cs = -cs;
	return (sector->b[SECTOR_SIZEB - 1] == cs && sector->b[SECTOR_SIZEB - 2] == 0xA5);
}

static SYS_SECTOR g_sys_sector =
{
	0x045a, // 0
	0,      // 1
	0xc837, // 2
	0,      // 3
	0,      // 4
	0,      // 5
	0,      // 6
	{0, 0, 0}, // 7,8,9
	{0},    // 10
	1,      // 20
	1,      // 21
	4,      // 22
	{0},    // 23
	{0},    // 27
	0x8001, // 47
	0,      // 48
	0x200,  // 49
	0x4000, // 50
	{0},    // 51
	0,      // 57,58
	0,      // 59
	0,      // 60,61
	{0}     // 62-255
};

void HDIStuff::CreateHDISector(IMGFormat *imgf, SYS_SECTOR *bufSector, const std::wstring &sn, const std::wstring &mn)
{
	memcpy(bufSector, &g_sys_sector, sizeof(SYS_SECTOR));
	bufSector->cylinders = imgf->C;
	bufSector->heads = imgf->H;
	bufSector->sectors = imgf->S;
	bufSector->raw_bytes_per_sector = SECTOR_SIZEB;
	bufSector->raw_bytes_per_track = SECTOR_SIZEB * imgf->S;
	const uint32_t nTotalSectors = imgf->C * imgf->H * imgf->S; // произведение максимальных значений влазит в 32 бита
	bufSector->total_used_sectors = nTotalSectors;
	bufSector->capacity_in_sectors = nTotalSectors + 1;
	CopyString2Sys(bufSector->serial_number, sn, 20);
	CopyString2Sys(bufSector->model_name, mn, 40);
	CopyString2Sys(bufSector->fw_version, L"v1.0.0.0", 8);
	auto pSector = reinterpret_cast<uint8_t *>(bufSector);
	// сформируем контрольную сумму
	uint8_t cs = 0;
	pSector[SECTOR_SIZEB - 2] = 0xA5;

	for (int i = 0; i < SECTOR_SIZEB - 1; ++i)
	{
		cs += pSector[i];
	}

	cs = -cs;
	pSector[SECTOR_SIZEB - 1] = cs;
}

// проверка на альтпро.
bool HDIStuff::CheckAltPro(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	USector buf{};
	imgf->nIOStatus = IMGIOSTATUS::IO_OK;

	if ((fseek(f, (AHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(buf.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		// проверяем блок 7 (сектор 8)
		InverseSector(buf);
		// прочитаем геометрию
		imgf->C = buf.w[AHDD_CYL_W];
		imgf->H = buf.w[AHDD_HEAD_W] & 0xff; // нужен только младший байт
		imgf->S = buf.w[AHDD_SEC_W];
		// и количество разделов
		imgf->nLogDiskNum = buf.w[AHDD_LOGD_W] & 0xff;
		uint16_t crc = 012701;

		// посчитаем контрольную сумму
		for (int i = 0; i < (imgf->nLogDiskNum * 2 + 4); ++i)
		{
			crc += buf.w[AHDD_CYL_W - i];
		}

		// если контрольная сумма совпадает.
		if (buf.w[AHDD_PART_W - imgf->nLogDiskNum * 2] == crc)
		{
			bRet = true; // параметры структуры IMGFormat достоверны
		}
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
	}

	return bRet;
}

// проверка на формат Самара
bool HDIStuff::CheckSamara(FILE *f, IMGFormat *imgf)
{
	bool bRet = false;
	USector buf{}, sector0{};
	imgf->nIOStatus = IMGIOSTATUS::IO_OK;

	if ((fseek(f, (SHDD_PT_SEC + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
	        && (fread(sector0.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
	{
		// проверяем блок 1 (сектор 2)
		InverseSector(sector0);
		// Определяем геометрию
		const int nCylVol = sector0.w[SHDD_CYLVOL_W]; // объём цилиндра
		imgf->S = sector0.b[SHDD_SEC_B];        // число секторов на дорожке
		imgf->H = sector0.b[SHDD_HEAD_B] + 1;   // число головок

		// число цилиндров нигде не хранится, и его надо будет вычислить примерное количество
		if (nCylVol && (nCylVol == imgf->S * imgf->H)) // если совпало, значит пока всё правильно
		{
			fseek(f, 0, SEEK_END);
			long len = ftell(f);
			fseek(f, 0, SEEK_SET);
			len = (len / SECTOR_SIZEB) - (imgf->bHDI ? 1 : 0);
			imgf->C = uint16_t(len / nCylVol); // вычислим количество дорожек

			if (len % nCylVol) // если ещё есть неполный цилиндр
			{
				imgf->C++; // его тоже засчитаем
			}

			int nPartitions = 0;
			uint16_t nld = 2;
			imgf->bSamara = true; // предполагаем что формат самарский

			while (nPartitions < 64)
			{
				// получим номер начального блока
				const uint32_t lba = uint32_t{ sector0.w[SHDD_PART_W + nPartitions * 2] } | (uint32_t{ sector0.w[SHDD_PART_W + 1 + nPartitions * 2] } << 16);

				if (lba == 0) // если 0 - разделы кончились
				{
					break;
				}

				// теперь перейдём к разделу и посмотрим его параметры

				if ((fseek(f, (lba + (imgf->bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
				        && (fread(buf.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
				{
					InverseSector(buf);

					if (buf.w[SHDD_NLD_W] != nld) // если номер логического диска не совпал с ожидаемым
					{
						imgf->bSamara = false;  // то либо формат не самарский, либо образ повреждён
						break;
					}
				}
				else
				{
					imgf->bSamara = false;
					break;
				}

				nPartitions++; nld++;
			}

			imgf->nLogDiskNum = nPartitions;
		}

		bRet = imgf->bSamara;
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
	}

	return bRet;
}

// проверка формата и заодно определение геометрии
// выход: true - параметры структуры IMGFormat достоверны
//       false - нет. формат не распознан или ошибка чтения
bool HDIStuff::CheckFormat(FILE *f, IMGFormat *imgf)
{
	USector sector0{};
	bool bRet = false;
	imgf->nIOStatus = IMGIOSTATUS::IO_OK;

	if (fread(sector0.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB)  // прочтём первый сектор.
	{
		// подсчитаем контрольную сумму
		if (CheckCS(&sector0))
		{
			imgf->bHDI = true; // если первый сектор правильный - то это вероятно образ HDI
		}
		else
		{
			imgf->bHDI = false; // если нет - то это просто IMG
			fseek(f, 0, SEEK_SET); // вернёмся к началу образа.
		}

		IMGFormat aimgf(imgf);
		IMGFormat simgf(imgf);
		bool bSamara = false;
		const bool bAltPro = CheckAltPro(f, &aimgf);  // проверка на альтпро.

		if (aimgf.nIOStatus == IMGIOSTATUS::IO_ERROR)
		{
			return false;
		}

		if (bAltPro)
		{
			*imgf = aimgf;
		}
		else
		{
			bSamara = CheckSamara(f, &simgf);   // проверка на формат Самара

			if (simgf.nIOStatus == IMGIOSTATUS::IO_ERROR)
			{
				return false;
			}

			if (bSamara)
			{
				*imgf = simgf;
			}
		}

		bRet = bAltPro || bSamara;
	}
	else
	{
		imgf->nIOStatus = IMGIOSTATUS::IO_ERROR;
	}

	return bRet;
}

std::list<AbstractLD> listLD;
// получение списка разделов c полной информацией о них
bool HDIStuff::getPartitionList(FILE *f, std::list<AbstractLD> *pLD)
{
	IMGFormat imgf;
	USector sector{};
	bool bRet = false;

	if (CheckFormat(f, &imgf))
	{
		imgf.nIOStatus = IMGIOSTATUS::IO_OK;
		AbstractLD ld;

		if (imgf.bSamara)
		{
			// получить список разделов самары
			USector buf{};

			// прочитаем сектор 2, нам нужна таблица разделов.
			if ((fseek(f, (SHDD_PT_SEC + (imgf.bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
			        && (fread(sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
			{
				InverseSector(sector);

				for (int nld = 0; nld < imgf.nLogDiskNum; ++nld)
				{
					uint32_t lba = ((uint32_t{ sector.w[SHDD_PART_W + 1 + nld * 2] } << 16) | uint32_t{ sector.w[SHDD_PART_W + nld * 2] });

					// перемещаемся к началу лог диска
					if (fseek(f, (lba + ((imgf.bHDI) ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
					{
						// читаем начальный блок
						if ((fread(buf.b, 1, SECTOR_SIZEB, f)) == SECTOR_SIZEB)
						{
							InverseSector(buf);
							const uint32_t len = buf.w[SHDD_LD_LEN_W];
							lba++;
							ld.Cb = lba / (imgf.H * imgf.S);
							ld.Hb = (lba / imgf.S) % imgf.H;
							ld.Sb = lba % imgf.S + 1;
							ld.lba = lba;
							ld.len = len;
							const uint32_t nEnd = lba + len; // конец логического диска
							ld.Ce = nEnd / (imgf.H * imgf.S);
							ld.He = (nEnd / imgf.S) % imgf.H;
							ld.Se = nEnd % imgf.S + 1;
							ld.attr = buf.b[SHDD_LD_FLAGS_W * 2]; // атрибуты
							ld.num = buf.b[SHDD_NLD_W]; // номер логического диска
							// тут ещё специфические для самары параметры
							ld.sam_ldr_addr = buf.w[SHDD_ADR_BOOT_W];
							ld.sam_bpb_addr = buf.w[SHHD_ADR_PAR_W];
							ld.sam_pages = buf.w[SHDD_PAGE_W];
							pLD->push_back(ld);
						}
					}
					else
					{
						imgf.nIOStatus = IMGIOSTATUS::IO_ERROR;
						bRet = false;
					}
				}
			}
			else
			{
				imgf.nIOStatus = IMGIOSTATUS::IO_ERROR;
				bRet = false;
			}
		}
		else
		{
			// получить список разделов альтпро
			if ((fseek(f, (AHDD_PT_SEC + (imgf.bHDI ? 1 : 0)) * SECTOR_SIZEB, SEEK_SET) == 0)
			        && (fread(sector.b, 1, SECTOR_SIZEB, f) == SECTOR_SIZEB))
			{
				InverseSector(sector);

				for (int nld = 0; nld < imgf.nLogDiskNum; ++nld)
				{
					uint16_t nCyl = sector.w[AHDD_PART_W - nld * 2]; // первое слово из таблицы разделов
					bool bProtect = false;

					if (short(nCyl) < 0)
					{
						bProtect = true;
						nCyl = ~nCyl;
					}

					const uint16_t nHead = nCyl & 0xF; // номер головки из таблицы разделов
					nCyl >>= 4; // номер дорожки из таблицы разделов
					const uint32_t lba = (nCyl * imgf.H + nHead) * imgf.S;    // рассчитываем начало раздела в блоках
					const uint32_t len = sector.w[AHDD_PART_W - 1 - nld * 2]; // размер логического диска в блоках
					ld.Cb = nCyl;
					ld.Hb = nHead;
					ld.Sb = 1; // всегда 1
					ld.lba = lba;
					ld.len = len;
					const uint32_t nEnd = lba + len; // конец логического диска
					ld.Ce = nEnd / (imgf.H * imgf.S);
					ld.He = (nEnd / imgf.S) % imgf.H;
					ld.Se = nEnd % imgf.S + 1;
					ld.attr = bProtect ? ALD_ATTR::WP : ALD_ATTR::NONE; // атрибуты раздела
					ld.num = 2 + nld; // номер логического диска, начиная с бувы С.
					// тут ещё специфические для самары параметры
					ld.sam_ldr_addr = 02000;
					ld.sam_bpb_addr = 01000;
					ld.sam_pages = 014400;
					pLD->push_back(ld);
				}
			}
			else
			{
				imgf.nIOStatus = IMGIOSTATUS::IO_ERROR;
				bRet = false;
			}
		}
	}

	return bRet;
}

