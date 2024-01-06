/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */


// FDDController.cpp: implementation of the CFDDController class.
// Floppy controller and drives emulation


#include "pch.h"
#include "Board.h"
#include "FDDController.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*
177130
Регистр управления НГМД -- КНГМД и АльтПро.
По чтению:
        01  бит 0: признак 0-ой дорожки
        02  бит 1: готовность к работе
        04  бит 2: защита от записи
      0200  бит 7: запрос на чтение/запись данных из/в регистра данных
    040000  бит 14: при операции чтения - "1" - признак 'прочлось без ошибок', "0" - ошибка,
                    при операции записи - признак 'записывается контр. код на диск'
                    ещё такая инфа про запись - в режиме запись индицирует ситуацию, когда предыдущее
                    требование не было обнаружено (сбойная ситуация).
    0100000 бит 15: признак 0-го сектора (индексного отверстия)
По записи:
       01   биты 0-3: выбор накопителя, один бит - один накопитель
       02
       04
      010
      020   бит 4: включение мотора, "1" - включить, "0" - выключить
      040   бит 5: выбор головки, "1" -- верхняя, "0" -- нижняя.
     0100   бит 6: направление перемещения головок "1" - вперёд, к центру  "0" - назад, к нулевой
     0200   бит 7: шаг головки, "1" - выполнить шаг, "0" - не шагать
     0400   бит 8: признак "начало чтения"
                    при установке разряда устройство частично приводится
                    в исходное состояние  и по очистке  разряда  готово
                    осуществлять поиск маркера. (т.е. надо сперва его установить, подождать немного и
                    сбросить, и ждать когда данные будут готовы)
    01000   бит 9: признак "запись маркера" "1" - запись.
                    после установки признака контроллер ждёт от дисковода установленного бита 7,
                    и по получении посылает в 177132 маркер, потом снова ждём готовности и снова пишем и т.д.
    02000   бит 10: включение схемы предкоррекции

177132
Регистр данных НГМД. Доступен по чтению и записи.

    Физически м/с имеет два регистра данных - РДЗ и РДЧ.

    В режиме 'запись' 7-й разряд РС (TR) устанавливается в
единицу после того, как младший байт РДЗ переписался в сдвиговый
регистр.

    В режиме 'чтение' 7-й разряд РС (TR) устанавливается в
единицу после того, как в РДЧ сформировалось очередное считанное
из накопителя слово.

    Сброс TR происходит при обращении к РД по записи или по
чтению, а также по сигналу INIT.

    При тактовой частоте 4 МГц, подаваемой на вход CLC слово
преобразуется за 64 мкс. С этим периодом по признаку TR
центральная ЭВМ должна прочитать данные из РД или записать их
туда. Иначе операции запись/чтение производятся некорректно.

    В режиме 'запись' 14-й разряд РС (CRC) служит для индикации
ситуации, когда текущее требование не было обслужено, а возникли
условия на формирование нового требования. Она является сбойной,
если возникает в середине цикла записи на диск. При этом м/с
прекращает формирование циклического кода и записывает
полученный контрольный код на диск.

    В режиме 'чтение' 14-й разряд РС (CRC) служит признаком
некорректности выполненного чтения.

    При применении м/с следует учесть, что м/с формирует
пропуск синхроимпульсов информации при наличии '1' в 9-ом
разряде РС (WM).

    М/с опознает маркер, и, тем самым, запускается на чтение
информации с диска, если обнаружен пропуск синхроимпульсов в
коде А1 (10100001).

    При записи '1' в 8-й разряд РС (Gdr) м/с частично
приводится в исходное состояние, а по ее снятию готова
осуществлять поиск маркера. Этим разрядом рекомендуется
пользоваться для принудительной синхронизации схемы
синхронизации со считываемыми данными, причём желательно
записать '1', а затем '0' в этот разряд в области зоны нулей
(домаркерной зоны).

    При частоте 4 МГц, подаваемой на вход CLC, м/с обеспечивает
временные характеристики накопителя типа 'Электроника 6022'.

*/
// для AltPro
constexpr auto BIT10 = 02000;
constexpr auto BIT03 = 00010;
constexpr auto BIT02 = 00004;
constexpr auto BIT00 = 00001;

// Маска флагов, сохраняемых в m_flags
constexpr auto FLOPPY_CMD_MASKSTORED = (FLOPPY_CMD_SELECT_DRIVE_A | FLOPPY_CMD_SELECT_DRIVE_B | FLOPPY_CMD_SELECT_DRIVE_C | FLOPPY_CMD_SELECT_DRIVE_D | \
                                        FLOPPY_CMD_ENGINESTART | FLOPPY_CMD_SIDEUP | FLOPPY_CMD_DIR | FLOPPY_CMD_STEP | FLOPPY_CMD_READDATA | \
                                        FLOPPY_CMD_WRITEMARKER | FLOPPY_CMD_PRECORRECTION);

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CFDDController::CFDDController()
	: m_FDDModel(BK_DEV_MPI::STD_FDD)
	, m_bA16M_Trigger(false)
	, m_nAltproMode(0)
	, m_nAltproMemBank(0)
	, m_pDrive(nullptr)
	, m_drive(FDD_DRIVE::NONE)
	, m_bFloppyIsInited(false)
{
	ini_crc_16_rd();
	InitVariables();
#ifdef _DEBUG
	m_msec = GetTickCount();
	m_nFrame = 0;
#endif
}

CFDDController::~CFDDController()
{
	DetachDrives();
}

void CFDDController::OnReset()
{
	for (auto &flp : m_pFloppy)
	{
		if (flp)
		{
			flp->FlushChanges();
		}
	}

	m_pDrive = nullptr;
	m_drive = FDD_DRIVE::NONE;
	InitVariables();
}

void CFDDController::InitVariables()
{
	m_side = m_track = 0;
	m_datareg = m_writereg = m_shiftreg = 0;
	m_bSearchSyncTrigger = false;
	m_bWriteMode = m_writemarker = m_shiftmarker = m_bCRCCalc = false;
	m_nCRCAcc = 0xffff;
	m_bSearchSync = true;
	m_writeflag = m_shiftflag = false;
	m_status = 0;
	m_flags = FLOPPY_CMD_SIDEUP | FLOPPY_CMD_DIR | FLOPPY_CMD_WRITEMARKER;
}

void CFDDController::SetFDDType(BK_DEV_MPI model)
{
	m_FDDModel = model;

	if (!m_bFloppyIsInited)
	{
		// теперь разберёмся с кнопками дисководов.
		switch (m_FDDModel)
		{
			case BK_DEV_MPI::STD_FDD:
			case BK_DEV_MPI::SAMARA:
				m_pFloppy[static_cast<int>(FDD_DRIVE::A)] = std::make_unique<CFloppyDrive>();
				m_pFloppy[static_cast<int>(FDD_DRIVE::B)] = std::make_unique<CFloppyDrive>();
				m_pFloppy[static_cast<int>(FDD_DRIVE::C)] = std::make_unique<CFloppyDrive>();
				m_pFloppy[static_cast<int>(FDD_DRIVE::D)] = std::make_unique<CFloppyDrive>();
				break;

			case BK_DEV_MPI::A16M:
			case BK_DEV_MPI::SMK512:
				m_pFloppy[static_cast<int>(FDD_DRIVE::A)] = std::make_unique<CFloppyDrive>();
				m_pFloppy[static_cast<int>(FDD_DRIVE::B)] = std::make_unique<CFloppyDrive>();
				m_pFloppy[static_cast<int>(FDD_DRIVE::C)].reset();
				m_pFloppy[static_cast<int>(FDD_DRIVE::D)].reset();
				break;

			default:
				m_pFloppy[static_cast<int>(FDD_DRIVE::A)].reset();
				m_pFloppy[static_cast<int>(FDD_DRIVE::B)].reset();
				m_pFloppy[static_cast<int>(FDD_DRIVE::C)].reset();
				m_pFloppy[static_cast<int>(FDD_DRIVE::D)].reset();
				break;
		}

		m_bFloppyIsInited = true;
	}
}


BK_DEV_MPI CFDDController::GetFDDType()
{
	return m_FDDModel;
}

void CFDDController::GetByte(const uint16_t addr, uint8_t *pValue)
{
	uint16_t w;
	GetWord(addr, &w);

	if (addr & 1)
	{
		*pValue = HIBYTE(w);
	}
	else
	{
		*pValue = LOBYTE(w);
	}
}


void CFDDController::GetWord(const uint16_t addr, uint16_t *pValue)
{
	switch (addr & 0177776)
	{
		case 0177130:
			*pValue = GetState();
			// TRACE(_T("Rd 177130: %06o\n"), *pValue);
			break;

		case 0177132:
			*pValue = GetData();
			// TRACE(_T("fdr - pos:%06d  %06o\n"), m_pFloppy[static_cast<int>(m_drive)]->m_nDataPtr, *pValue);
			break;
	}
}


void CFDDController::SetByte(const uint16_t addr, uint8_t value)
{
	if (addr & 1)
	{
		SetWord(addr, ((uint16_t)value) << 8);
	}
	else
	{
		SetWord(addr, ((uint16_t)value));
	}
}


void CFDDController::SetWord(const uint16_t addr, uint16_t value)
{
	switch (addr & 0177776)
	{
		case 0177130:
			SetCommand(value);
			break;

		case 0177132:
			WriteData(value);
			break;
	}
}


void CFDDController::AttachDrives()
{
	for (FDD_DRIVE d = FDD_DRIVE::A; d <= FDD_DRIVE::D; ((int &)d)++)
	{
		fs::path strImgName = g_Config.GetDriveImgName(d);

		if (!Global::isEmptyUnit(strImgName))
		{
			// если ничего не было подключено, то подключаем
			AttachImage(d, strImgName);
		}
		else
		{
			// если что-то было подключено - отключаем
			DetachImage(d);
		}
	}

	// и винчестеров
	if (m_FDDModel == BK_DEV_MPI::SMK512 || m_FDDModel == BK_DEV_MPI::SAMARA)
	{
		fs::path strHDDImgName = g_Config.GetDriveImgName(HDD_MODE::MASTER);

		// если в канал Master что-то нужно примонтировать
		if (!Global::isEmptyUnit(strHDDImgName))
		{
			m_ATA_IDE.attach(strHDDImgName, HDD_MODE::MASTER);
		}
		else
		{
			m_ATA_IDE.detach(HDD_MODE::MASTER);
		}

		strHDDImgName = g_Config.GetDriveImgName(HDD_MODE::SLAVE);

		// если в канал Slave что-то нужно примонтировать
		if (!Global::isEmptyUnit(strHDDImgName))
		{
			m_ATA_IDE.attach(strHDDImgName, HDD_MODE::SLAVE);
		}
		else
		{
			m_ATA_IDE.detach(HDD_MODE::SLAVE);
		}

		m_ATA_IDE.reset();
	}
}

void CFDDController::DetachDrives()
{
	for (FDD_DRIVE d = FDD_DRIVE::A; d <= FDD_DRIVE::D; ((int &)d)++)
	{
		DetachImage(d);
	}

	// и винчестеров
	if (m_FDDModel == BK_DEV_MPI::SMK512 || m_FDDModel == BK_DEV_MPI::SAMARA)
	{
		m_ATA_IDE.detach(HDD_MODE::MASTER);
		m_ATA_IDE.detach(HDD_MODE::SLAVE);
	}
}


void CFDDController::EmulateFDD(CMotherBoard *pBoard)
{
	TABLE_EMFDD dt{};
	uint16_t table_addr = pBoard->GetRON(CCPU::REGISTER::R3);
	// заполняем блок параметров драйвера дисковода
	auto wdt = reinterpret_cast<uint16_t *>(&dt); // структура в виде массива слов.
	uint16_t t = table_addr;

	for (size_t i = 0; i < sizeof(dt) / sizeof(uint16_t); ++i)
	{
		wdt[i] = pBoard->GetWordIndirect(t);
		t += sizeof(uint16_t);
	}

	int drive = dt.UNIT;

	// Если номер привода выходит за диапазон A: - D:
	if (drive > 3)
	{
		pBoard->SetPSWBit(PSW_BIT::C, true);
		pBoard->SetWordIndirect(052, FDD_0_TRACK_ERROR);
		return;
	}

	int nSides = 2;

	if (dt.FLGTAB[drive] & 2) // определим, сколько сторон у диска
	{
		nSides = 1;
	}

	// специально для 253й прошивки
	if (0167 != pBoard->GetWordIndirect(0160016))
	{
		dt.MAXSEC = 10;
	}

	// Если длина чтения/записи равна 0 - просто выходим без ошибок
	if (!dt.WCNT)
	{
		pBoard->SetPSWBit(PSW_BIT::C, false);
		pBoard->SetWordIndirect(052, FDD_NOERROR);
		return;
	}

	CFloppyDrive *pFD = m_pFloppy[drive].get();
	// Проверим, примонтирован ли образ в заданный привод
	bool bRes = pFD && pFD->isAttached();

	// Если файл не примонтирован, выходим с ошибкой "Нет Диска"
	if (!bRes)
	{
		pBoard->SetPSWBit(PSW_BIT::C, true);
		pBoard->SetWordIndirect(052, FDD_NO_DISK);
	}
	else
	{
		bool bErrors = false;

		if ((dt.SECTOR == 0 || dt.SECTOR > dt.MAXSEC) || (dt.SIDE >= 2) || (dt.TRK >= 82))
		{
			bErrors = true;
			pBoard->SetPSWBit(PSW_BIT::C, true);
			pBoard->SetWordIndirect(052, FDD_BAD_FORMAT);
		}
		else
		{
			// зададим значение копии регистра по записи как в оригинале
			pBoard->SetWordIndirect(table_addr, 020 | (1 << drive));
			// высчитаем позицию.
			LONGLONG pos = (((dt.TRK * static_cast<LONGLONG>(nSides)) + dt.SIDE) * dt.MAXSEC + (static_cast<LONGLONG>(dt.SECTOR) - 1)) * 512;

			if (pos == pFD->FDSeek(pos, 0))
			{
				uint16_t addr = dt.ADDR;
				int length = dt.WCNT;

				if (length > 0)
				{
					uint16_t word;

					// чтение
					do
					{
						UINT count = pFD->FDRead(&word, sizeof(uint16_t));

						if (count != sizeof(uint16_t))
						{
							bErrors = true;
							pBoard->SetPSWBit(PSW_BIT::C, true);
							pBoard->SetWordIndirect(052, FDD_STOP);
							break;
						}

						pBoard->SetWord(addr, word);
						addr += sizeof(uint16_t);
					}
					while (--length);
				}
				else
				{
					// Если образ примонтирован, и операция записи, а образ защищён от записи
					if (pFD->isReadOnly())
					{
						// то сообщим об этом ошибкой
						pBoard->SetPSWBit(PSW_BIT::C, true);
						pBoard->SetWordIndirect(052, FDD_DISK_PROTECTED);
						return;
					}

					// запись
					length = -length;

					do
					{
						uint16_t word = pBoard->GetWord(addr);

						try
						{
							pFD->FDWrite(&word, sizeof(uint16_t));
						}
						catch (...) // только так можно отловить ошибку записи
						{
							bErrors = true;
							pBoard->SetPSWBit(PSW_BIT::C, true);
							pBoard->SetWordIndirect(052, FDD_STOP);
							break;
						}

						addr += sizeof(uint16_t);
					}
					while (--length);
				}
			}
			else
			{
				bErrors = true;
				pBoard->SetPSWBit(PSW_BIT::C, true);
				pBoard->SetWordIndirect(052, FDD_BAD_FORMAT);
			}
		}

		if (!bErrors)
		{
			pBoard->SetPSWBit(PSW_BIT::C, false);
			pBoard->SetWordIndirect(052, FDD_NOERROR);
		}
	}
}


bool CFDDController::IsAttached(FDD_DRIVE eDrive)
{
	const int nDrive = static_cast<int>(eDrive) & 3;
	return m_pFloppy[nDrive] && m_pFloppy[nDrive]->isAttached();
}

bool CFDDController::IsReadOnly(FDD_DRIVE eDrive)
{
	const int nDrive = static_cast<int>(eDrive) & 3;
	return m_pFloppy[nDrive] && m_pFloppy[nDrive]->isReadOnly();
}

bool CFDDController::GetDriveState(FDD_DRIVE eDrive)
{
	const int nDrive = static_cast<int>(eDrive) & 3;
	return !!m_pFloppy[nDrive]; // true - привод есть, false - привода нет. для разных типов контроллеров
}

bool CFDDController::AttachImage(FDD_DRIVE eDrive, const fs::path &strFileName)
{
	const int nDrive = static_cast<int>(eDrive) & 3;
	CFloppyDrive *pFD = m_pFloppy[nDrive].get();

	if (pFD)
	{
		// Открываем файл
		if (pFD->Attach(strFileName, g_Config.m_bExclusiveOpenImages))
		{
			InitVariables();
			m_status = FLOPPY_STATUS_TRACK0;

			if (pFD->isReadOnly())
			{
				m_status |= FLOPPY_STATUS_WRITEPROTECT;
			}

			pFD->PrepareTrack(m_track, m_side);
		}

		return true;
	}

	return false;
}

void CFDDController::DetachImage(FDD_DRIVE eDrive)
{
	const int nDrive = static_cast<int>(eDrive) & 3;

	if (m_pFloppy[nDrive])
	{
		m_pFloppy[nDrive]->Detach();
	}
}

// проверка и отлов кода режима
// выход:
// true - в mmodl->nAltProMode помещён достоверный код режима.
// false - всё остальное.
bool CFDDController::Change_AltPro_Mode(ConfBKModel_t *mmodl, const uint16_t w)
{
	switch (m_FDDModel)
	{
		case BK_DEV_MPI::SMK512:
			// отловим nExtCodes
			mmodl->nExtCodes = w & 014; // в СМК512 код 10 ни на что влиять не должен.

			// вот тут проявляется отличительная особенность реплики СМК-512
			// в ней строб - число 0b0110, а в оригинальных СМК и А16М строб - число 0bx11x
			if (!m_bA16M_Trigger && ((w & 0xf) == ALTPRO_BEGIN_STROB_TRIGGER)) // если строб-код
			{
				m_bA16M_Trigger = true; // взводим триггер, означающий, что любое следующее значение будет кодом режима
				return false; // обязательно ретурн, т.к. следующей записью должны принимать код.
			}

			// если никакой не строб, то проверяем
			if (m_bA16M_Trigger) // если триггер взведён
			{
				m_nAltproMode = w & ALTPRO_MODE_MASK;       // вычленим режим
				m_nAltproMemBank = w & ALTPRO_CODE_MASK;    // вычленим код страницы памяти на СМК.

				if ((w & 0xf) != ALTPRO_BEGIN_STROB_TRIGGER)
				{
					m_bA16M_Trigger = false;    // сбрасываем триггер
					mmodl->nAltProMode = m_nAltproMode;
					mmodl->nAltProMemBank = m_nAltproMemBank;
					return true;    // просигналим, что надо запустить MemoryManager() для переключения банков
				}
			}

			break;

		case BK_DEV_MPI::A16M:
			// отловим nExtCodes
			mmodl->nExtCodes = w & 014;

			if (!m_bA16M_Trigger && ((w & ALTPRO_BEGIN_STROB_TRIGGER) == ALTPRO_BEGIN_STROB_TRIGGER)) // если строб-код
			{
				m_bA16M_Trigger = true; // взводим триггер, означающий, что любое следующее значение будет кодом режима
				return false; // обязательно ретурн, т.к. следующей записью должны принимать код.
			}

			// если никакой не строб, то проверяем
			if (m_bA16M_Trigger) // если триггер взведён
			{
				m_nAltproMode = w & ALTPRO_MODE_MASK;       // вычленим режим
				m_nAltproMemBank = w & ALTPRO_CODE_MASK;    // вычленим код страницы памяти на СМК.

				if ((w & ALTPRO_BEGIN_STROB_TRIGGER) != ALTPRO_BEGIN_STROB_TRIGGER)
				{
					m_bA16M_Trigger = false;    // сбрасываем триггер
					mmodl->nAltProMode = m_nAltproMode;
					mmodl->nAltProMemBank = m_nAltproMemBank;
					return true;    // просигналим, что надо запустить MemoryManager() для переключения банков
				}
			}

			break;
	}

	return false;
}

void CFDDController::init_A16M_10(ConfBKModel_t *mmodl, const int v)
{
	mmodl->nAltProMemBank = 0; // коды подключения страниц
	mmodl->nExtCodes = 0; // доп коды.
	mmodl->nROMPresent = 0;
	mmodl->nAltProMode = v; // режим работы
}

void CFDDController::init_A16M_11M(ConfBKModel_t *mmodl, const int v)
{
	mmodl->nAltProMemBank = 0; // коды подключения страниц
	mmodl->nExtCodes = 0; // доп коды.
	mmodl->nROMPresent = 0;
	mmodl->nAltProMode = (m_FDDModel == BK_DEV_MPI::STD_FDD) ? 0 : v;   // режим работы
}

void CFDDController::A16M_MemManager_10(BKMEMBank_t *mmap, ConfBKModel_t *mmodl)
{
	// устанавливаем новый режим
	switch (mmodl->nAltProMode & ALTPRO_MODE_MASK)
	{
		case ALTPRO_A16M_START_MODE:
		{
			// Start (БК11М: откл.окно1)
			// банк 8,9 - просто не трогаем
			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_10);
			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_10);
			// банк 7 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// причём по 170000 то же самое пзу
			A16M_SetRomBank(mmap, 017, A16M_ROM_10);
			break;
		}

		case ALTPRO_A16M_STD10_MODE:
		{
			// Std10 (БК11М: откл.окно1, откл.мон11)
			// банк 8,9 - просто не трогаем
			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_10);
			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_10);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZU10_MODE:
		{
			// ОЗУ10 (БК11М: откл.окно1; БК10: откл.мон10)
			// банк 8,9 - банк А0,А1
			A16M_SetMemBank(mmap, 010, A16M_A0_10);
			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_10);

			// банк 12,13 - отключаем за ненадобностью
			for (int i = 014; i <= 015; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_BASIC_MODE:
		{
			// Basic (БК11М: откл.окно1, откл.мон11; БК10: откл.мон10)
			// банк 8,9 - банк А0,А1
			A16M_SetMemBank(mmap, 010, A16M_A0_10);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;

			// эти банки просто отключаем.
			for (int i = 012; i <= 017; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			break;
		}

		case ALTPRO_A16M_STD11_MODE:
		{
			// Std11
			// банки 8..9 просто не трогаем
			// банки 10..13 отключаем, даже если включено пзу бейсика, ибо нафиг оно неполное нужно
			for (int i = 012; i <= 015; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZU11_MODE:
		{
			// ОЗУ11 (БК11М: откл.мон11)
			// банк 8,9 - просто не трогаем
			// банк 10,11 - отключаем
			for (int i = 012; i <= 013; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_10);
			// банк 14 - банк А2
			A16M_SetMemBank(mmap, 016, A16M_A2_10);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZUZZ_MODE:
		{
			// ОЗУзз (БК10: откл.мон10)
			// банк 8,9 - банк А0,А1 с защитой от записи
			A16M_SetMemBank(mmap, 010, A16M_A0_10);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;
			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_10);

			// банк 12,13 - отключаем за ненадобностью
			for (int i = 014; i <= 015; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_HLT11_MODE:
		{
			// Hlt11 (БК11М: откл.мон11; БК10: откл.мон10)
			// банк 8,9, 10,11 - отключаем
			for (int i = 010; i <= 013; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_10);
			// банк 14,15 - А2,А3
			A16M_SetMemBank(mmap, 016, A16M_A2_10);
			// причём банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		default:
			ASSERT(false);
	}
}

inline void CFDDController::A16M_SetMemBank(BKMEMBank_t *mmap, int nBnk, int BnkNum)
{
	A16M_SetMemSegment(mmap, nBnk++, BnkNum++);
	A16M_SetMemSegment(mmap, nBnk,   BnkNum);
}

inline void CFDDController::A16M_SetMemSegment(BKMEMBank_t *mmap, int nBnk, int BnkNum)
{
	BKMEMBank_t *d = mmap + nBnk;
	d->bWritable = d->bReadable = true;
	d->nBank = BnkNum;
	d->nOffset = BnkNum << 12;
	d->nTimingCorrection = RAM_TIMING_CORR_VALUE_SMK;
}

inline void CFDDController::A16M_SetRomBank(BKMEMBank_t *mmap, int nBnk, int BnkNum)
{
	BKMEMBank_t *d = mmap + nBnk;
	d->bReadable = true;
	d->bWritable = false;
	d->nBank = BnkNum;
	d->nOffset = BnkNum << 12;
	d->nTimingCorrection = ROM_TIMING_CORR_VALUE_SMK;
}


void CFDDController::A16M_MemManager_11M(BKMEMBank_t *mmap, ConfBKModel_t *mmodl)
{
	if (m_FDDModel != BK_DEV_MPI::A16M)
	{
		return;
	}

	// устанавливаем новый режим
	switch (mmodl->nAltProMode & ALTPRO_MODE_MASK)
	{
		case ALTPRO_A16M_START_MODE:
		{
			// Start (БК11М: откл.окно1)
			// банк 8,9 - пустой на БК11М, а на БК10 - там остаётся монитор
			for (int i = 010; i <= 011; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 10,11 - банк А2,A3
			A16M_SetMemBank(mmap, 012, A16M_A2_11M);
			// банк 12,13 - банк А0,A1
			A16M_SetMemBank(mmap, 014, A16M_A0_11M);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15 - то же самое пзу
			A16M_SetRomBank(mmap, 017, A16M_ROM_11M);
			break;
		}

		case ALTPRO_A16M_STD10_MODE:
		{
			// Std10 (БК11М: откл.окно1, откл.мон11)
			// банк 8,9 - пустой на БК11М, а на БК10 - там остаётся монитор
			for (int i = 010; i <= 011; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_11M);
			// банк 12,13 - банк А0,A1
			A16M_SetMemBank(mmap, 014, A16M_A0_11M);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZU10_MODE:
		{
			// ОЗУ10 (БК11М: откл.окно1; БК10: откл.мон10)
			// банк 8,9 - банк А0,A1
			A16M_SetMemBank(mmap, 010, A16M_A0_11M);
			// банк 10,11 - банк А2,A3
			A16M_SetMemBank(mmap, 012, A16M_A2_11M);
			// банк 12,13 - монитор 11М
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_BASIC_MODE:
		{
			// Basic (БК11М: откл.окно1, откл.мон11; БК10: откл.мон10)
			// банк 8,9 - банк А0,A1
			A16M_SetMemBank(mmap, 010, A16M_A0_11M);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;

			// банки 10..15 - пустые на БК11М
			for (int i = 012; i <= 017; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			break;
		}

		case ALTPRO_A16M_STD11_MODE:
		{
			// Std11
			// банки 8..13. просто не трогаем
			// банк 14. - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15. - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZU11_MODE:
		{
			// ОЗУ11 (БК11М: откл.мон11)
			// банки 8..11 просто не трогаем
			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_11M);
			// банк 14 - банк А2
			A16M_SetMemBank(mmap, 016, A16M_A2_11M);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_OZUZZ_MODE:
		{
			// ОЗУзз (БК10: откл.мон10)
			// банк 8,9 - банк А0,A1 с защитой от записи
			A16M_SetMemBank(mmap, 010, A16M_A0_11M);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;
			// банк 10,11 - банк А2,А3
			A16M_SetMemBank(mmap, 012, A16M_A2_11M);
			// банк 12,13 - монитор 11М
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_A16M_HLT11_MODE:
		{
			// Hlt11 (БК11М: откл.мон11; БК10: откл.мон10)
			// банки 8..11 просто не трогаем
			// банк 12,13 - банк А0,А1
			A16M_SetMemBank(mmap, 014, A16M_A0_11M);
			// банк 14,15 - банк А2,A3
			A16M_SetMemBank(mmap, 016, A16M_A2_11M);
			// причём банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		default:
			ASSERT(false);
	}
}

void CFDDController::SMK512_MemManager_10(BKMEMBank_t *mmap, ConfBKModel_t *mmodl)
{
	int BaseSegment = A16M_A0_10;
	int code = mmodl->nAltProMemBank & ALTPRO_CODE_MASK;

	if (code & BIT10)
	{
		BaseSegment += 010;
	}

	if (code & BIT02)
	{
		BaseSegment += 020;
	}

	if (code & BIT03)
	{
		BaseSegment += 040;
	}

	if (code & BIT00)
	{
		BaseSegment += 0100;
	}

	// устанавливаем новый режим
	switch (mmodl->nAltProMode & ALTPRO_MODE_MASK)
	{
		case ALTPRO_SMK_SYS_MODE:
		{
			// Sys (БК11М: откл.окно1)
			// банк 8,9 - там остаётся монитор
			// банк 10,11 - сегменты 6,7
			A16M_SetMemBank(mmap, 012, BaseSegment + 6);
			// банк 12,13 - сегменты 0,1
			A16M_SetMemBank(mmap, 014, BaseSegment);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// банк 15 - то же самое пзу
			A16M_SetRomBank(mmap, 017, A16M_ROM_10);
			break;
		}

		case ALTPRO_SMK_STD10_MODE:
		{
			// Std10 (БК11М: откл.окно1, откл.мон11)
			// банк 8,9 - там остаётся монитор
			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// для HDD банк 15 - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			A16M_SetMemSegment(mmap, 017, BaseSegment + 7);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_OZU10_MODE:
		{
			// ОЗУ10 (БК11М: откл.окно1; БК10: откл.мон10)
			// банк 8,9 - сегменты 0,1
			A16M_SetMemBank(mmap, 010, BaseSegment);
			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5 + наложенный монитор 11М
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_ALL_MODE:
		{
			// All (БК11М: откл.окно1, откл.мон11; БК10: откл.мон10)
			A16M_SetMemBank(mmap, 010, BaseSegment + 4);
			A16M_SetMemBank(mmap, 012, BaseSegment + 6);
			A16M_SetMemBank(mmap, 014, BaseSegment);
			A16M_SetMemBank(mmap, 016, BaseSegment + 2);
			// причём банк 15 доступен только по чтению
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_STD11_MODE:
		{
			// Std11
			// банки 8..9 просто не трогаем
			// банки 10..13 отключаем
			for (int i = 012; i <= 015; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 14. - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_10);
			// для HDD банк 15. - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			A16M_SetMemSegment(mmap, 017, BaseSegment + 7);
			// банк 15. - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_OZU11_MODE:
		{
			// ОЗУ11 (БК11М: откл.мон11)
			// банки 8,9 просто не трогаем
			// банк 10,11 - отключаем
			for (int i = 012; i <= 013; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14 - сегмент 6
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_HLT10_MODE:
		{
			// Hlt10 (БК10: откл.мон10)
			// банк 8,9 - сегменты 0,1 с защитой от записи
			A16M_SetMemBank(mmap, 010, BaseSegment);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;
			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5 + наложенный монитор 11М
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// причём банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		case ALTPRO_SMK_HLT11_MODE:
		{
			// Hlt11 (БК11М: откл.мон11; БК10: откл.мон10)
			// банки 8,9 просто не трогаем
			// банк 10,11 - отключаем
			for (int i = 012; i <= 013; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// причём банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		default:
			ASSERT(false);
	}
}

/*// эмуляция неисправного контроллера. при заданном коде просто не будет подключаться никакой памяти
bool b_MakeCorrupt = false;
inline void A16M_SetMemBank_Corrupt(BKMEMBank_Type* mmap, int nBnk, int BnkNum)
{
    for (int i = nBnk; i <= nBnk + 1; ++i)
    {
        if (b_MakeCorrupt)
        {
            mmap[i].bReadable = false;
            mmap[i].bWritable = false;
        }
        else
        {
            mmap[i].bReadable = true;
            mmap[i].bWritable = true;
        }

        mmap[i].nScreenBuffer = 0;
        mmap[i].nPage = BnkNum >> 2;
        mmap[i].nOffset = BnkNum << 12;
        mmap[i].nBank = BnkNum++;
    }
}
*/
void CFDDController::SMK512_MemManager_11M(BKMEMBank_t *mmap, ConfBKModel_t *mmodl)
{
	if (m_FDDModel != BK_DEV_MPI::SMK512)
	{
		return;
	}

	int BaseSegment = A16M_A0_11M;
	int code = mmodl->nAltProMemBank & ALTPRO_CODE_MASK;

	if (code & BIT10)
	{
		BaseSegment += 010;
	}

	if (code & BIT02)
	{
		BaseSegment += 020;
	}

	if (code & BIT03)
	{
		BaseSegment += 040;
	}

	if (code & BIT00)
	{
		BaseSegment += 0100;
	}

	// устанавливаем новый режим
	switch (mmodl->nAltProMode & ALTPRO_MODE_MASK)
	{
		case ALTPRO_SMK_SYS_MODE:
		{
			// Sys (БК11М: откл.окно1)
			// банк 8,9 - пустой на БК11М, а на БК10 - там остаётся монитор
			for (int i = 010; i <= 011; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 10,11 - сегменты 6,7
			A16M_SetMemBank(mmap, 012, BaseSegment + 6);
			// банк 12,13 - сегменты 0,1
			A16M_SetMemBank(mmap, 014, BaseSegment);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// банк 15 - то же самое пзу
			A16M_SetRomBank(mmap, 017, A16M_ROM_11M);
			break;
		}

		case ALTPRO_SMK_STD10_MODE:
		{
			// Std10 (БК11М: откл.окно1, откл.мон11)
			// банк 8,9 - пустой на БК11М, а на БК10 - там остаётся монитор
			for (int i = 010; i <= 011; ++i)
			{
				mmap[i].bReadable = false;
				mmap[i].bWritable = false;
			}

			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14 - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// для HDD банк 15 - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			A16M_SetMemSegment(mmap, 017, BaseSegment + 7);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_OZU10_MODE:
		{
			// ОЗУ10 (БК11М: откл.окно1; БК10: откл.мон10)
			// банк 8,9 - сегменты 0,1
			A16M_SetMemBank(mmap, 010, BaseSegment);
			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5 + наложенный монитор 11М
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// для HDD банк 15 - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			// банк 15 - Только запись - старая схема
			// mmap[017].bReadable = false;
			// mmap[017].bWritable = true;
			// банк 15 - отключено - новая схема
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_ALL_MODE:
		{
			// All (БК11М: откл.окно1, откл.мон11; БК10: откл.мон10)
			// банк 8,9 - сегменты 4,5
			A16M_SetMemBank(mmap, 010, BaseSegment + 4);
			// банк 10,11 - сегменты 6,7
			A16M_SetMemBank(mmap, 012, BaseSegment + 6);
			// банк 12,13 - сегменты 0,1
			A16M_SetMemBank(mmap, 014, BaseSegment);
			// банк 14,15 - сегменты 2,3
			A16M_SetMemBank(mmap, 016, BaseSegment + 2);
			// для HDD банк 15 - сегмент 3, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			// банк 15 доступен только по чтению
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_STD11_MODE:
		{
			// Std11
			// банки 8..13. просто не трогаем
			// банк 14. - обычное пзу контроллера
			A16M_SetRomBank(mmap, 016, A16M_ROM_11M);
			// для HDD банк 15 - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			A16M_SetMemSegment(mmap, 017, BaseSegment + 7);
			// банк 15. - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_OZU11_MODE:
		{
			// ОЗУ11 (БК11М: откл.мон11)
			// банки 8..11 просто не трогаем
			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14 - сегмент 6
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// для HDD банк 15 - сегмент 7, так надо. Сперва назначаем ОЗУ, потом выставляем то, что положено по режиму
			A16M_SetMemSegment(mmap, 017, BaseSegment + 7);
			// банк 15 - отключено
			mmap[017].bReadable = false;
			mmap[017].bWritable = false;
			break;
		}

		case ALTPRO_SMK_HLT10_MODE:
		{
			// Hlt10 (БК10: откл.мон10)
			// банк 8,9 - сегменты 0,1 с защитой от записи
			A16M_SetMemBank(mmap, 010, BaseSegment);
			// причём банк 8 доступен только по чтению
			mmap[010].bWritable = false;
			// банк 10,11 - сегменты 2,3
			A16M_SetMemBank(mmap, 012, BaseSegment + 2);
			// банк 12,13 - сегменты 4,5 + наложенный монитор 11М
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// банк 14 - только чтение - старая схема
			// mmap[016].bWritable = false;
			// банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		case ALTPRO_SMK_HLT11_MODE:
		{
			// Hlt11 (БК11М: откл.мон11; БК10: откл.мон10)
			// банки 8..11 просто не трогаем
			// банк 12,13 - сегменты 4,5
			A16M_SetMemBank(mmap, 014, BaseSegment + 4);
			// банк 14,15 - сегменты 6,7
			A16M_SetMemBank(mmap, 016, BaseSegment + 6);
			// банк 15 доступен только по записи
			mmap[017].bReadable = false;
			break;
		}

		default:
			ASSERT(false);
	}
}


bool CFDDController::WriteHDDRegisters(uint16_t num, uint16_t data)
{
	uint16_t addr = num & 0177776;

	// если у нас SMK512, то у него есть регистры HDD
	switch (GetFDDType())
	{
		case BK_DEV_MPI::SMK512:
		{
			// обработка регистров HDD SMK
			switch (addr)
			{
				case 0177740:
					m_ATA_IDE.write_regs((num == 0177741) ? HDD_REGISTER::H3F7 : HDD_REGISTER::H1F7, data);
					break;

				case 0177742:
					m_ATA_IDE.write_regs((num == 0177743) ? HDD_REGISTER::H3F6 : HDD_REGISTER::H1F6, data);
					break;

				case 0177744:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F5, data);
					break;

				case 0177746:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F4, data);
					break;

				case 0177750:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F3, data);
					break;

				case 0177752:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F2, data);
					break;

				case 0177754:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F1, data);
					break;

				case 0177756:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F0, data);
					break;

				default:
					return false;
			}

			return true;
		}

		case BK_DEV_MPI::SAMARA:
		{
			// обработка регистров HDD SMK
			switch (addr)
			{
				case 0177620:
					m_ATA_IDE.write_regs(HDD_REGISTER::H3F7, data);
					break;

				case 0177622:
					m_ATA_IDE.write_regs(HDD_REGISTER::H3F6, data);
					break;

				case 0177640:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F7, data);
					break;

				case 0177642:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F6, data);
					break;

				case 0177644:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F5, data);
					break;

				case 0177646:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F4, data);
					break;

				case 0177650:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F3, data);
					break;

				case 0177652:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F2, data);
					break;

				case 0177654:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F1, data);
					break;

				case 0177656:
					m_ATA_IDE.write_regs(HDD_REGISTER::H1F0, data);
					break;

				default:
					return false;
			}

			return true;
		}
	}

	return false;
}

bool CFDDController::ReadHDDRegisters(uint16_t num, uint16_t &data)
{
	uint16_t addr = num & 0177776;

	// если у нас SMK512, то у него есть регистры HDD
	switch (GetFDDType())
	{
		case BK_DEV_MPI::SMK512:
		{
			uint16_t result = 0;

			// обработка регистров HDD SMK
			// оказывается, есть ещё и байтовые обращения
			switch (addr)
			{
				case 0177740:
					result = m_ATA_IDE.read_regs((num == 0177741) ? HDD_REGISTER::H3F7 : HDD_REGISTER::H1F7);
					break;

				case 0177742:
					result = m_ATA_IDE.read_regs((num == 0177743) ? HDD_REGISTER::H3F6 : HDD_REGISTER::H1F6);
					break;

				case 0177744:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F5);
					break;

				case 0177746:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F4);
					break;

				case 0177750:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F3);
					break;

				case 0177752:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F2);
					break;

				case 0177754:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F1);
					break;

				case 0177756:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F0);
					break;

				default:
					return false;
			}

			data = ~result;
			return true;
		}

		case BK_DEV_MPI::SAMARA:
		{
			uint16_t result = 0;

			// обработка регистров HDD Samara
			switch (addr)
			{
				case 0177620:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H3F7);
					break;

				case 0177622:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H3F6);
					break;

				case 0177640:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F7);
					break;

				case 0177642:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F6);
					break;

				case 0177644:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F5);
					break;

				case 0177646:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F4);
					break;

				case 0177650:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F3);
					break;

				case 0177652:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F2);
					break;

				case 0177654:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F1);
					break;

				case 0177656:
					result = m_ATA_IDE.read_regs(HDD_REGISTER::H1F0);
					break;

				default:
					return false;
			}

			data = ~result;
			return true;
		}
	}

	data = ~0;
	return false;
}

bool CFDDController::ReadHDDRegistersInternal(uint16_t num, uint16_t &data)
{
	uint16_t addr = num & 0177776;

	// если у нас SMK512, то у него есть регистры HDD
	switch (GetFDDType())
	{
		case BK_DEV_MPI::SMK512:
		{
			uint16_t result = 0;

			// обработка регистров HDD SMK
			// оказывается, есть ещё и байтовые обращения
			switch (addr)
			{
				case 0177740:
					result = m_ATA_IDE.read_regs_internal((num == 0177741) ? HDD_REGISTER::H3F7 : HDD_REGISTER::H1F7);
					break;

				case 0177742:
					result = m_ATA_IDE.read_regs_internal((num == 0177743) ? HDD_REGISTER::H3F6 : HDD_REGISTER::H1F6);
					break;

				case 0177744:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F5);
					break;

				case 0177746:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F4);
					break;

				case 0177750:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F3);
					break;

				case 0177752:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F2);
					break;

				case 0177754:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F1);
					break;

				case 0177756:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F0);
					break;

				default:
					return false;
			}

			data = ~result;
			return true;
		}

		case BK_DEV_MPI::SAMARA:
		{
			uint16_t result = 0;

			// обработка регистров HDD Samara
			switch (addr)
			{
				case 0177620:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H3F7);
					break;

				case 0177622:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H3F6);
					break;

				case 0177640:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F7);
					break;

				case 0177642:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F6);
					break;

				case 0177644:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F5);
					break;

				case 0177646:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F4);
					break;

				case 0177650:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F3);
					break;

				case 0177652:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F2);
					break;

				case 0177654:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F1);
					break;

				case 0177656:
					result = m_ATA_IDE.read_regs_internal(HDD_REGISTER::H1F0);
					break;

				default:
					return false;
			}

			data = ~result;
			return true;
		}
	}

	data = ~0;
	return false;
}

uint16_t CFDDController::ReadDebugHDDRegisters(int nDrive, HDD_REGISTER num, bool bReadMode)
{
	return m_ATA_IDE.get_debug_regs(nDrive, num, bReadMode);
}


bool CFDDController::IsEngineOn()
{
	return !!(m_flags & FLOPPY_CMD_ENGINESTART);
}

uint16_t CFDDController::GetState()
{
	if (m_pDrive == nullptr)   // если привод не выбран
	{
		return 0;    // то возвращаем 0
	}

	if (!m_pDrive->isAttached())    // если файл не открыт - аналог того, что в дисководе нет диска
	{
		return FLOPPY_STATUS_INDEXMARK | (m_status & FLOPPY_STATUS_TRACK0);
	}

///	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);

	// далее, формируем возвращаемый статус, для выбранного привода.
	// если двигатель включён, то выставим флаг готовности.
	if (m_flags & FLOPPY_CMD_ENGINESTART)
	{
		m_status |= FLOPPY_STATUS_RDY;
	}
	else
	{
		m_status &= ~(FLOPPY_STATUS_RDY | FLOPPY_STATUS_MOREDATA | FLOPPY_STATUS_CHECKSUMOK);
	}

	return m_status;
}

void CFDDController::SetCommand(uint16_t cmd)
{
	// Копируем новые флаги в m_flags
	m_flags &= ~FLOPPY_CMD_MASKSTORED;
	m_flags |= cmd & FLOPPY_CMD_MASKSTORED;
	bool okPrepareTrack = false;  // Нужно ли считывать дорожку в буфер
	// Проверить, не сменился ли текущий привод
	// тут выбирается текущий привод, чем более младший бит, тем больше приоритет
	// если установлено несколько битов - выбирается тот, что младше.
	FDD_DRIVE newdrive = (cmd & 1) ? FDD_DRIVE::A : (cmd & 2) ? FDD_DRIVE::B : (cmd & 4) ? FDD_DRIVE::C : (cmd & 8) ? FDD_DRIVE::D : FDD_DRIVE::NONE;

	if (m_drive != newdrive)
	{
		// предыдущему приводу, если он был, надо сохранить изменения
		if (m_pDrive)
		{
			m_pDrive->FlushChanges();
		}

		m_drive = newdrive;
		m_pDrive = (newdrive == FDD_DRIVE::NONE) ? nullptr : m_pFloppy[static_cast<int>(m_drive)].get();
		okPrepareTrack = true;
	}

	if (m_drive == FDD_DRIVE::NONE || m_pDrive == nullptr) // если привод не выбран или выбран не существующий
	{
		return; // нечего и делать
	}

	// Проверяем, не сменилась ли сторона
	if (m_flags & FLOPPY_CMD_SIDEUP)  // Выбор стороны: 0 - нижняя, 1 - верхняя
	{
		if (m_side == 0)
		{
			m_side = 1;
			okPrepareTrack = true;
		}
	}
	else
	{
		if (m_side == 1)
		{
			m_side = 0;
			okPrepareTrack = true;
		}
	}

	if (m_flags & FLOPPY_CMD_STEP)  // Двигаем головки в заданном направлении
	{
		if (m_flags & FLOPPY_CMD_DIR)
		{
			if (m_track < 82)
			{
				m_track++;
				okPrepareTrack = true;
			}
		}
		else
		{
			if (m_track >= 1)
			{
				m_track--;
				okPrepareTrack = true;
			}
		}

		if (m_track == 0)
		{
			m_status |= FLOPPY_STATUS_TRACK0;
		}
		else
		{
			m_status &= ~FLOPPY_STATUS_TRACK0;
		}
	}

	if (okPrepareTrack)
	{
		m_pDrive->PrepareTrack(m_track, m_side);
	}

	if (m_flags & FLOPPY_CMD_READDATA)  // Поиск маркера
	{
		m_bSearchSyncTrigger = true;    // если бит FLOPPY_CMD_READDATA == 1, взводим триггер
	}
	else if (m_bSearchSyncTrigger)      // если бит FLOPPY_CMD_READDATA == 0 и триггер взведён
	{
		m_bSearchSyncTrigger = false;   // переходим в режим поиска маркера
		m_bSearchSync = true;
		m_status &= ~(FLOPPY_STATUS_CHECKSUMOK | FLOPPY_STATUS_MOREDATA);
		m_bCrcOK = false;
	}

	if (m_bWriteMode && (m_flags & FLOPPY_CMD_WRITEMARKER))  // Запись маркера
	{
		m_writemarker = true;
		m_status &= ~(FLOPPY_STATUS_CHECKSUMOK | FLOPPY_STATUS_MOREDATA);
		m_bCrcOK = false;
	}
}

uint16_t CFDDController::GetDataDebug() // Получить значение порта 177132 - данные для отладки
{
	return m_datareg;
}

uint16_t CFDDController::GetStateDebug() // Получить значение порта 177130 - состояние устройства для отладки
{
	return m_status;
}

uint16_t CFDDController::GetWriteDataDebug() // Получить значение порта 177132 - переданные данные для отладки
{
	return m_writereg;
}

uint16_t CFDDController::GetCmdDebug() // Получить значение порта 177130 - переданные команды для отладки
{
	return m_flags;
}

uint16_t CFDDController::GetData()
{
///	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);
	m_status &= ~(FLOPPY_STATUS_MOREDATA);
	m_bWriteMode = m_bSearchSync = false;
	m_writeflag = m_shiftflag = false;

	if (m_pDrive && m_pDrive->isAttached())
	{
		return m_datareg;
	}

	return 0;
}

void CFDDController::WriteData(uint16_t data)
{
///	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);
	m_bWriteMode = true;  // Переключаемся в режим записи, если ещё не переключились
	m_bSearchSync = false;

	if (!m_writeflag && !m_shiftflag)  // оба регистра пустые
	{
		m_shiftreg = data;
		m_shiftflag = true;
		m_status |= FLOPPY_STATUS_MOREDATA;
	}
	else if (!m_writeflag && m_shiftflag)  // Регистр записи пуст
	{
		m_writereg = data;
		m_writeflag = true;
		m_status &= ~FLOPPY_STATUS_MOREDATA;
	}
	else if (m_writeflag && !m_shiftflag)  // Регистр сдвига пуст
	{
		m_shiftreg = m_writereg;
		m_shiftflag = m_writeflag;
		m_writereg = data;
		m_writeflag = true;
		m_status &= ~FLOPPY_STATUS_MOREDATA;
	}
	else // Оба регистра не пусты
	{
		m_writereg = data;
	}
}

void CFDDController::Periodic() // сдвиг на 1 RAW слово на дорожке
{
	if (!IsEngineOn())
	{
		return;    // Вращаем дискеты только если включён мотор
	}

	{
		// область действия мутекса
	///	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);

		// Вращаем дискеты во всех приводах сразу
		for (auto &flp : m_pFloppy)
		{
			if (flp)
			{
				flp->MovePtr();
			}
		}

		// Далее обрабатываем чтение/запись на текущем приводе
		if (m_pDrive && m_pDrive->isAttached())
		{
			if (m_pDrive->isIndex())
			{
				m_status |= FLOPPY_STATUS_INDEXMARK;
			}
			else
			{
				m_status &= ~FLOPPY_STATUS_INDEXMARK;
			}

			if (m_bWriteMode)   // Режим записи
			{
				if (m_shiftflag)
				{
					m_pDrive->WrData(m_shiftreg);
					m_shiftflag = false;

					if (m_shiftmarker)
					{
						m_pDrive->setMarker(true);
						m_shiftmarker = false;
						m_bCRCCalc = true;  // Начинаем считать CRC
						ini_crc_16_rd();
					}
					else
					{
						m_pDrive->setMarker(false);
					}

					if (m_bCRCCalc)
					{
						add_crc_16(m_shiftreg & 0377);
						add_crc_16((m_shiftreg >> 8) & 0377);
					}

					if (m_writeflag)
					{
						m_shiftreg = m_writereg;
						m_shiftflag = m_writeflag;
						m_writeflag = false;
						m_shiftmarker = m_writemarker;
						m_writemarker = false;
						m_status |= FLOPPY_STATUS_MOREDATA;
					}
					else
					{
						if (m_bCRCCalc)  // Прекращаем считать CRC
						{
							m_bCRCCalc = false;
							m_shiftreg = (m_nCRCAcc << 8) | (m_nCRCAcc >> 8) & 0377; // вот она, наша рассчитанная CRC, пока так.
							m_shiftflag = true;
							m_shiftmarker = false;
							m_status |= FLOPPY_STATUS_CHECKSUMOK;
						}
					}
				}
			}
			else    // Режим чтения
			{
				m_datareg = m_pDrive->RdData();

				if (m_bCRCCalc)
				{
					add_crc_16_rd((m_shiftreg >> 8) & 0377);
					add_crc_16_rd(m_shiftreg & 0377);
				}

				if (m_status & FLOPPY_STATUS_MOREDATA)
				{
					if (m_bCRCCalc)  // Прекращаем считать CRC
					{
						m_bCRCCalc = false;

						// Сравниваем рассчитанную CRC с m_datareg
						if (m_bCrcOK)
						{
							m_status |= FLOPPY_STATUS_CHECKSUMOK;
						}
					}
				}
				else
				{
					m_shiftreg = m_datareg;

					if (m_bSearchSync)  // Поиск маркера
					{
						if (m_pDrive->getMarker())  // Нашли маркер
						{
							m_status |= FLOPPY_STATUS_MOREDATA;
							m_bSearchSync = false;
							m_bCRCCalc = true;
							ini_crc_16_rd();
						}
					}
					else  // Просто чтение
					{
						m_status |= FLOPPY_STATUS_MOREDATA;
					}
				}
			}
		}
	}   // область действия мутекса

#ifdef _DEBUG
	// Посчитаем FPS, в норме должно быть 15625 в сек. == 300 об/мин == 5 об/сек
	m_nFrame++; // сколько раз в секунду вызывается эта функция
	DWORD mtmpsec;

	if ((mtmpsec = GetTickCount()) - m_msec >= 1000)
	{
		m_msec = mtmpsec;
		TRACE(_T("FDD Frames %d\n"), m_nFrame);
		m_nFrame = 0;
	}

#endif
}

// инициализация вспомогательных переменных для подсчёта CRC при чтении
void CFDDController::ini_crc_16_rd()
{
	m_nCRCAcc = 0xffff; // начальное значение CRC
	m_bCrcOK = false;   // флаг, что CRC совпала
	m_nA1Cnt = 0;       // счётчик A1
	m_nCrcCnt = 0;      // счётчик байтов данных
	m_nCrcSecSz = FLOPPY_SECTOR_TYPE;   // тип размера сектора
	m_bHdrMk = false;   // флаг маркера заголовка сектора
	m_bCrcMarker = true; // флаг анализа маркера
}

void CFDDController::add_crc_16(uint8_t val)
{
	m_nCRCAcc ^= val << 8;

	for (int i = 0; i < 8; ++i)
	{
		if ((m_nCRCAcc <<= 1) & 0x10000)
		{
			m_nCRCAcc ^= 0x1021;
		}
	}
}

// переусложнённая процедура подсчёта CRC при чтении
// всё из-за того, что было непонятно, как останавливаться и не считать лишнего.
// на самом деле наверное всё гораздо проще, но я не знаю как это сделать.
void CFDDController::add_crc_16_rd(uint8_t val)
{
	if (m_bCrcMarker)               // если анализируем маркер
	{
		if (val == 0xa1)            // если A1,
		{
			m_nA1Cnt++;             // то посчитаем её
			m_nCrcCnt = 1;          // пока будет 1 байт
		}
		else if (m_nA1Cnt == 3)     // если не A1, но их уже было три подряд
		{
			m_nA1Cnt = 0;           // всё, это условие больше не нужно

			if (val == 0xfe)        // это маркер заголовка?
			{
				m_bHdrMk = true;    // да
				m_nCrcCnt = 4;      // размер данных заголовка
			}
			else
			{
				m_bHdrMk = false;   // это не маркер заголовка

				if (val == 0xfb || val == 0xf8) // это маркер данных?
				{
					m_nCrcCnt = CFloppyDrive::m_nSectorSizes[m_nCrcSecSz]; // да, вот размер данных сектора
				}
			}

			m_nCrcCnt++;            // учтём в подсчёте байт маркера
			m_bCrcMarker = false;   // всё, анализ маркера закончен, чтобы не было ложных срабатываний при подсчёте CRC
		}
		else // если A1 было не три подряд, то это и не маркер
		{
			m_bCrcMarker = false; // не надо больше анализировать
		}
	}

	if (m_nCrcCnt <= 0)     // расчёт окончен?
	{
		return;             // да - игнорируем данные
	}

	if (m_bHdrMk && m_nCrcCnt == 1) // это заголовок и его последний байт?
	{
		m_bHdrMk = false;           // да - всё, больше это условие не нужно
		m_nCrcSecSz = val;          // вот это тип размера сектора
		ASSERT(m_nCrcSecSz < 4);

		if (m_nCrcSecSz >= 4)       // чтобы прога не падала
		{
			m_nCrcSecSz = FLOPPY_SECTOR_TYPE; // подкорректируем некорректное значение
		}
	}

	add_crc_16(val);    // считаем CRC
	m_nCrcCnt--;    // уменьшаем счётчик байтов

	if (m_nCrcCnt == 0) // если это был последний байт,
	{
		// то проверяем, совпадает ли прочитанная CRC с расчётной
		m_bCrcOK = !(m_datareg - (m_nCRCAcc & 0xffff));
		// больше мы сюда не попадём, и этот флаг останется в этом состоянии
		// пока не будет сброшен.
	}
}



