#include "pch.h"
#include "ScreenColors_Shared.h"

const DWORD  g_pColorPalettes_std[16][4] =
{
	//                                                            Palette#     01           10          11
	Color_Black, Color_Blue, Color_Green, Color_Red,                // 00    синий   |   зелёный  |  красный
	Color_Black, Color_Yellow, Color_Magenta, Color_Red,            // 01   жёлтый   |  сиреневый |  красный
	Color_Black, Color_Cyan, Color_Blue, Color_Magenta,             // 02   голубой  |    синий   | сиреневый
	Color_Black, Color_Green, Color_Cyan, Color_Yellow,             // 03   зелёный  |   голубой  |  жёлтый
	Color_Black, Color_Magenta, Color_Cyan, Color_White,            // 04  сиреневый |   голубой  |   белый
	Color_Black, Color_White, Color_White, Color_White,             // 05    белый   |    белый   |   белый
	Color_Black, Color_DarkRed, Color_RedBrown, Color_Red,          // 06  тёмн-красн| красн-корич|  красный    !
	Color_Black, Color_Salatovyi, Color_LightGreen, Color_Yellow,   // 07  салатовый | светл-зелён|  жёлтый     !
	Color_Black, Color_Violet, Color_VioletBlue, Color_Magenta,     // 08  фиолетовый| фиол-синий | сиреневый   !
	Color_Black, Color_LightGreen, Color_VioletBlue, Color_RedBrown,// 09 светл-зелён| фиол-синий |красн-корич  !
	Color_Black, Color_Salatovyi, Color_Violet, Color_DarkRed,      // 10  салатовый | фиолетовый |тёмн-красный !
	Color_Black, Color_Cyan, Color_Yellow, Color_Red,               // 11   голубой  |   жёлтый   |  красный
	Color_Black, Color_Red, Color_Green, Color_Cyan,                // 12   красный  |   зелёный  |  голубой
	Color_Black, Color_Cyan, Color_Yellow, Color_White,             // 13   голубой  |   жёлтый   |   белый
	Color_Black, Color_Yellow, Color_Green, Color_White,            // 14   жёлтый   |   зелёный  |   белый
	Color_Black, Color_Cyan, Color_Green, Color_White               // 15   голубой  |   зелёный  |   белый
};

// 0,1,2,3
const DWORD  g_pMonochromePalette_std[2][4] =
{
	Color_Black, Color_White, Color_Black, Color_White,
	Color_Black, Color_Black, Color_White, Color_White
};

const DWORD  g_pAdaptMonochromePalette_std[8][4] =
{
	// Gray
	Color_Black, 0xff6B6B6B, 0xffa9a9a9, Color_White,
	Color_Black, 0xff6B6B6B, 0xffa9a9a9, Color_White,
	// Green
	0xff111b14, 0xff2e8b57, 0xff48a36d, Color_Green /*0xff9ac5a8*/,
	0xff111b14, 0xff2e8b57, 0xff48a36d, Color_Green /*0xff9ac5a8*/,
	// Blue
	0xff03000F, 0xff3300CC, 0xff3A00EA, Color_Blue,
	0xff03000F, 0xff3300CC, 0xff3A00EA, Color_Blue,
	// Yellow
	0xff0A0700, 0xffB57E00, 0xffFFAE00, Color_Yellow,
	0xff0A0700, 0xffB57E00, 0xffFFAE00, Color_Yellow
};



DWORD g_pColorPalettes[16][4] = { 0, };
DWORD g_pMonochromePalette[2][4] = { 0, };
DWORD g_pAdaptMonochromePalette[2][4] = { 0, };

CString ColorToStr(DWORD col)
{
#pragma warning(disable:4996)
	CString strCol;
	LPTSTR buf = strCol.GetBufferSetLength(16);
	snprintf(buf, 16, _T("%06x"), (col & 0xffffff));
	strCol.ReleaseBuffer();
	return _T("#") + strCol;
}

// вход: массив из 4 слов
// выход: строки типа #rrggbb,#rrggbb,#rrggbb,#rrggbb
CString PaletteToStr(const DWORD *pal)
{
	CString strOut;

	for (int i = 0; i < 4; ++i)
	{
		if (i)
		{
			strOut += _T(",");
		}

		strOut += ColorToStr(pal[i]);
	}

	return strOut;
}

// вход: strPal - строка типа #rrggbb,#rrggbb,#rrggbb,#rrggbb
// pal - массив из 4 слов
// выход: заполненный массив, сколько определится, столько и заполняется, но не более 4
void StrToPalette(CString &strPal, DWORD *pal)
{
	strPal.Trim();
	int delim = 0;

	for (int n = 0; n < 4; ++n) // количество принимаемых значений
	{
		int sharp = strPal.Find(_T('#'), delim); // поищем начало числа

		if (sharp < 0)  // если не нашлось
		{
			break;      // значит всё, нечего больше определять
		}

		delim = strPal.Find(_T(','), delim + 1); // поищем конец

		if (delim <= 0) // если не нашлось - то конец это конец строки
		{
			delim = strPal.GetLength();
		}

		sharp++; // сдвинемся с позиции '#', чтоб его не захватывать
		CString strNum = strPal.Mid(sharp, delim - sharp).Trim(); // выделим нужный участок
		pal[n] = 0xff000000; // начальное значение - 0

		if (!strNum.IsEmpty()) // если что-то было
		{
			TCHAR *p;
			pal[n] |= (_tcstoul(strNum.GetBuffer(), &p, 16) & 0xffffff); //преобразуем в число
		}

		if (delim == strPal.GetLength()) // если дошли до конца строки
		{
			break;  // то выходим из цикла
		}
	}
}

