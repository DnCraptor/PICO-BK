#include "fdd.h"
#include "CPU_ef.h"
#include "MKDOS318B.h"

void EmulateFDD() {
    DSK_PRINT(("EmulateFDD enter"));
    uint16_t table_addr = Device_Data.CPU_State.r[3];
    // заполняем блок параметров драйвера дисковода
    TABLE_EMFDD dt = { 0 };
    uint16_t* wdt = (uint16_t*)(&dt); // структура в виде массива слов.
	uint16_t t = table_addr;
	for (size_t i = 0; i < sizeof(dt) / sizeof(uint16_t); ++i) {
		wdt[i] = CPU_ReadMemW(t);
        DSK_PRINT(("EmulateFDD 0%o: %04Xh", i*2, wdt[i]));
		t += sizeof(uint16_t);
	}
	int drive = dt.UNIT;
    DSK_PRINT(("EmulateFDD drive: %d", drive));
	// Если номер привода выходит за диапазон A: - D:
	if (drive > 3) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_0_TRACK_ERROR);
        DSK_PRINT(("EmulateFDD FDD_0_TRACK_ERROR"));
		return;
	}
	int nSides = 2;
	if (dt.FLGTAB[drive] & 2) { // определим, сколько сторон у диска
		nSides = 1;
	}
    DSK_PRINT(("EmulateFDD nSides: %d", nSides));
	// специально для 253й прошивки
	if (0167 != CPU_ReadMemW(0160016)) {
		dt.MAXSEC = 10;
	}
    DSK_PRINT(("EmulateFDD nSides: %d MAXSEC: %d WCNT: %d", nSides, dt.MAXSEC, dt.WCNT));
	// Если длина чтения/записи равна 0 - просто выходим без ошибок
	if (!dt.WCNT) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_NOERROR);
        DSK_PRINT(("EmulateFDD FDD_NOERROR"));
		return;
	}
    DSK_PRINT(("EmulateFDD SECTOR: %d SIDE: %d TRK: %d", dt.SECTOR, dt.SIDE, dt.TRK));
	if ((dt.SECTOR == 0 || dt.SECTOR > dt.MAXSEC) || (dt.SIDE >= 2) || (dt.TRK >= 82)) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_BAD_FORMAT);
        DSK_PRINT(("EmulateFDD FDD_BAD_FORMAT"));
		return;
	}
	bool bErrors = false;
	// зададим значение копии регистра по записи как в оригинале
	CPU_WriteW(table_addr, 020 | (1 << drive));
	// высчитаем позицию.
	size_t pos = (((dt.TRK * (size_t)nSides) + dt.SIDE) * dt.MAXSEC + (size_t)dt.SECTOR - 1) * 512;
	uint16_t addr = dt.ADDR;
	int length = dt.WCNT;
	if (length > 0) {
		// чтение
		do {
			if (pos > sizeof(MKDOS318B) - sizeof(uint16_t)) {
				bErrors = true;
                Device_Data.CPU_State.psw |= (1 << C_FLAG);
                CPU_WriteW(052, FDD_STOP);
                DSK_PRINT(("EmulateFDD FDD_STOP"));
				break;
			}
            uint16_t word = (uint16_t)MKDOS318B[pos++] << 8;
            word |= (uint16_t) MKDOS318B[pos++];
			CPU_WriteW(addr, word);
            DSK_PRINT(("EmulateFDD addr: 0%o, %04Xh", addr, word));
			addr += sizeof(uint16_t);
		}
		while (--length);
	}
	else {
		// Если образ примонтирован, и операция записи, а образ защищён от записи
		if (true) {
			// то сообщим об этом ошибкой
            Device_Data.CPU_State.psw |= (1 << C_FLAG);
            CPU_WriteW(052, FDD_DISK_PROTECTED);
            DSK_PRINT(("EmulateFDD FDD_DISK_PROTECTED"));
			return;
		}
		// запись
		/*			length = -length;
					do {
						uint16_t word = pBoard->GetWord(addr);
						if (!pFD->FDWrite(&word, sizeof(uint16_t))) {
							bErrors = true;
							pBoard->SetPSWBit(PSW_BIT::C, true);
							pBoard->SetWordIndirect(052, FDD_STOP);
							break;
						}
						addr += sizeof(uint16_t);
					}
					while (--length);*/
	}
	if (!bErrors) {
        Device_Data.CPU_State.psw &= ~(1 << C_FLAG);
        CPU_WriteW(052, FDD_NOERROR);
        DSK_PRINT(("EmulateFDD FDD_NOERROR"));
	}
    DSK_PRINT(("EmulateFDD exit"));
}

/////////////////////////////////////////////////////////////////////////////
// CFloppy

const uint16_t FLOPPY_CMD_SELECT_DRIVE_A    =  01   ;   // выбор накопителя 0
const uint16_t FLOPPY_CMD_SELECT_DRIVE_B    =  02   ;   // выбор накопителя 1
const uint16_t FLOPPY_CMD_SELECT_DRIVE_C    =  04   ;   // выбор накопителя 2
const uint16_t FLOPPY_CMD_SELECT_DRIVE_D    =  010  ;   // выбор накопителя 3
const uint16_t FLOPPY_CMD_ENGINESTART       =  020  ;   // включение электродвигателя
const uint16_t FLOPPY_CMD_SIDEUP            =  040  ;   // выбор стороны, 0 - верхняя
const uint16_t FLOPPY_CMD_DIR               =  0100 ;   // направление шага
const uint16_t FLOPPY_CMD_STEP              =  0200 ;   // шаг
const uint16_t FLOPPY_CMD_READDATA          =  0400 ;   // признак "начало чтения"
const uint16_t FLOPPY_CMD_WRITEMARKER       =  01000;   // признак "запись маркера"
const uint16_t FLOPPY_CMD_PRECORRECTION     =  02000;   // включение схемы прекоррекции

const uint16_t FLOPPY_CMD_MASKSTORED = (FLOPPY_CMD_SELECT_DRIVE_A | FLOPPY_CMD_SELECT_DRIVE_B | FLOPPY_CMD_SELECT_DRIVE_C | FLOPPY_CMD_SELECT_DRIVE_D | \
                                        FLOPPY_CMD_ENGINESTART | FLOPPY_CMD_SIDEUP | FLOPPY_CMD_DIR | FLOPPY_CMD_STEP | FLOPPY_CMD_READDATA | \
                                        FLOPPY_CMD_WRITEMARKER | FLOPPY_CMD_PRECORRECTION);

const uint16_t FLOPPY_STATUS_TRACK0         = 01     ;  // признак "дорожка 0"
const uint16_t FLOPPY_STATUS_RDY            = 02     ;  // признак готовности накопителя
const uint16_t FLOPPY_STATUS_WRITEPROTECT   = 04     ;  // запрет записи
const uint16_t FLOPPY_STATUS_MOREDATA       = 0200   ;  // признак, что данные готовы или требование новых данных
const uint16_t FLOPPY_STATUS_CHECKSUMOK     = 040000 ;  // для чтения - признак записи ЦКС
const uint16_t FLOPPY_STATUS_INDEXMARK      = 0100000;  // датчик индексного отверстия

static uint16_t m_flags = FLOPPY_CMD_SIDEUP | FLOPPY_CMD_DIR | FLOPPY_CMD_WRITEMARKER; // См. определения FLOPPY_CMD_XXX (флаги, передаваемые контроллеру дисковода)
static int m_drive = -1; // Номер привода: от 0 до 3; -1 если не выбран
static int m_side = 0;   // Сторона диска: 0 или 1
static int m_track = 0;
static int m_datareg, m_writereg, m_shiftreg = 0;
static bool m_bSearchSyncTrigger = false;
static bool m_bWriteMode, m_writemarker, m_shiftmarker, m_bCRCCalc = false;
static uint16_t m_nCRCAcc = 0xffff;
static bool m_bSearchSync = true;
static bool m_writeflag, m_shiftflag = false;
static int m_status = 0;
static bool m_bCrcOK = false;

void SetCommand(uint16_t cmd) {
	// Копируем новые флаги в m_flags
	m_flags &= ~FLOPPY_CMD_MASKSTORED;
	m_flags |= cmd & FLOPPY_CMD_MASKSTORED;
	bool okPrepareTrack = false;  // Нужно ли считывать дорожку в буфер
	// Проверить, не сменился ли текущий привод
	// тут выбирается текущий привод, чем более младший бит, тем больше приоритет
	// если установлено несколько битов - выбирается тот, что младше.
	int newdrive = (cmd & 1) ? 0 : (cmd & 2) ? 1: (cmd & 4) ? 2 : (cmd & 8) ? 3 : -1;
	if (m_drive != newdrive) {
		// предыдущему приводу, если он был, надо сохранить изменения
	//	if (m_pDrive) {
	//		m_pDrive->FlushChanges();
	//	}
		m_drive = newdrive;
	//	m_pDrive = (newdrive == FDD_DRIVE::NONE) ? nullptr : m_pFloppy[static_cast<int>(m_drive)].get();
		okPrepareTrack = true;
	}

	if (m_drive == -1 /*|| m_pDrive == nullptr*/) // если привод не выбран или выбран не существующий
	{
		return; // нечего и делать
	}
	// Проверяем, не сменилась ли сторона
	if (m_flags & FLOPPY_CMD_SIDEUP) { // Выбор стороны: 0 - нижняя, 1 - верхняя
		if (m_side == 0) {
			m_side = 1;
			okPrepareTrack = true;
		}
	} else {
		if (m_side == 1) {
			m_side = 0;
			okPrepareTrack = true;
		}
	}
	if (m_flags & FLOPPY_CMD_STEP) { // Двигаем головки в заданном направлении
		if (m_flags & FLOPPY_CMD_DIR) {
			if (m_track < 82) {
				m_track++;
				okPrepareTrack = true;
			}
		} else {
			if (m_track >= 1) {
				m_track--;
				okPrepareTrack = true;
			}
		}
		if (m_track == 0) {
			m_status |= FLOPPY_STATUS_TRACK0;
		} else {
			m_status &= ~FLOPPY_STATUS_TRACK0;
		}
	}
	if (okPrepareTrack) {
	//	m_pDrive->PrepareTrack(m_track, m_side);
	}
	if (m_flags & FLOPPY_CMD_READDATA) { // Поиск маркера
		m_bSearchSyncTrigger = true;    // если бит FLOPPY_CMD_READDATA == 1, взводим триггер
	} else if (m_bSearchSyncTrigger) {  // если бит FLOPPY_CMD_READDATA == 0 и триггер взведён
		m_bSearchSyncTrigger = false;   // переходим в режим поиска маркера
		m_bSearchSync = true;
		m_status &= ~(FLOPPY_STATUS_CHECKSUMOK | FLOPPY_STATUS_MOREDATA);
		m_bCrcOK = false;
	}
	if (m_bWriteMode && (m_flags & FLOPPY_CMD_WRITEMARKER)) { // Запись маркера
		m_writemarker = true;
		m_status &= ~(FLOPPY_STATUS_CHECKSUMOK | FLOPPY_STATUS_MOREDATA);
		m_bCrcOK = false;
	}
}

uint16_t GetState() {
//	if (m_pDrive == nullptr)   // если привод не выбран
//	{
//		return 0;    // то возвращаем 0
//	}
//	if (!m_pDrive->isAttached())    // если файл не открыт - аналог того, что в дисководе нет диска
//	{
//		return FLOPPY_STATUS_INDEXMARK | (m_status & FLOPPY_STATUS_TRACK0);
//	}
//	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);
	// далее, формируем возвращаемый статус, для выбранного привода.
	// если двигатель включён, то выставим флаг готовности.
	if (m_flags & FLOPPY_CMD_ENGINESTART) {
		m_status |= FLOPPY_STATUS_RDY;
	} else {
		m_status &= ~(FLOPPY_STATUS_RDY | FLOPPY_STATUS_MOREDATA | FLOPPY_STATUS_CHECKSUMOK);
	}
	return m_status;
}

uint16_t GetData() {
//	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);
	m_status &= ~(FLOPPY_STATUS_MOREDATA);
	m_bWriteMode = m_bSearchSync = false;
	m_writeflag = m_shiftflag = false;
//	if (m_pDrive && m_pDrive->isAttached())
	{
		return m_datareg;
	}
	return 0;
}

void WriteData(uint16_t data) {
//	std::lock_guard<std::mutex> lk(m_mutPeriodicBusy);
	m_bWriteMode = true;  // Переключаемся в режим записи, если ещё не переключились
	m_bSearchSync = false;
	if (!m_writeflag && !m_shiftflag) { // оба регистра пустые
		m_shiftreg = data;
		m_shiftflag = true;
		m_status |= FLOPPY_STATUS_MOREDATA;
	} else if (!m_writeflag && m_shiftflag) { // Регистр записи пуст
		m_writereg = data;
		m_writeflag = true;
		m_status &= ~FLOPPY_STATUS_MOREDATA;
	} else if (m_writeflag && !m_shiftflag) { // Регистр сдвига пуст
		m_shiftreg = m_writereg;
		m_shiftflag = m_writeflag;
		m_writereg = data;
		m_writeflag = true;
		m_status &= ~FLOPPY_STATUS_MOREDATA;
	} else { // Оба регистра не пусты
		m_writereg = data;
	}
}
