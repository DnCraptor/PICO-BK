/**
 * Partly migration from https://gid.pdp-11.ru/bkemuc.html?custom=3.13.2401.10604
*/
#include "disk_img.h"
#include "mkdos.h"
extern "C" {
#define DBGM_PRINT(X)
#include "stdint.h"
#include "string.h"
#include "ff.h"
#include "stdio.h"
#include "emulator.h"
}
static PARSE_RESULT_C parse_result;

const char *strID_Andos = "ANDOS ";
const char *strID_DXDOS = "DX-DOS";
const char *strID_FAT12 = "FAT12   ";
const char *strID_Aodos = " AO-DOS ";
const char *strID_Nord  = "NORD ";
const char *strID_Nord_Voland = "BY VOLAND"; // мультизагрузочные диски - тоже будем опознавать как норд
const char *strID_RT11  = "?BOOT-";
const char *strID_RT11Q = "?QUBOOT-";

/*
анализатор каталога микродос, для уточнения формата.
2 - Nord
1 - Aodos
0 - Microdos
-1 - ошибка чтения файла
*/
inline static int AnalyseMicrodos(FIL *f, uint8_t *buf)
{
#pragma pack(push)
#pragma pack(1)
	struct MikrodosFileRecord
	{
		uint8_t dir_dir;    // статус записи: бит 7 - бэд блок, остальные значения - неизвестно
		uint8_t dir_num;    // статус записи: значения неизвестны
		uint8_t name[14];   // имя файла, если начинается с 0177 - каталог. и значит это не микродос
		uint16_t start_block;// начальный блок
		uint16_t len_blk;   // длина в блоках, если len_blk == 0 то это тоже с большой долей вероятности каталог
		uint16_t address;   // стартовый адрес
		uint16_t length;    // длина, или остаток длины от длины в блоках, если размер файла > 64кб
		MikrodosFileRecord() {
			memset(this, 0, sizeof(MikrodosFileRecord));
		}
		MikrodosFileRecord &operator = (const MikrodosFileRecord &src) {
			memcpy(this, &src, sizeof(MikrodosFileRecord));
			return *this;
		}
		MikrodosFileRecord &operator = (const MikrodosFileRecord *src) {
			memcpy(this, src, sizeof(MikrodosFileRecord));
			return *this;
		}
	};
#pragma pack(pop)
	int nRet = -1;
	constexpr int nCatSize = 10; // размер каталога - одна сторона нулевой дорожки.
	char pCatBuffer[nCatSize * BLOCK_SIZE] = { 0 }; // TODO: read one by one
	if (f_lseek(f, 0) == FR_OK) {
		// перемещаемся к нулевому сектору
		UINT br;
		if (f_read(f, pCatBuffer, sizeof pCatBuffer, &br) == FR_OK)
		{
			int nRecNum = *(reinterpret_cast<uint16_t *>(&pCatBuffer[030])); // читаем общее кол-во файлов.
			auto pDiskCat = reinterpret_cast<MikrodosFileRecord *>(pCatBuffer + 0500); // каталог диска
			int nMKDirFlag = 0;
			int nAODirFlag = 0;
			// сканируем каталог и ищем там директории
			for (int i = 0; i < nRecNum; ++i) {
				if (pDiskCat[i].name[0] == 0177) {
					nMKDirFlag++;
				} else if (pDiskCat[i].name[0] != 0 && pDiskCat[i].len_blk == 0) {
					nAODirFlag++;
				}
			}
			if (nMKDirFlag && nAODirFlag) {
				nRet = 0;
			} else if (nMKDirFlag) {
				nRet = 2;
			} else if (nAODirFlag) {
				nRet = 1;
			} else {
				nRet = 0;
			}
		}
	}
	return nRet;
}

// сравнение двух символов, без учёта регистра, русских букв не сравниваем, т.к. пока не нужно
// выход: true - равны
//      false - не равны
inline static bool charcompare(uint8_t a, uint8_t b) {
	if ('a' <= a && a <= 'z') {
		a -= ' '; // делаем из маленькой буквы большую
	}
	if ('a' <= b && b <= 'z') {
		b -= ' '; // делаем из маленькой буквы большую
	}
	if (a == b) {
		return true;
	}
	return false;
}

inline static bool substrfind(uint8_t *bufs, size_t len_bufs, uint8_t *buff, size_t len_buff)
{
	size_t b_pos = 0; // позиция в буфере.
	size_t s_pos = 0; // позиция в строке, которую ищем
	size_t t_pos;
	bool bOk = false;
	while (b_pos < len_bufs) {
		// пока есть где искать
		// ищем совпадение с первым символом
		if (charcompare(bufs[b_pos++], buff[s_pos])) {
			// нашли. теперь проверим на совпадения с остальными символами
			t_pos = b_pos;
			s_pos++;    // первый символ совпал, начинаем проверять со второго
			bOk = true; // флаг выхода из цикла поиска на совпадение строки
			while (s_pos < len_buff) {
				// пока строка не кончится
				if (!charcompare(buff[s_pos++], bufs[t_pos++])) {
					// если символ не совпал
					bOk = false; // то не ОК
					break;
				}
			}
			b_pos = t_pos;
			s_pos = 0; // вернёмся на начало строки
			if (bOk) {
				// если таки вся строка совпала
				break; // незачем дальше искать
			}
		}
	}
	// на выходе отсюда bOk == true если строка нашлась, иначе - false
	return bOk;
}

/*
Проверим, а не rt11 ли это старым методом. Поскольку непонятно как узнать, что это у нас УКНЦшный АДОС.
выход:
1 - rt11
0 - не rt11
-1 - ошибка чтения файла
*/
inline static int IsRT11_old(uint8_t *buf) {
	int nRet = 0;
	// надо найти строку strID_RT11 где-то в нулевом секторе.
	size_t s_len = strlen(strID_RT11);
	size_t b_len = BLOCK_SIZE - s_len;
	bool bOk = substrfind(buf, b_len, (uint8_t *)strID_RT11, s_len);
	if (bOk) {
		nRet = 1;
	} else {
		// если не нашли строку первого вида, поищем второго
		s_len = strlen(strID_RT11Q);
		b_len = BLOCK_SIZE - s_len;
		bOk = substrfind(buf, b_len, (uint8_t *) strID_RT11Q, s_len);
		if (bOk) {
			nRet = 1;
		}
	}
	return nRet;
}

#pragma pack(push)
#pragma pack(1)
struct RT11SegmentHdr {
	uint16_t segments_num;          // число сегментов, отведённых под каталог
	uint16_t next_segment;          // номер следующего сегмента каталога, если 0 - то этот сегмент последний
	uint16_t used_segments;         // счётчик сегментов, имеющих записи, есть только в первом сегменте
	uint16_t filerecord_add_bytes;  // число дополнительных БАЙТОВ! в записи о файле в каталоге
	uint16_t file_block;            // номер блока, с которого начинается самый первый файл, описанный в данном сегменте
	RT11SegmentHdr() {
		memset(this, 0, sizeof(RT11SegmentHdr));
	}
	RT11SegmentHdr &operator = (const RT11SegmentHdr &src) {
		memcpy(this, &src, sizeof(RT11SegmentHdr));
		return *this;
	}
	RT11SegmentHdr &operator = (const RT11SegmentHdr *src) {
		memcpy(this, src, sizeof(RT11SegmentHdr));
		return *this;
	}
};
#pragma pack(pop)

/*
Проверим, а не rt11 ли это. выход:
1 - rt11
0 - не rt11
-1 - ошибка чтения файла
*/
inline static int IsRT11(FIL *f, uint8_t *buf) {
	int nRet = 0;
	auto pwBuf = reinterpret_cast<uint16_t *>(buf);
	// проверку будем осуществлять методом эмуляции чтения каталога.
	const unsigned int nImgSize = f_size(f) / BLOCK_SIZE; // размер образа в блоках.
    UINT br;
	// перемещаемся к первому сектору
	if (f_lseek(f, BLOCK_SIZE) != FR_OK && f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
		return -1;
	}
	unsigned int nBeginBlock = pwBuf[0724 / 2]; // получим блок, с которого начинается сектор.
	unsigned int nCurrentSegment = 1;   // номер текущего сегмента
	unsigned int nTotalSegments = 0;    // всего возможных сегментов в каталоге
	unsigned int nUsedSegments = -1;    // число занятых сегментов, не должно совпадать с nCount
	unsigned int nAddBytes = 0;
	unsigned int nCount = 0;            // счётчик пройденных сегментов
	// !!! фикс для АДОС
	if ((nBeginBlock == 0) || (nBeginBlock > 255)) {
		nBeginBlock = 6;
	}
	if (nBeginBlock < nImgSize) {
		while (nCurrentSegment > 0)	{
			int offs = ((nCurrentSegment - 1) * 2 + nBeginBlock);
            if (f_lseek(f, offs / BLOCK_SIZE) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
                // перемещаемся к началу текущего сегмента каталога и читаем
				return -1;
			}
			auto CurrentSegmentHeader = reinterpret_cast<RT11SegmentHdr *>(buf);
			if (nTotalSegments == 0) {
				nTotalSegments = CurrentSegmentHeader->segments_num; // если в первый раз, запомним число сегментов.
				if (nTotalSegments > 31) // если число сегментов больше 31, то не RT-11
				{
					break;
				}
				nUsedSegments = CurrentSegmentHeader->used_segments;
				// если в первом сегменте счётчик использованных сегментов больше общего числа сегментов
				if (nUsedSegments > nTotalSegments) {
					break;
				}
				nAddBytes = CurrentSegmentHeader->filerecord_add_bytes;
				if (nAddBytes > 252) // если число доп слов больше размера сегмента - то это не RT-11
				{
					// т.к. в сегменте обязательно должен быть маркер конца сегмента, то в нём должны умещаться как минимум 2 записи.
					// поэтому 72*7 делим пополам
					break;
				}
			} else {
				// если не в первый - сравним старое кол-во с новым, если не равно - значит не RT-11
				if (nTotalSegments != CurrentSegmentHeader->segments_num) {
					break;
				}
				if (nAddBytes != CurrentSegmentHeader->filerecord_add_bytes) {
					break;
				}
			}
			// если начальный блок в сегменте выходит за пределы образа - то это не RT-11
			if (CurrentSegmentHeader->file_block > nImgSize) {
				break;
			}
			nCurrentSegment = CurrentSegmentHeader->next_segment;
			if (nCurrentSegment > 31) // если номер следующего сегмента больше 31, то не RT-11
			{
				break;
			}
			if (nCurrentSegment > nTotalSegments) // если номер следующего сегмента больше общего кол-ва сегментов, то не RT-11
			{
				break;
			}
			nCount++; // чтобы не зациклиться, считаем пройденные сегменты
			if (nCount > nTotalSegments) // если их было больше, чем общее число сегментов - то это не RT-11
			{
				break;
			}
		}
		// когда выходим из цикла
		// если соблюдаются следующие условия, то очень вероятно, что это всё-таки RT-11
		// третье условие - избыточно
		if ((nCurrentSegment == 0) && (nCount == nUsedSegments) && (nCount <= nTotalSegments)) {
			nRet = 1;
		}
	}
	return nRet;
}

/*
Проверим, а не ксидос ли это. выход:
1 - ксидос
0 - не ксидос
-1 - ошибка чтения
*/
inline static int IsCSIDOS3(FIL *f, uint8_t *buf) {
	auto pwBuf = reinterpret_cast<uint16_t *>(buf);
	// проверим, а не ксидос ли у нас
    UINT br;
	// перемещаемся ко второму сектору
	if (f_lseek(f, 2 * BLOCK_SIZE) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
		if ((pwBuf[8 / 2] == 0123123) || (pwBuf[8 / 2] == 0123124)) {
			// если ксидос или особый ксидос
			if ((pwBuf[4 / 2] == 0123123) && (pwBuf[6 / 2] == 0123123)) // если ксидос3
			{
				return 1;
			}
		}
		// про ксидос других версий ничего неизвестно
		return 0; // вообще не ксидос
	}
	return -1;
}

/*
Проверим, вдруг это ОПТОК. выход:
1 - Опток
0 - не Опток
-1 - ошибка чтения
*/
inline static int IsOPTOK(FIL *f, uint8_t *buf) {
	auto pwBuf = reinterpret_cast<uint16_t *>(buf);
    UINT br;
	// перемещаемся ко второму сектору
	if (f_lseek(f, 2 * BLOCK_SIZE) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
		return -1;
	}
	// первое слово - сигнатура "**", затем 13 нулевых слов.
	uint16_t s = 0;
	for (int i = 0; i < 14; ++i) {
		s += pwBuf[i]; // проверяем
	}
	if (s == 0x2a2a) // если всё так и есть
	{
		return 1;   // то это скорее всего Опток
	}
	return 0;
}

inline static uint16_t CalcCRC(uint8_t *buffer, size_t len) {
	uint32_t crc = 0;
	for (size_t i = 0; i < len; ++i) {
		crc += buffer[i];
		if (crc & 0xFFFF0000) {
			// если случился перенос в 17 разряд (т.е. бит С для word)
			crc &= 0x0000FFFF; // его обнулим
			crc++; // но прибавим к сумме
		}
	}
	return static_cast<uint16_t>(crc & 0xffff);
}

inline static int IsHolography(FIL *f, uint8_t *buf) {
	auto pwBuf = reinterpret_cast<uint16_t *>(buf);
    UINT br;
	// читаем бут сектор
	if (f_lseek(f, 0) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
		return -1;
	}
	// будем проверять по КС, потому что существует всего 2 дискеты с этой, так сказать, ОС
	uint16_t crc = CalcCRC(buf, 0162);
	return (crc == 8384);
}

inline static int IsDaleOS(FIL *f, uint8_t *buf) {
	auto pwBuf = reinterpret_cast<uint16_t *>(buf);
	static uint8_t OSSignature[3] = "Da";
    UINT br;
	// читаем бут сектор
	if (f_lseek(f, 0) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
		return -1;
	}
	if (pwBuf[1] == 0240) {
		// если второе слово у звгрузчика тоже NOP
		// читаем блок 1
		if (f_lseek(f, BLOCK_SIZE) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
			return -1;
		}
		if (pwBuf[0] == *(reinterpret_cast<uint16_t *>(&OSSignature[0]))) {
			// если первое слово второго блока - сигнатура ОС
			// читаем блок 2
			if (f_lseek(f, 2* BLOCK_SIZE) != FR_OK || f_read(f, buf, BLOCK_SIZE, &br) != FR_OK) {
				return -1;
			}
			// если последнее слово второго блока - сигнатура каталога
			if (pwBuf[0776 / 2] == 041125) {
				return 1; // то это оно
			}
		}
	}
	return 0; // иначе - это не оно
}

/* определение типа образа, на выходе номер, соответствующий определённой ОС
    0 - образ не опознан, -1 - ошибка
*/
static void ParseImage(const char* fname, PARSE_RESULT_C& ret) {
	union {
		uint8_t pSector[BLOCK_SIZE];
		uint16_t wSector[BLOCK_SIZE / 2];
	};
	ret.bImageBootable = false;
	strncpy(ret.strName, fname, 256);
	ret.nBaseOffset = 0;
	ret.imageOSType = IMAGE_TYPE::ERROR_NOIMAGE; // предполагаем изначально такое
    FIL fil;
	if (f_open(&fil, fname, FA_READ) != FR_OK) {
		DBGM_PRINT(("Unable to open %s", fname));
		return;
	}
	ret.nImageSize = f_size(&fil); // получим размер файла
    DBGM_PRINT(("%s nImageSize: %d", fname, ret.nImageSize));
	UINT br;
	if (f_read(&fil, pSector, BLOCK_SIZE, &br) == FR_OK) {
		bool bAC1 = false; // условие андос 1 (метка диска)
		bool bAC2 = false; // условие андос 2 (параметры диска)
		// Получим признак системного диска
		if (wSector[0] == 0240 /*NOP*/) {
			if (wSector[1] != 5 /*RESET*/) {
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
			if (pSector[4] == 032 && (wSector[6 / 2] == 256 || wSector[6 / 2] == 512)) {
				// либо расширенный формат
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::AODOS;
			} else if (memcmp(strID_Aodos, reinterpret_cast<char *>(pSector + 4), strlen(strID_Aodos)) == 0) {
				// либо по метке диска
				// аодос 1.77 так не распознаётся. его будем считать просто микродосом. т.к. там нету директорий
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::AODOS;
			} else if (memcmp(strID_Nord, reinterpret_cast<char *>(pSector + 4), strlen(strID_Nord)) == 0) {
				// предполагаем, что это метка для системы
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::NORD;
			} else if (memcmp(strID_Nord_Voland, reinterpret_cast<char *>(pSector + 0xa0), strlen(strID_Nord_Voland)) == 0) {
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
			else if (wSector[0402 / 2] == 051414) {
				ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 0466));
				ret.imageOSType = IMAGE_TYPE::MKDOS;
			}
			// тут надо найти способ как определить прочую экзотику
			else {
				int nRet = AnalyseMicrodos(&fil, pSector);
				switch (nRet) {
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
		) {
			DBGM_PRINT(("FAT12"));
			uint8_t nMediaDescriptor = pSector[025];
			int nBootSize = *(reinterpret_cast<uint16_t *>(pSector + 016)) * (*(reinterpret_cast<uint16_t *>(pSector + 013)));
			uint8_t b[BLOCK_SIZE];
			f_lseek(&fil, nBootSize);
			f_read(&fil, b, BLOCK_SIZE, &br);
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
				} else {
					ret.nPartLen = *(reinterpret_cast<uint16_t *>(pSector + 023));
					ret.imageOSType = IMAGE_TYPE::MSDOS; // тут только MS DOS остался
				}
			} else {
				ret.imageOSType = IMAGE_TYPE::UNKNOWN;
				DBGM_PRINT(("UNKNOWN"));
			}
		}
		// проверка на предположительно возможно НС-ДОС
		else if (wSector[2 / 2] == 012700 && wSector[4 / 2] == 0404 && wSector[6 / 2] == 0104012) {
			DBGM_PRINT(("NCDOS"));
			// нс-дос никак себя не выделяет среди других ос, поэтому будем её определять по коду.
			ret.nPartLen = 1600;
			ret.imageOSType = IMAGE_TYPE::NCDOS;
		} else {
			// дальше сложные проверки
			// выход из функций: 1 - да, это проверяемая ОС
			//                   0 - нет, это какая-то другая ОС
			//                  -1 - ошибка ввода вывода
			// проверим, а не рт11 ли у нас
			int t = IsRT11_old(pSector);
            DBGM_PRINT(("IsRT11_old: %d", t));
			switch (t) {
				case 0:
					t = IsRT11(&fil, pSector);
                    DBGM_PRINT(("IsRT11: %d", t));
					switch (t) {
						case 0:	{
							// нет, не рт11
							// проверим, а не ксидос ли у нас
							t = IsCSIDOS3(&fil, pSector);
                            DBGM_PRINT(("IsCSIDOS3: %d", t));
							switch (t)
							{
								case 0:
									// нет, не ксидос
									// проверим на ОПТОК
									t = IsOPTOK(&fil, pSector);
									DBGM_PRINT(("IsOPTOK: %d", t));
									switch (t)
									{
										case 0:
											// нет, не опток
											// проверим на Holography
											t = IsHolography(&fil, pSector);
											DBGM_PRINT(("IsHolography: %d", t));
											switch (t)
											{
												case 0:
													// нет, не Holography
													// проверим на Dale OS
													t = IsDaleOS(&fil, pSector);
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
	f_close(&fil);
	return;
}

extern "C" bool is_browse_os_supported() {
   return parse_result.imageOSType == IMAGE_TYPE::MKDOS;
}

inline static const char* GetOSName(const IMAGE_TYPE t) {
	switch (t) {
		case IMAGE_TYPE::ERROR_NOIMAGE:
			return { "" };
		case IMAGE_TYPE::ANDOS:
			return { "ANDOS" };
		case IMAGE_TYPE::MKDOS:
			return { "MKDOS" };
		case IMAGE_TYPE::AODOS:
			return { "AODOS" };
		case IMAGE_TYPE::NORD:
			return { "NORD" };
		case IMAGE_TYPE::MIKRODOS:
			return { "MicroDOS" };
		case IMAGE_TYPE::CSIDOS3:
			return { "CSIDOS3" };
		case IMAGE_TYPE::RT11:
			return { "RT-11" };
		case IMAGE_TYPE::NCDOS:
			return { "HCDOS" };
		case IMAGE_TYPE::DXDOS:
			return { "DXDOS" };
		case IMAGE_TYPE::OPTOK:
			return { "OPTOK" };
		case IMAGE_TYPE::HOLOGRAPHY:
			return { "Holografy OS" };
		case IMAGE_TYPE::DALE:
			return { "Dale OS" };
		case IMAGE_TYPE::MSDOS:
			return { "MS-DOS" };
    	default:
			return { "" };
	}
}

extern "C" void detect_os_type(const char* path, char* os_type, size_t sz) {
    try {
        ParseImage(path, parse_result);
		snprintf(os_type, sz, "%d KB %s%s",
		         parse_result.nImageSize >> 10,
				 GetOSName(parse_result.imageOSType),
				 parse_result.bImageBootable ? " [bootable]" : "");
        DBGM_PRINT(("detect_os_type: %s %s", path, os_type));
    } catch(...) {
        DBGM_PRINT(("detect_os_type: %s FAILED", path));
        strncpy(os_type, "DETECT OS TYPE FAILED", sz);
    }
}

#if EXT_DRIVES_MOUNT
extern "C" bool mount_img(const char* path, int curr_dir_num) {
    DBGM_PRINT(("mount_img: %s dir_num: %d", path, curr_dir_num));
    if ( !is_browse_os_supported() ) {
        DBGM_PRINT(("mount_img: %s unsupported file type (resources)", path));
        return false;
    }
    m_cleanup_ext();
	mkdos_review(parse_result, curr_dir_num); // TODO: pass callback
	return true;
}

/*
Класс, где будут все основные методы для работы с образом.
Открытие образа, закрытие,
добавление файлов/директорий (групповое)
удаление файлов/директорий (групповое и рекурсивное)
создание директорий
извлечение файлов и преобразование форматов
*/
enum class ADD_ERROR : int
{
	OK_NOERROR = 0, // нет ошибок
	IMAGE_NOT_OPEN, // файл образа не открыт
	FILE_TOO_LARGE, // файл слишком большой
	USER_CANCEL,    // операция отменена пользователем
	IMAGE_ERROR,    // ошибку смотри в nImageErrorNumber
	NUMBERS
};

enum class IMAGE_ERROR : int
{
	OK_NOERRORS = 0,        // нет ошибок
	NOT_ENOUGHT_MEMORY,     // Недостаточно памяти.
	IMAGE_CANNOT_OPEN,      // Невозможно открыть файл образа
	FILE_CANNOT_CREATE,     // Невозможно создать файл
	IMAGE_NOT_OPEN,         // Файл образа не открыт
	IMAGE_WRITE_PROTECRD,   // Файл образа защищён от записи.
	IMAGE_CANNOT_SEEK,      // Ошибка позиционирования в файле образа
	IMAGE_CANNOT_READ,      // Ошибка чтения файла образа
	IMAGE_CANNOT_WRITE,     // Ошибка записи в файл образа
	FS_CANNOT_CREATE_DIR,   // Невозможно создать директорию
	FS_FILE_NOT_FOUND,      // Файл(запись о файле в каталоге не найдена)
	FS_FORMAT_ERROR,        // ошибка в формате файловой системы
	FS_FILE_EXIST,          // Файл с таким именем уже существует.
	FS_DIR_EXIST,           // Директория с таким именем уже существует.
	FS_DIR_NOT_EXIST,       // Директории с таким именем не существует.
	FS_CAT_FULL,            // Каталог заполнен.
	FS_DISK_FULL,           // Диск заполнен.
	FS_DISK_NEED_SQEEZE,    // Диск сильно фрагментирован, нужно провести сквизирование.
	FS_DIR_NOT_EMPTY,       // Директория не пуста.
	FS_STRUCT_ERR,          // Нарушена структура каталога.
	FS_IS_NOT_DIR,          // Это не директория. - попытка подсунуть файл функции change dir
	FS_IS_NOT_FILE,         // Это не файл. - попытка подсунуть не файл функции, работающей с файлами
	FS_NOT_SUPPORT_DIRS,    // Файловая система не поддерживает директории.
	FS_FILE_PROTECTED,      // Файл защищён от удаления
	FS_DIR_DUPLICATE,       // встретилось дублирование номеров директорий
	FS_DIRNUM_FULL,         // закончились номера для директорий
    IMAGE_TYPE_IS_UNSUPPORTED,
	NUMBERS
};

struct ADDOP_RESULT
{
	bool            bFatal;     // флаг необходимости прервать работу.
	ADD_ERROR       nError;     // номер ошибки в результате добавления объекта в образ
	IMAGE_ERROR     nImageErrorNumber; // номер ошибки в результате операций с образом
	BKDirDataItem   afr;        // экземпляр абстрактной записи, которая вызвала ошибку.
	// Она нам нужна будет для последующей обработки ошибок
	ADDOP_RESULT()
		: bFatal(false)
		, nError(ADD_ERROR::OK_NOERROR)
		, nImageErrorNumber(IMAGE_ERROR::OK_NOERRORS)
	{
		afr.clear();
	}
};

bool MkDirCreateDir(BKDirDataItem *pFR);
inline static bool CreateDir(BKDirDataItem *pFR) {
	if (is_browse_os_supported()) {
		return false;
	}
	return MkDirCreateDir(pFR);
}

// возвращаем коды состояния и ошибок вызывающей функции. Она должна заниматься обработкой
// разных возникающих ситуаций
// сюда принимаем структуру sendfiledata и делаем из неё AbstractFileRecord
// bExistDir - флаг, когда мы пытаемся создать уже существующую директорию, - игнорировать ошибку
ADDOP_RESULT AddObject(const char* pFileName, bool folder, bool bExistDir = false) {
	ADDOP_RESULT ret;
	if (is_browse_os_supported()) {
		ret.bFatal = IMAGE_ERROR::IMAGE_TYPE_IS_UNSUPPORTED;
		ret.nError = ADD_ERROR::IMAGE_ERROR;
		return ret;
	}
/*	if (!m_pFloppyImage)
	{
		// файл образа не открыт
		ret.bFatal = true;
		ret.nError = ADD_ERROR::IMAGE_NOT_OPEN;
		return ret;
	}
*/
	BKDirDataItem AFR;
	strncpy(AFR.strName, pFileName, 16);

	// мы не можем сформировать данные:
	// nDirBelong - потому, что это внутренние данные образа
	// nBlkSize - потому, что это данные, зависящие от формата образа
	// nStartBlock - потому, что эти данные ещё неизвестны.
	if (folder)	{
		AFR.nAttr |= FR_ATTR::DIRECTORY;
		AFR.nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;
		if (CreateDir(&AFR) || bExistDir) {
			if (!ChangeDir(&AFR)) {
				// ошибка изменения директории:
				// IMAGE_ERROR::FS_NOT_SUPPORT_DIRS
				// IMAGE_ERROR::IS_NOT_DIR
				ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
				// обрабатывать теперь
				ret.nError = ADD_ERROR::IMAGE_ERROR;
				switch (ret.nImageErrorNumber) {
					case IMAGE_ERROR::FS_NOT_SUPPORT_DIRS:
						ret.bFatal = false;
						break;
					case IMAGE_ERROR::FS_IS_NOT_DIR:
					default:
						ret.bFatal = true;
						break;
				}
			}
		} else {
			// ошибка создания директории:
			// IMAGE_ERROR::FS_NOT_SUPPORT_DIRS - два варианта: игнорировать, остановиться
			// IMAGE_ERROR::CANNOT_WRITE_FILE
			// IMAGE_ERROR::FS_DIR_EXIST - такая директория уже существует, два варианта: игнорировать, остановиться
			ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
			// обрабатывать теперь
			ret.nError = ADD_ERROR::IMAGE_ERROR;
			switch (ret.nImageErrorNumber) {
				case IMAGE_ERROR::FS_NOT_SUPPORT_DIRS:
				case IMAGE_ERROR::FS_DIR_EXIST:
					ret.bFatal = false;
					break;
				default:
					ret.bFatal = true;
					break;
			}
		}
	} else {
		AFR.nRecType = BKDirDataItem::RECORD_TYPE::FILE;
		AFR.nSize = fs::file_size(findFile);
		if (AFR.nSize > m_pFloppyImage->GetImageFreeSpace()) {
			// файл слишком большой
			ret.bFatal = true;
			ret.nError = ADD_ERROR::FILE_TOO_LARGE;
		} else {
			const auto nBufferSize = m_pFloppyImage->EvenSizeByBlock(AFR.nSize);
			auto Buffer = std::vector<uint8_t>(nBufferSize);
			const auto pBuffer = Buffer.data();
			if (pBuffer) {
				memset(pBuffer, 0, nBufferSize);
				AnalyseFileStruct AFS;
				if ((AFS.file = _tfopen(findFile.c_str(), _T("rb"))) != nullptr) {
					AFS.strName = findFile.stem();
					AFS.strExt = findFile.extension();
					AFS.nAddr = 01000;
					AFS.nLen = AFR.nSize;
					AFS.nCRC = 0;
					if (imgUtil::AnalyseImportFile(&AFS)) {
						// если обнаружился формат .bin
						// если в заголовке бин было оригинальное имя файла
						if (AFS.OrigName[0]) {
							// то восстановим оригинальное
							AFR.strName = strUtil::trim(imgUtil::BKToUNICODE(AFS.OrigName, 16, m_pFloppyImage->m_pKoi8tbl));
						} else {
							AFR.strName = AFS.strName + AFS.strExt;
						}
					}
					AFR.nAddress = AFS.nAddr;
					AFR.nSize = AFS.nLen;
					fread(pBuffer, 1, AFR.nSize, AFS.file);
					if (AFS.bIsCRC) {
						fread(&AFS.nCRC, 1, 2, AFS.file);
						if (AFS.nCRC != imgUtil::CalcCRC(pBuffer, AFR.nSize)) {
							TRACE("CRC Mismatch!\n");
						}
					} else {
						AFS.nCRC = imgUtil::CalcCRC(pBuffer, AFR.nSize);
					}
					fclose(AFS.file);
					constexpr int MAX_SQUEEZE_ITERATIONS = 3;
					int nSqIter = MAX_SQUEEZE_ITERATIONS;
					bool bNeedSqueeze = false;
l_sque_retries:
					if (!m_pFloppyImage->WriteFile(&AFR, pBuffer, bNeedSqueeze)) {
						// ошибка записи файла:
						// IMAGE_ERROR::CANNOT_WRITE_FILE
						// IMAGE_ERROR::FS_DISK_FULL
						// IMAGE_ERROR::FS_FILE_EXIST - Файл существует. два варианта: остановиться, удалить старый и перезаписать новый файл
						// IMAGE_ERROR::FS_STRUCT_ERR
						// IMAGE_ERROR::FS_CAT_FULL
						// IMAGE_ERROR::FS_DISK_NEED_SQEEZE - нужно сделать сквизирование, но от него отказались
						// и прочие ошибки позиционирования и записи
						ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
						// предполагаем ошибку безусловно фатальную
						ret.bFatal = true;
						// если нужно делать сквизирование
						if (bNeedSqueeze) {
							// если попытки не кончились ещё
							if (nSqIter > 0) {
								// выведем сообщение
								CString str;
								str.Format(_T("Попытка %d из %d.\n"), nSqIter, MAX_SQUEEZE_ITERATIONS);
								str += (m_pFloppyImage->GetImgOSType() == IMAGE_TYPE::OPTOK) ?
								       _T("Попробовать выполнить уплотнение?\nПоможет только если есть удалённые файлы.") :
								       _T("Диск сильно фрагментирован. Выполнить сквизирование?");
								LRESULT definite = AfxGetMainWnd()->SendMessage(WM_SEND_MESSAGEBOX, WPARAM(MB_YESNO | MB_ICONINFORMATION), reinterpret_cast<LPARAM>(str.GetString()));
								// если согласны
								nSqIter--;
								if (definite == IDYES) {
									goto l_sque_retries;
								}
								// если отказ, то сразу выход с фатальной ошибкой - диск полон
							} else {
								// итерации кончились, но так ничего и не получилось, то фатальная ошибка
								if (ret.nImageErrorNumber == IMAGE_ERROR::FS_DISK_NEED_SQEEZE) {
									ret.nImageErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
								}
							}
							// у сквизирования есть 2 причины: в каталоге нет места для записи, надо делать уплотнение
							// или место есть, но нету дырки нужного размера, тоже надо делать уплотнение
							// если и после этого записать ничего не получится, то диск точно полон.
							// обычно это происходит в случае, когда каталог заполнен и ничего не помогает.
						} else if (ret.nImageErrorNumber == IMAGE_ERROR::FS_FILE_EXIST) {
							// если файл существует, то ошибка не фатальная, нужно спросить
							// перезаписывать файл или нет
							ret.bFatal = false;
						}
						ret.nError = ADD_ERROR::IMAGE_ERROR;
					}
				}
			} else {
				// недостаточно памяти
				ret.bFatal = true;
				ret.nError = ADD_ERROR::IMAGE_ERROR;
				ret.nImageErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
			}
		}
	}
	ret.afr = AFR;
	return ret;
}
#endif
