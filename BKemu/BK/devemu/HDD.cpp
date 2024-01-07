#include "pch.h"
#include "HDD.h"

// начало области данных в файле образе винта
constexpr auto DATA_OFFSET = SECTOR_SIZEB;

CHDD::CHDD(const fs::path &name, HDD_MODE mode)
	: m_nDeviceID(0)
#if (THREADED_MODEL)
	, m_bThreadTerminate(false)
	, m_bNotify(false)
#endif // THREADED_MODEL
{
	attach_hdd(name, mode);
	int_init();
}

CHDD::CHDD()
	: m_nDeviceID(0)
#if (THREADED_MODEL)
	, m_bThreadTerminate(false)
	, m_bNotify(false)
#endif // THREADED_MODEL
{
	attach_hdd(_T(""), HDD_MODE::MASTER);
	int_init();
}

CHDD::~CHDD()
{
	// если в буфере есть данные нужно сделать flush buffer;
	if (is_attached())
	{
		write_file();
	}

	detach_hdd();
#if (THREADED_MODEL)
	StopExecuteThread();
#endif // THREADED_MODEL
}


#if (THREADED_MODEL)

void CHDD::StopExecuteThread()
{
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		m_bThreadTerminate = true;
		m_bNotify = true;
	}
	m_cv.notify_one();
	// вот такая странная ситуация: после подачи notify_one, поток не успевает
	// пробудиться и захватить мутекс, и деструктор успевает убить объект CHDD.
	// а поток остаётся в памяти. Потому что мутекс захватывается тут, а потом
	// сразу освобождается. Поэтому приходится добавлять ещё один мутекс,
	// который залочен всегда, пока поток существует, и разлочивается при его
	// уничтожении, а тут мы ждём, пока сможем залочить этот второй мутекс
	std::lock_guard<std::mutex> lk(m_mutThr);
}

bool CHDD::StartExecuteThread()
{
	m_ExecuteThread = std::thread(&CHDD::ExecuteThreadFunc, this);

	if (m_ExecuteThread.joinable())
	{
		m_ExecuteThread.detach();
		return true;
	}

	return false;
}

void CHDD::ExecuteThreadFunc()
{
	m_mutThr.lock();

	for (;;)
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		m_cv.wait(lk, [&]()
		{
			return m_bNotify;
		});
		m_bNotify = false;
		lk.unlock();

		if (m_bThreadTerminate) // сигнал к остановке потока получен?
		{
			break;   // и выходим
		}

		execute_command(static_cast<IDE_CMD>(m_reg.command));
		m_reg.status &= ~HDD_STATUS::BSY;
	}

	m_mutThr.unlock();
}

#endif // THREADED_MODEL


void CHDD::int_init()
{
	m_nSectorCounter = 0;
	m_nSectorPosition = DATA_OFFSET;
	m_reg.clear();
	memset(m_pSectorBuffer.b, 0, SECTOR_SIZEB);
	m_bChangeBuffer = false;
	m_bReadOnly = true;
#if (THREADED_MODEL)
	StartExecuteThread();
#endif // THREADED_MODEL
}

bool CHDD::attach_hdd(const fs::path &name, HDD_MODE mode)
{
	detach_hdd();
	m_strImageFileName = name;
	m_nDeviceID = (mode == HDD_MODE::SLAVE) ? 0x10 : 0;

	if (!m_strImageFileName.empty()) // если имя файла не пустое
	{
		bool bRet = false;

		if (bRet = !!(m_fHDDImageFile.Open(m_strImageFileName.c_str(), CFile::modeReadWrite | CFile::shareDenyWrite | CFile::typeBinary | CFile::osRandomAccess)))
		{
			m_bReadOnly = false;
		}
		else if (bRet = !!(m_fHDDImageFile.Open(m_strImageFileName.c_str(), CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary | CFile::osRandomAccess)))
		{
			m_bReadOnly = true;
		}

		if (bRet)
		{
			return configure();
		}
	}

	return false;
}

void CHDD::detach_hdd()
{
	if (is_attached())
	{
		m_fHDDImageFile.Close();
		m_strImageFileName.clear();
	}
}

bool CHDD::is_attached() const
{
	return (m_fHDDImageFile.m_hFile != CFile::hFileNull);
}


bool CHDD::configure()
{
	m_nPhase = HDD_IDE_PHASE::FAILURE;

	if (is_attached())
	{
		/* перемещаемся к началу файла */
		if (m_fHDDImageFile.Seek(0, CFile::SeekPosition::begin) != 0)
		{
			return false;
		}

		/* читаем служебный сектор */
		if (m_fHDDImageFile.Read(m_pSectorBuffer.b, SECTOR_SIZEB) != SECTOR_SIZEB)
		{
			return false;    /* read error */
		}

		// проверим данные на валидность
		uint8_t cs = 0;

		for (int i = 0; i < 511; ++i)
		{
			cs += m_pSectorBuffer.b[i];
		}

		cs = -cs;

		if (m_pSectorBuffer.b[511] == cs && m_pSectorBuffer.b[510] == 0xA5)
		{
			m_nCylinders = m_pSectorBuffer.w[1];
			m_nHeads = m_pSectorBuffer.w[3];
			m_nSectors = m_pSectorBuffer.w[6];
			m_nHDDSize = m_nCylinders * m_nHeads * m_nSectors;
			command_ok();
			return true;
		}

		// если образ неверный, то его просто отключаем.
		// чтобы с ним не случилось ничего такого.
		detach_hdd();
	}

	return false;
}

void CHDD::command_ok()
{
	m_nPhase = HDD_IDE_PHASE::READY;
	m_nSectIdx_Word = 0;
	m_reg.error = 0;
	m_reg.status = HDD_STATUS::DRDY | HDD_STATUS::DSC;
}

void CHDD::write_regs(HDD_REGISTER reg, uint16_t data)
{
	if (!is_attached())
	{
		return;
	}

	switch (reg)
	{
		case HDD_REGISTER::H1F0:
			if ((m_reg.head ^ m_nDeviceID) & 0x10)
			{
				return;
			}

			if (m_reg.status & HDD_STATUS::DRQ)
			{
				write_data(data);
			}

			break;

		case HDD_REGISTER::H1F1:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.features = data & 0xff;
			}

			break;

		case HDD_REGISTER::H1F2:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.count = data & 0xff;
				m_nSectorCounter = m_reg.count ? m_reg.count : 256;
			}

			break;

		case HDD_REGISTER::H1F3:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.sector = data & 0xff;
			}

			break;

		case HDD_REGISTER::H1F4:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.cyl_lo = data & 0xff;
			}

			break;

		case HDD_REGISTER::H1F5:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.cyl_hi = data & 0xff;
			}

			break;

		case HDD_REGISTER::H1F6:
			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.head = data & 0xff;
				uint16_t hs = (m_reg.head & HDD_IDE_HEADREG::HEAD) << 2;
				m_reg.addr &= ~HDD_DRVADDR_HS;
				m_reg.addr |= hs;

				if (m_reg.head & HDD_IDE_HEADREG::DEV)
				{
					m_reg.addr |= HDD_DRVADDR_DS1;
					m_reg.addr &= ~HDD_DRVADDR_DS0;
				}
				else
				{
					m_reg.addr |= HDD_DRVADDR_DS0;
					m_reg.addr &= ~HDD_DRVADDR_DS1;
				}
			}

			break;

		case HDD_REGISTER::H1F7:
			if (((m_reg.head ^ m_nDeviceID) & 0x10) && (data != 0x90))
			{
				return;
			}

			if (!(m_reg.status & (HDD_STATUS::BSY | HDD_STATUS::DRQ)))
			{
				m_reg.command = data & 0xff;
#if (THREADED_MODEL)
				m_reg.status |= HDD_STATUS::BSY; // выставляем статус BSY, он снимается после выполнения команды
				{
					std::lock_guard<std::mutex> lk(m_mutex);
					m_bNotify = true;
				}
				m_cv.notify_one();
#else
				execute_command(static_cast<IDE_CMD>(m_reg.command));
#endif // THREADED_MODEL
			}

			break;

		case HDD_REGISTER::H3F6:
			m_reg.control = data & 0xff;

			if (m_reg.control & CONTROL_SRST)
			{
				reset(false);
			}

			break;
			// запись в остальные значения надо просто игнорировать
	}
}

void CHDD::write_data(uint16_t data)
{
	/* Data can only be written in PIO output phase */
	if (m_nPhase != HDD_IDE_PHASE::PIO_OUT)
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.status &= ~HDD_STATUS::DRQ;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE;  //!!!
		return;
	}

	m_reg.status |= HDD_STATUS::BSY;
	m_bChangeBuffer = true;
	m_reg.data = data;
	m_pSectorBuffer.w[m_nSectIdx_Word] = data; // пишем данные в буфер

	if (++m_nSectIdx_Word >= SECTOR_SIZEW) // если пришла пора сохранять буфер
	{
		m_reg.addr |= HDD_DRVADDR_WG;

		if (!write_file())
		{
			m_reg.status |= HDD_STATUS::ERR | HDD_STATUS::DWF;
			m_reg.error = ERR_ABRT | ERR_AMNF;
		}

		if (m_nSectorCounter)
		{
			writesector();
		}
		else
		{
			/* all sectors done */
			command_ok();
		}

		m_reg.addr &= ~HDD_DRVADDR_WG;
	}

	m_reg.status &= ~HDD_STATUS::BSY;
}

/* Execute the WRITE SECTOR command */
void CHDD::writesector()
{
	if (!seek())
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE;  //!!!
		return;
	}

	/* Initiate the PIO output phase */
	m_nPhase = HDD_IDE_PHASE::PIO_OUT;
	m_reg.status |= HDD_STATUS::DRQ;
	m_reg.error = 0;
	m_nSectIdx_Word = 0;
}

bool CHDD::write_file()
{
	if (!m_bChangeBuffer) // если буфер не изменялся, то и записывать нечего.
	{
		return true;
	}

	/* перемещаемся к нужной позиции */
	if (m_fHDDImageFile.Seek(m_nSectorPosition, CFile::SeekPosition::begin) != m_nSectorPosition)
	{
		return false;
	}

	/* Сохраняем сектор */
	try
	{
		m_fHDDImageFile.Write(m_pSectorBuffer.b, SECTOR_SIZEB);
	}
	catch (...) // единственный способ узнать, произошла ли ошибка при записи
	{
		/* write error */
		return false;
	}

	m_bChangeBuffer = false;
	return true;
}

uint16_t CHDD::read_regs(HDD_REGISTER reg)
{
	uint16_t dret = 0xffff;

	if (!is_attached())
	{
		return dret;
	}

	if ((m_reg.head ^ m_nDeviceID) & 0x10)
	{
		return dret;
	}

	switch (reg)
	{
		case HDD_REGISTER::H1F0:
			if (m_reg.status & HDD_STATUS::DRQ)
			{
				return read_data();
			}

			break;

		case HDD_REGISTER::H1F1:
			return m_reg.error;

		case HDD_REGISTER::H1F2:
			return m_reg.count;

		case HDD_REGISTER::H1F3:
			return m_reg.sector;

		case HDD_REGISTER::H1F4:
			return m_reg.cyl_lo;

		case HDD_REGISTER::H1F5:
			return m_reg.cyl_hi;

		case HDD_REGISTER::H1F6:
			return m_reg.head;

		case HDD_REGISTER::H1F7:
			return m_reg.status;

		case HDD_REGISTER::H3F6:
			return m_reg.status;

		case HDD_REGISTER::H3F7:
			return m_reg.addr;
	}

	return dret;
}

uint16_t CHDD::read_data()
{
	if (m_nPhase != HDD_IDE_PHASE::PIO_IN)
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.status &= ~HDD_STATUS::DRQ;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE;  //!!!
		return 0xffff;
	}

	m_reg.status |= HDD_STATUS::BSY;
	m_reg.data = m_pSectorBuffer.w[m_nSectIdx_Word];

	if (++m_nSectIdx_Word >= SECTOR_SIZEW)
	{
		if (m_nSectorCounter)
		{
			/* more sectors to read */
			readsector(false);
		}
		else
		{
			/* all sectors done */
			command_ok();
		}
	}

	m_reg.status &= ~HDD_STATUS::BSY;
	return m_reg.data;
}

void CHDD::read_verify_sector()
{
	if (m_nPhase != HDD_IDE_PHASE::READY)
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.status &= ~HDD_STATUS::DRQ;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE;  //!!!
		return;
	}

#ifndef THREADED_MODEL
	m_reg.status |= HDD_STATUS::BSY;
#endif

	while (m_nSectorCounter && !(m_reg.status & HDD_STATUS::ERR))
	{
		/* more sectors to read */
		readsector(true);
	}

	/* all sectors done */
	m_nPhase = HDD_IDE_PHASE::READY;
	m_nSectIdx_Word = 0;
	m_reg.error = 0;
#ifndef THREADED_MODEL
	m_reg.status &= ~HDD_STATUS::BSY;
#endif
}

/* Execute the READ SECTOR command */
void CHDD::readsector(bool bVerify)
{
	if (!seek())
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE; //!!!
		return;
	}

	/* Read data from disk */
	if (read_file())
	{
		/* Initiate the PIO input phase */
		m_nPhase = HDD_IDE_PHASE::PIO_IN;

		if (!bVerify)
		{
			m_reg.status |= HDD_STATUS::DRQ;
		}

		m_reg.error = 0;
		m_nSectIdx_Word = 0;
	}
	else
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.status &= ~HDD_STATUS::DRQ;
		m_reg.error = ERR_ABRT | ERR_AMNF | ERR_UNC;
		m_nPhase = HDD_IDE_PHASE::FAILURE; //!!!
	}
}

bool CHDD::read_file()
{
	/* Seek to the correct file position */
	if (m_fHDDImageFile.Seek(m_nSectorPosition, CFile::SeekPosition::begin) != m_nSectorPosition)
	{
		return false;
	}

	/* Read the packed data into a temporary buffer */
	if (m_fHDDImageFile.Read(m_pSectorBuffer.b, SECTOR_SIZEB) != SECTOR_SIZEB)
	{
		return false;    /* read error */
	}

	m_bChangeBuffer = false;
	return true;
}

/* Seek to the addressed sector */
bool CHDD::seek()
{
	int sectornumber = -1;

	/* Calculate sector number, depending upon LBA/CHS mode. */
	if (m_reg.head & HDD_IDE_HEADREG::LBA)
	{
		sectornumber = (int(m_reg.head & HDD_IDE_HEADREG::HEAD) << 24) |
		               (int(m_reg.cyl_hi) << 16) |
		               (int(m_reg.cyl_lo) << 8) |
		               (m_reg.sector);
	}
	else
	{
		int cylinder = (int(m_reg.cyl_hi) << 8) | (m_reg.cyl_lo);
		int head = (m_reg.head & HDD_IDE_HEADREG::HEAD);
		int sector = int(m_reg.sector) - 1;

		if (cylinder < m_nCylinders && head < m_nHeads &&
		        sector < m_nSectors && sector >= 0)
		{
			sectornumber = (((cylinder * m_nHeads) + head) * m_nSectors) + sector;
		}
	}

	/* Seek to the correct position */
	if (sectornumber < 0 || sectornumber >= m_nHDDSize)
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.status &= ~HDD_STATUS::DRQ;
		m_reg.error = ERR_ABRT | ERR_IDNF;
		m_nPhase = HDD_IDE_PHASE::FAILURE; //!!!
		return false;
	}

	m_nSectorPosition = DATA_OFFSET + (SECTOR_SIZEB * sectornumber);
	/* advance registers to next sector, for multiple sector accesses */
	(--m_reg.count) &= 0xff;

	if (--m_nSectorCounter > 0)
	{
		if (m_reg.head & HDD_IDE_HEADREG::LBA)
		{
			/* increment using LBA scheme */
			uint16_t &s = m_reg.sector;
			s = (s + 1) & 0xff;

			if (!s)
			{
				uint16_t &cl = m_reg.cyl_lo;
				cl = (cl + 1) & 0xff;

				if (!cl)
				{
					uint16_t &ch = m_reg.cyl_hi;
					ch = (ch + 1) & 0xff;

					if (!ch)
					{
						uint16_t &h = m_reg.head;
						uint16_t next_head = (h + 1) & HDD_IDE_HEADREG::HEAD;
						h = (h & ~HDD_IDE_HEADREG::HEAD) | next_head;
					}
				}
			}
		}
		else
		{
			/* increment using CHS scheme */
			uint16_t &s = m_reg.sector;
			s = (s % m_nSectors) + 1;

			/* NB sector number is 1-based */
			if (s == 1)
			{
				uint16_t &h = m_reg.head;
				uint16_t nh = ((h + 1) & HDD_IDE_HEADREG::HEAD) % m_nHeads;
				h = (h & ~HDD_IDE_HEADREG::HEAD) | nh;

				if (!nh)
				{
					uint16_t &cl = m_reg.cyl_lo;
					cl = (cl + 1) & 0xff;

					if (!cl)
					{
						uint16_t &ch = m_reg.cyl_hi;
						ch = (ch + 1) & 0xff;
					}
				}
			}
		}
	}
	else
	{
		m_reg.status &= ~HDD_STATUS::DRQ;
	}

	m_reg.status |= HDD_STATUS::DSC;
	return true;
}

void CHDD::execute_command(IDE_CMD cmd)
{
	if (!is_attached())
	{
		return;
	}

	bool bRet = true;
	m_reg.error = 0;
#if (THREADED_MODEL)
	m_reg.status &= HDD_STATUS::BSY; // очищаем всё кроме BSY
#else
	m_reg.status = 0;
#endif // THREADED_MODEL
	TRACE1("HDD: Execute command %02x\n", cmd);

	switch (cmd)
	{
		case IDE_CMD::NOP:
		case IDE_CMD::IDLE:
		case IDE_CMD::IDLE_IMMEDIATE:
		case IDE_CMD::RECALIBRATE:
		case IDE_CMD::SLEEP:
		case IDE_CMD::STANDBY:
		case IDE_CMD::STANBY_IMMEDIATE:
		case IDE_CMD::DEVICE_RESET: // пока ничего не делать. потому что другие команды, которые должны что-то
			// делать тоже ничего не делают
			command_ok();
			break;

		case IDE_CMD::READ_VERIFY_SECTOR_RETRY:
		case IDE_CMD::READ_VERIFY_SECTOR_NORETRY:
			read_verify_sector();
			break;

		case IDE_CMD::SEEK:
			m_reg.status |= HDD_STATUS::BSY;

			if (seek())
			{
				command_ok();
			}
			else
			{
				m_reg.status &= ~HDD_STATUS::BSY;
				bRet = false;
			}

			break;

		case IDE_CMD::FORMAT_TRACK:
			command_ok();
			m_reg.status |= HDD_STATUS::DRQ;
			break;

		case IDE_CMD::EXECUTE_DEVICE_DIAGNOSTICS:
			reset_signature(false);
			return;

		case IDE_CMD::READ_SECTOR_RETRY:
		case IDE_CMD::READ_SECTOR_NORETRY:
			m_reg.status |= HDD_STATUS::BSY;
			readsector(false);
			m_reg.status &= ~HDD_STATUS::BSY;
			break;

// пока непонятно как считать ecc и как принимать
//  case IDE_CMD::READ_LONG_SECTOR_RETRY:
//  case IDE_CMD::READ_LONG_SECTOR_NORETRY:
//      return;
		case IDE_CMD::WRITE_SECTOR_RETRY:
		case IDE_CMD::WRITE_SECTOR_NORETRY:
		case IDE_CMD::WRITE_VERIFY_SECTOR:
			if (m_bReadOnly)
			{
				m_reg.status = HDD_STATUS::DWF;
			}
			else
			{
				m_reg.status |= HDD_STATUS::BSY;
				writesector();
				m_reg.status &= ~HDD_STATUS::BSY;
			}

			break;

// пока непонятно как считать ecc и как передавать
//  case IDE_CMD::WRITE_LONG_SECTOR_RETRY:
//  case IDE_CMD::WRITE_LONG_SECTOR_NORETRY:
//      return;
// ещё не реализовано.
//  case IDE_CMD::WRITE_SAME:
//  case IDE_CMD::READ_BUFFER:
//  case IDE_CMD::WRITE_BUFFER:
		case IDE_CMD::INITIALIZE_DEVICE_PARAMETERS:
			// эту команду надо игнорировать. Ничего не делать
			break;

		case IDE_CMD::IDENTIFY_DEVICE:
			identifydevice();
			break;

		default:
			TRACE1("HDD: Unknown command %02x\n", cmd);
			// неизвестные команды надо игнорировать, но говорить, что всё ОК
			// а то фигня получается
			m_nPhase = HDD_IDE_PHASE::READY;
	}

	m_reg.status |= HDD_STATUS::DSC | HDD_STATUS::DRDY;

	if (!bRet)
	{
		m_reg.status |= HDD_STATUS::ERR;
		m_reg.error |= ERR_ABRT;
		m_nPhase = HDD_IDE_PHASE::FAILURE; //!!!
	}
}

bool CHDD::identifydevice()
{
	if (is_attached())
	{
		/* перемещаемся к началу файла */
		if (m_fHDDImageFile.Seek(0, CFile::SeekPosition::begin) == 0)
		{
			/* читаем служебный сектор */
			if (m_fHDDImageFile.Read(m_pSectorBuffer.b, SECTOR_SIZEB) == SECTOR_SIZEB)
			{
				m_reg.count = 0; m_nSectorCounter = 0;
				// в регистры CHS поместим геометрию диска
				// m_reg.sector = m_nSectors;
				// m_reg.cyl_lo = LOBYTE(m_nCylinders); // LBA 15..8
				// m_reg.cyl_hi = HIBYTE(m_nCylinders); // LBA 23..16
				// m_reg.head &= ~HDD_IDE_HEADREG::HEAD;
				// m_reg.head |= ((m_nHeads-1) & HDD_IDE_HEADREG::HEAD);
				// в регистры CHS поместим начало диска
				m_reg.sector = 1;
				m_reg.cyl_lo = 0; // LBA 15..8
				m_reg.cyl_hi = 0; // LBA 23..16
				m_reg.head &= ~HDD_IDE_HEADREG::HEAD;
				/* Initiate the PIO input phase */
				m_nPhase = HDD_IDE_PHASE::PIO_IN;
				m_reg.status |= HDD_STATUS::DRQ;
				m_nSectIdx_Word = 0;
				return true;
			}
		}
	}

	memset(m_pSectorBuffer.b, 0, SECTOR_SIZEB);
	m_reg.count = 0; m_nSectorCounter = 0;
	/* Initiate the PIO input phase */
	m_nPhase = HDD_IDE_PHASE::FAILURE;
	m_reg.status |= HDD_STATUS::ERR;
	m_reg.error = ERR_ABRT | ERR_UNC;
	m_nSectIdx_Word = 0;
	return false;
}

void CHDD::reset(bool reset_hard)
{
	m_reg.control = 0;
	reset_signature(reset_hard);

	if (is_attached() && m_nPhase != HDD_IDE_PHASE::FAILURE)
	{
		if (reset_hard)
		{
			command_ok();
		}
		else
		{
			m_nPhase = HDD_IDE_PHASE::READY;
			m_nSectIdx_Word = 0;
		}
	}
}

void CHDD::reset_signature(bool reset_hard)
{
	if (is_attached() && m_nPhase != HDD_IDE_PHASE::FAILURE)
	{
		m_nSectorCounter = 1;
		m_reg.count = m_reg.sector = m_reg.error = 0x01;
		m_reg.cyl_lo = m_reg.cyl_hi = 0;
		m_reg.head = 0;
		m_reg.status = reset_hard ? 0 : HDD_STATUS::DRDY;
	}
	else
	{
		m_nSectorCounter = 0;
		m_nPhase = HDD_IDE_PHASE::FAILURE;
		m_reg.unset();
	}
}

bool CATA_IDE::attach(const fs::path &name, HDD_MODE mode)
{
	int nDrive = 0;

	switch (mode)
	{
		case HDD_MODE::MASTER:
			nDrive = 0;
			break;

		case HDD_MODE::SLAVE:
			nDrive = 1;
			break;
	}

	bool bRet = m_mDrive[nDrive].attach_hdd(name, mode);

	if (bRet)
	{
		m_mDrive[nDrive].reset(true);
	}

	return bRet;
}

void CATA_IDE::detach(HDD_MODE mode /*= HDD_MODE::MASTER*/)
{
	int nDrive = 0;

	switch (mode)
	{
		case HDD_MODE::MASTER:
			nDrive = 0;
			break;

		case HDD_MODE::SLAVE:
			nDrive = 1;
			break;
	}

	m_mDrive[nDrive].detach_hdd();
}

void CATA_IDE::reset()
{
	m_mDrive[0].reset(true);
	m_mDrive[1].reset(true);
}

void CATA_IDE::write_regs(HDD_REGISTER reg, uint16_t data)
{
	m_mDrive[0].write_regs(reg, data);
	m_mDrive[1].write_regs(reg, data);
}

uint16_t CATA_IDE::read_regs(HDD_REGISTER reg)
{
	uint16_t r0 = m_mDrive[0].read_regs(reg);
	uint16_t r1 = m_mDrive[1].read_regs(reg);
	uint16_t r = r0 & r1; // это чтобы смотреть, что происходит, при отладке
	return r;
}

uint16_t CATA_IDE::read_regs_internal(HDD_REGISTER reg) const
{
	uint16_t r0 = m_mDrive[0].get_debug_regs(reg, true);
	uint16_t r1 = m_mDrive[1].get_debug_regs(reg, true);
	uint16_t r = r0 & r1; // это чтобы смотреть, что происходит, при отладке
	return r;
}

uint16_t CATA_IDE::get_debug_regs(int nDrive, HDD_REGISTER reg, bool bReadMode) const
{
	return m_mDrive[nDrive].get_debug_regs(reg, bReadMode);
}

// выдача отладчику содержимого регистров

uint16_t CHDD::get_debug_regs(HDD_REGISTER reg, bool bReadMode) const
{
	if (bReadMode)
	{
		// регистры по чтению
		switch (reg)
		{
			case HDD_REGISTER::H1F0:
				return m_reg.data;

			case HDD_REGISTER::H1F1:
				return m_reg.error;   // по чтению

			case HDD_REGISTER::H1F2:
				return m_reg.count;

			case HDD_REGISTER::H1F3:
				return m_reg.sector;

			case HDD_REGISTER::H1F4:
				return m_reg.cyl_lo;

			case HDD_REGISTER::H1F5:
				return m_reg.cyl_hi;

			case HDD_REGISTER::H1F6:
				return m_reg.head;

			case HDD_REGISTER::H1F7:
				return m_reg.status;  // по чтению

			case HDD_REGISTER::H3F6:
				return m_reg.status;  // по чтению

			case HDD_REGISTER::H3F7:
				return m_reg.addr;
		}
	}
	else
	{
		// регистры по записи
		switch (reg)
		{
			case HDD_REGISTER::H1F0:
				return m_reg.data;

			case HDD_REGISTER::H1F1:
				return m_reg.features; // по записи

			case HDD_REGISTER::H1F2:
				return m_reg.count;

			case HDD_REGISTER::H1F3:
				return m_reg.sector;

			case HDD_REGISTER::H1F4:
				return m_reg.cyl_lo;

			case HDD_REGISTER::H1F5:
				return m_reg.cyl_hi;

			case HDD_REGISTER::H1F6:
				return m_reg.head;

			case HDD_REGISTER::H1F7:
				return m_reg.command; // по записи

			case HDD_REGISTER::H3F6:
				return m_reg.control; // по записи

			case HDD_REGISTER::H3F7:
				return m_reg.addr;
		}
	}

	return -1;
}


