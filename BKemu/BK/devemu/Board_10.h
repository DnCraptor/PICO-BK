// Board_10.h: interface for the CMotherBoard_10 class.
//

#pragma once

#include "Board.h"

class CMotherBoard_10 : public CMotherBoard
{
	protected:
		// Initialize memory
		virtual bool        InitMemoryModules() override;

	public:
		CMotherBoard_10();
		virtual ~CMotherBoard_10() override;

		virtual MSF_CONF    GetConfiguration() override;

};

/*
Организация массива памяти (делается один большой массив)
32кб: ОЗУ
000000--банк 0 ---страница 0 --- 0
010000--банк 1
020000--банк 2
030000--банк 3
040000--банк 4 ---страница 1 --- 16384
050000--банк 5
060000--банк 6
070000--банк 7
32кб: ПЗУ
100000--банк 8 ---страница 2 --- 32768 - монитор MONIT10.ROM
110000--банк 9
120000--банк 10------------------------- фокал Focal84.rom
130000--банк 11
140000--банк 12---страница 3 --- 49152 - дырка
150000--банк 13
160000--банк 14------------------------- тесты мстд Tests.rom
170000--банк 15
*/
