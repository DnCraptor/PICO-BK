#include <stdio.h>
#include <stdint.h>

uint8_t ROM [6 * 0x2000];

int Load (char *pName, uint8_t *pData, size_t Size)
{
    FILE *pFile = pFile = fopen (pName,"rb");
    int  Res;

    if (pFile == NULL)
    {
        printf ("Can not open input file \"%s\"\n", pName);
        return 1;
    }

    Res = fread (pData, Size, 1, pFile);

    fclose (pFile);

    if (Res == 1) return 0;

    printf ("Can not read input file \"%s\"\n", pName);
    return 2;
}

int main (void)
{
    FILE *pFile;
    int  Res;

    if (Load ("bk11m_328_basic2.rom", &ROM [0 * 0x2000], sizeof (uint8_t) * 8192)) return 1;
    if (Load ("bk11m_329_basic3.rom", &ROM [1 * 0x2000], sizeof (uint8_t) * 8192)) return 1;
    if (Load ("bk11m_327_basic1.rom", &ROM [2 * 0x2000], sizeof (uint8_t) * 8192)) return 1;
    if (Load ("bk11m_325_ext.rom"   , &ROM [3 * 0x2000], sizeof (uint8_t) * 8192)) return 1;
    if (Load ("bk11m_324_bos.rom"   , &ROM [4 * 0x2000], sizeof (uint8_t) * 8192)) return 1;
    if (Load ("bk11m_330_mstd.rom"  , &ROM [5 * 0x2000], sizeof (uint8_t) * 8192)) return 1;

    pFile = fopen ("rom.bin", "wb");

    if (pFile == NULL)
    {
        printf ("Can not open output file \"rom.bin\"\n");
        return 2;
    }

    Res = fwrite (ROM, sizeof (ROM), 1, pFile);

    fclose (pFile);

    if (Res == 1) return 0;

    printf ("Can not write output file \"rom.bin\"\n");
    return 3;
}
