#include "UTF8_KOI8.h"

static const char UTF8_To_KOI8_Tab [32] =
{
    0x01,  // �  0x410
    0x02,  // �  0x411
    0x17,  // �  0x412
    0x07,  // �  0x413
    0x04,  // �  0x414
    0x05,  // �  0x415
    0x16,  // �  0x416
    0x1A,  // �  0x417
    0x09,  // �  0x418
    0x0A,  // �  0x419
    0x0B,  // �  0x41A
    0x0C,  // �  0x41B
    0x0D,  // �  0x41C
    0x0E,  // �  0x41D
    0x0F,  // �  0x41E
    0x10,  // �  0x41F
    0x12,  // �  0x420
    0x13,  // �  0x421
    0x14,  // �  0x422
    0x15,  // �  0x423
    0x06,  // �  0x424
    0x08,  // �  0x425
    0x03,  // �  0x426
    0x1E,  // �  0x427
    0x1B,  // �  0x428
    0x1D,  // �  0x429
    0x1F,  // �  0x42A
    0x19,  // �  0x42B
    0x18,  // �  0x42C
    0x1C,  // �  0x42D
    0x00,  // �  0x42E
    0x11   // �  0x42F
};

static char KOI8_To_UTF8_Tab [32] =
{
    0x1E,  // �  0x00  0x42E
    0x00,  // �  0x01  0x410
    0x01,  // �  0x02  0x411
    0x16,  // �  0x03  0x426
    0x04,  // �  0x04  0x414
    0x05,  // �  0x05  0x415
    0x14,  // �  0x06  0x424
    0x03,  // �  0x07  0x413
    0x15,  // �  0x08  0x425
    0x08,  // �  0x09  0x418
    0x09,  // �  0x0A  0x419
    0x0A,  // �  0x0B  0x41A
    0x0B,  // �  0x0C  0x41B
    0x0C,  // �  0x0D  0x41C
    0x0D,  // �  0x0E  0x41D
    0x0E,  // �  0x0F  0x41E
    0x0F,  // �  0x10  0x41F
    0x1F,  // �  0x11  0x42F
    0x10,  // �  0x12  0x420
    0x11,  // �  0x13  0x421
    0x12,  // �  0x14  0x422
    0x13,  // �  0x15  0x423
    0x06,  // �  0x16  0x416
    0x02,  // �  0x17  0x412
    0x1C,  // �  0x18  0x42C
    0x1B,  // �  0x19  0x42B
    0x07,  // �  0x1A  0x417
    0x18,  // �  0x1B  0x428
    0x1D,  // �  0x1C  0x42D
    0x19,  // �  0x1D  0x429
    0x17,  // �  0x1E  0x427
    0x1A   // �  0x1F  0x42A
};

static unsigned char CharToHex (unsigned char Char)
{
    if (Char >= '0' && Char <= '9') return (Char -  '0'        );
    if (Char >= 'a' && Char <= 'f') return (Char - ('a' - 0x0A));
    if (Char >= 'A' && Char <= 'F') return (Char - ('A' - 0x0A));

    return 0x10;
}

void URL_UTF8_To_KOI8 (char *pKOI8, const char *pURL)
{
    unsigned char  PrevChar = 0;
    unsigned char  Char;

    while ((Char = *pURL++) != 0)
    {
        if (Char == ' ') break;

        if (Char == '%')
        {
            unsigned char Char1 = CharToHex (pURL [0]);

            if (Char1 <= 0x0F)
            {
                unsigned char Char2 = CharToHex (pURL [1]);

                if (Char2 <= 0x0F)
                {
                    pURL += 2;
                    Char = (Char1 << 4) | Char2;
                }
            }
        }

        if (Char < 128)
        {
            *pKOI8++ = Char;
        }
        else if (((PrevChar & 0xE0) == 0xC0) && ((Char & 0xC0) == 0x80))
        {
            unsigned int Char16 = ((unsigned int) (PrevChar & 0x1F) << 6) | (Char & 0x3F);

            if ((0x0410 <= Char16) && (Char16 <= 0x044F))
            {
                Char = UTF8_To_KOI8_Tab [(Char16 - 0x410) & 0x1F] + 0xC0;

                if (Char16 < 0x430) Char += 0x20;

                *pKOI8++ = Char;
            }
            else if (Char16 == 0x0401) *pKOI8++ = 0xB3; // �
            else if (Char16 == 0x0451) *pKOI8++ = 0xA3; // �

            PrevChar = 0;

            continue;
        }

        PrevChar = Char;
    }

    *pKOI8++ = 0;
}

void KOI8_To_UTF8 (char *pUTF8, const char *pKOI8)
{
    unsigned char Char;

    while ((Char = *pKOI8++) != 0)
    {
        unsigned int Char16;

        if      (Char <   128) {*pUTF8++ = Char; continue;}
        if      (Char >= 0xC0) {Char16 = KOI8_To_UTF8_Tab [Char & 0x1F] + 0x410; if (Char < 0xE0) Char16 += 0x20;}
        else if (Char == 0xB3)  Char16 = 0x0401; // �
        else if (Char == 0xA3)  Char16 = 0x0451; // �
        else                    continue;

        *pUTF8++ = (Char16 >>   6) | 0xC0;
        *pUTF8++ = (Char16 & 0x3F) | 0x80;
    }

    *pUTF8++ = 0;
}
