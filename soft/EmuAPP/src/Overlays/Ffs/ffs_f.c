#include <stdint.h>
#include <string.h>

#include "AnyMem.h"
#include "ffs.h"
#include "spi_flash.h"
#include "crc8.h"

#include "ffs_f.h"
#include "spi_flash_f.h"

#include "../FfsUi/crc8_fu.h"
#include "../FfsUi/str_fu.h"
#include "../FfsUi/ffs_fu.h"

#define AT_OVL __attribute__((section(".ovl3_f.text")))

//FILE fat[FAT_SIZE];

//uint32_t ffs_PageBuf [0x1000 / 4];

static void AT_OVL f_read (uint32_t pos, uint8_t *data, int size)
{
    spi_flash_Read (FFS_AT + pos, (uint32_t *) data, size);
}


static void AT_OVL f_write (uint32_t pos, const uint8_t *data, int size)
{
    spi_flash_Write (FFS_AT + pos, (const uint32_t *) data, size);
}


static void AT_OVL f_erase (uint32_t pos)
{
    spi_flash_EraseSector ((FFS_AT + pos) / 4096);
}


void AT_OVL ffs_cvt_name(const char *name, char *tmp)
{
    uint8_t i=0;
    uint8_t Size=0;
    char    Char;
    
    while ((i<16) && ((Char = *name++)!=0))
    {
        if (((Char >= 0x7F) && (Char <  0xC0)) ||
            ( Char <  0x20                   ) || 
            ( Char == '\"'                   ) ||
            ( Char ==  '*'                   ) ||
            ( Char == '\\'                   ) ||
            ( Char ==  '/'                   ) ||
            ( Char ==  ':'                   ) ||
            ( Char ==  '<'                   ) ||
            ( Char ==  '>'                   ) ||
            ( Char ==  '?'                   ) ||
            ( Char ==  '|'                   )    ) continue;

        tmp [i++] = Char;

        if (Char != ' ') Size = i;
    }

    i = Size;

    while (i<16) tmp[i++]=0;
}

uint32_t AT_OVL ffs_image_at(void)
{
    return FFS_AT;
}

uint32_t AT_OVL ffs_image_size(void)
{
    return f_size*4096 + FFS_DATA_FIRST_ADR;
}

uint32_t AT_OVL ffs_size(void)
{
    return f_size*4096;
}

uint32_t AT_OVL ffs_free(void)
{
    static const uint8_t n_bits[16] =
    {
    0, 1, 1, 2, 1, 2, 2, 3, 
    1, 2, 2, 3, 2, 3, 3, 4
    };
    uint32_t size=0;
    uint16_t i;
    for (i=0; i<f_size/8; i++)
    {
    size+=n_bits[free_sect[i] >> 4] + n_bits[free_sect[i] & 0x0f];
    }
    return size * 4096;
}

void AT_OVL ffs_read (uint16_t n, uint16_t offs, uint8_t *data, uint16_t size)
{
    ffs_GetFile (n);
    f_read (FFS_DATA_FIRST_ADR + ffs_File.page*4096 + offs, data, size);
}

int16_t AT_OVL ffs_create (const char *fname, uint8_t type, uint16_t size)
{
    uint16_t n;
    
    // ���� ��������� ������ � FAT
    for (n = 0; n < FAT_SIZE; n++)
    {
        ffs_GetFile (n);
        if (ffs_File.type == TYPE_FREE) break;
    }

    if (n >= FAT_SIZE) return -1; // ��� �������

    // ���� ��������� �����
    uint16_t s_count = (size + 4095) / 4096;  // �������� ���-�� ��������
    uint16_t s       = 0;

    while (s < f_size)
    {
        // ���������� ������� �������
        while ((s < f_size) && (!(free_sect [s >> 3] & (1 << (s & 0x07))))) s++;
        if (s >= f_size) break;
        
        // ������� ������ ������� �����
        uint16_t begin = s++; // ���������� ������ ������� �����
        while ((s < f_size) && (s - begin < s_count) && (free_sect [s >> 3] & (1 << (s & 0x07)))) s++;
        if (s - begin < s_count) continue;    // ���� ������� ��������
        // ���������� ���� ������
        s = begin;
        break;
    }

    if (s >= f_size) return -1; // ��� �����
    
    // ��������� ������ FAT
    ffs_cvt_name (fname, ffs_File.name);
    ffs_File.page = s;
    ffs_File.size = size;
    ffs_File.type = type;
    ffs_File.crc8 = CRC8 (CRC8_INIT, (uint8_t*) &ffs_File, sizeof (FILE) - 1);
    
    // ���������� FAT
    f_write (n * sizeof (FILE), (uint8_t*) &ffs_File, sizeof (FILE));
    
    // �������� ������� ��� �������
    while (s_count--)
    {
        free_sect [s >> 3] &= ~(1 << (s & 0x07));
        s++;
    }
    
    return n;
}

void AT_OVL ffs_writeData (uint16_t n, uint16_t offs, const uint8_t *data, uint16_t size)
{
    // ���������� � ��������������� ���������
    size = (size + 3) & ~0x03;

    while (size > 0)
    {
        uint32_t addr;

        // �������� �����
        ffs_GetFile (n);
        addr = FFS_DATA_FIRST_ADR + ffs_File.page * 4096 + offs;
        
        // ������� ��������, ���� ����
        if ((addr & 4095) == 0) f_erase (addr);
        
        // �������� ������ ������ (�� ������ ���� �� ����� ��������)
        int s = 4096 - (addr & 4095); // ������� �������� �� ����� ��������
        if (s > size) s = size;
        
        // ����������
        f_write (addr, data, s);
        offs += s;
        data += s;
        size -= s;
    }
}

uint_fast8_t AT_OVL ffs_write(const char *fname, uint8_t type, const uint8_t *data, uint16_t size)
{
    // ������� ����
    int16_t n=ffs_create(fname, type, size);
    if (n < 0) return 0;
    
    // ���������� ������
    ffs_writeData(n, 0, data, size);
    
    return 1;
}

int16_t AT_OVL ffs_find (const char *fname)
{
    uint16_t n;
    char tmp [16];
    
    ffs_cvt_name (fname, tmp);
    
    for (n = 0; n < FAT_SIZE; n++)
    {
        int Count;

        ffs_GetFile (n);

        if ((ffs_File.type == TYPE_REMOVED) || (ffs_File.type == TYPE_FREE)) continue;

        for (Count = 0; Count < 16; Count++)
        {
            if (to_upper (ffs_File.name [Count]) != to_upper (tmp [Count])) break;
        }

        if (Count >= 16) return n;
    }
    
    return -1;
}

uint32_t AT_OVL ffs_flash_addr(uint16_t n)
{
    ffs_GetFile (n);

    return FFS_AT + FFS_DATA_FIRST_ADR + ffs_File.page * 4096;
}

void AT_OVL ffs_remove(uint16_t n)
{
    ffs_GetFile (n);
    // �������� ������ ��� �������
    ffs_File.type = TYPE_REMOVED;
    
    // ���������� FAT
    f_write (n * sizeof (FILE), (uint8_t*) &ffs_File, sizeof (FILE));
    
    // ������� ������� �������
    uint16_t s_count = (ffs_File.size + 4095) / 4096;   // �������� ���-�� ��������
    uint16_t s       =  ffs_File.page;

    while (s_count--)
    {
        free_sect [s >> 3] |= (1 << (s & 0x07));
        s++;
    }
}

int16_t AT_OVL ffs_rename (uint16_t old_n, const char *fname)
{
    uint16_t n;
    
    // ���� ��������� ������ � FAT
    for (n = 0; n < FAT_SIZE; n++)
    {
        ffs_GetFile (n);
        if (ffs_File.type == TYPE_FREE) break;
    }

    if (n >= FAT_SIZE) return -1; // ��� �������
    
    // ��������� ����� ������ FAT
    ffs_GetFile (old_n);
    ffs_cvt_name (fname, ffs_File.name);
    ffs_File.crc8 = CRC8 (CRC8_INIT, (uint8_t*) &ffs_File, sizeof (FILE) - 1);
    // ���������� ����� FAT
    f_write (n * sizeof (FILE), (uint8_t*) &ffs_File, sizeof (FILE));
    
    // �������� ������ ������ ��� �������
    ffs_File.type = TYPE_REMOVED;
    
    f_write (old_n * sizeof (FILE), (uint8_t*) &ffs_File, sizeof (FILE));
    
    return n;
}
