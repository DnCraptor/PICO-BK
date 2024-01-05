#pragma once

/*
цвета               R G B
по таблице Kisser'a, по его сведениям, красный это 100%, тёмно-красный - 33% красного, красно-коричневый - 66% красного
красный             ff0000
зелёный             00ff00
синий               0000ff
сиреневый           ff00ff - magenta
жёлтый              ffff00
голубой             00ffff - cyan
белый               ffffff
тёмно-красный       550000
красно-коричневый   aa0000
салатовый           55ff00
светло-зелёный      aaff00
фиолетовый          5500ff
фиолетово-синий     aa00ff
это цвета, которые имеют цифровое значение исходя из своей логики.
т.е. красно-коричневый на самом деле светлее тёмно-красного. Но на БК11 с палитрами 06-10
какая-то неразбериха. Цвета 01 и 10 поменяны местами, поэтому в дефинициях ниже значения тоже поменяны местами.
фиолетовый  5500ff выглядит слишком синим, поэтому ему задано значение 7000ff.
тёмно-красный 550000 слишком тёмный, поэтому ему задано значение 700000
старшие 8 бит - альфа-канал, на всякий случай
*/

constexpr auto Color_Black      = 0xff000000;
constexpr auto Color_Red        = 0xffff0000;
constexpr auto Color_Green      = 0xff00ff00;
constexpr auto Color_Blue       = 0xff0000ff;
constexpr auto Color_Yellow     = 0xffffff00;
constexpr auto Color_Cyan       = 0xff00ffff;
constexpr auto Color_Magenta    = 0xffff00ff;
constexpr auto Color_DarkRed    = 0xffaa0000;
constexpr auto Color_RedBrown   = 0xff700000;
constexpr auto Color_Violet     = 0xffaa00ff;
constexpr auto Color_VioletBlue = 0xff7000ff;
constexpr auto Color_LightGreen = 0xff55ff00;
constexpr auto Color_Salatovyi  = 0xffaaff00;
constexpr auto Color_White      = 0xffffffff;

// константные стандартные палитры
extern const DWORD  g_pColorPalettes_std[16][4];
// 0,1,2,3
extern const DWORD  g_pMonochromePalette_std[2][4];
extern const DWORD  g_pAdaptMonochromePalette_std[8][4];

// переменные текущие палитры
extern DWORD        g_pColorPalettes[16][4];
extern DWORD        g_pMonochromePalette[2][4];
extern DWORD        g_pAdaptMonochromePalette[2][4];

CString ColorToStr(DWORD col);
CString PaletteToStr(const DWORD *pal);
void StrToPalette(CString &strPal, DWORD *pal);

