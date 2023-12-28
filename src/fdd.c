#include "fdd.h"
#include "CPU_ef.h"
#include "MKDOS318B.h"

void EmulateFDD() {
    uint16_t table_addr = Device_Data.CPU_State.r[3];
    // заполняем блок параметров драйвера дисковода
    TABLE_EMFDD dt = { 0 };
    uint16_t* wdt = (uint16_t*)(&dt); // структура в виде массива слов.
	uint16_t t = table_addr;
	for (size_t i = 0; i < sizeof(dt) / sizeof(uint16_t); ++i) {
		*(wdt + i) = CPU_ReadMemW(t);
		t += sizeof(uint16_t);
	}
	int drive = dt.UNIT;
	// Если номер привода выходит за диапазон A: - D:
	if (drive > 3) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_0_TRACK_ERROR);
		return;
	}
	int nSides = 2;
	if (dt.FLGTAB[drive] & 2) { // определим, сколько сторон у диска
		nSides = 1;
	}
	// специально для 253й прошивки
	if (0167 != CPU_ReadMemW(0160016)) {
		dt.MAXSEC = 10;
	}
	// Если длина чтения/записи равна 0 - просто выходим без ошибок
	if (!dt.WCNT) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_NOERROR);
		return;
	}
	if ((dt.SECTOR == 0 || dt.SECTOR > dt.MAXSEC) || (dt.SIDE >= 2) || (dt.TRK >= 82)) {
        Device_Data.CPU_State.psw |= (1 << C_FLAG);
        CPU_WriteW(052, FDD_BAD_FORMAT);
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
		uint16_t word;
		// чтение
		do {
			if (pos > sizeof(MKDOS318B) - sizeof(uint16_t)) {
				bErrors = true;
                Device_Data.CPU_State.psw |= (1 << C_FLAG);
                CPU_WriteW(052, FDD_STOP);
				break;
			}
            word = *(uint16_t*)&MKDOS318B[pos];
			CPU_WriteW(addr, word);
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
	}
}

