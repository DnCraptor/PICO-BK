#include "ffs.h"

#ifdef __ets__
    #include "ets.h"
    #include "spi_flash.h"
#else
    #include <osapi.h>
    #include <user_interface.h>
#endif
#include "crc8.h"
#include "str.h"
#include "UTF8_KOI8.h"


FILE fat[FAT_SIZE];
static uint8_t free_sect[128];	// �������� 4��
static uint32_t f_size;

#define DATA_FIRST_PAGE ((sizeof (fat) + 0xFFFUL) / 0x1000UL)
#define DATA_FIRST_ADR  (DATA_FIRST_PAGE * 0x1000UL)

#ifdef __ets__
    typedef unsigned int	uint32;
    #define spi_flash_read(addr, buf, size)	SPIRead(addr, buf, size)
    #define spi_flash_write(addr, buf, size)	SPIWrite(addr, buf, size)
    #define spi_flash_erase_sector(addr)	do { spi_flash_unlock(); SPIEraseSector(addr); } while(0)
#else
    #define ets_printf os_printf
#endif


#define FFS_AT		0x80000


static void f_init(void)
{
    struct rom_header
    {
        uint8_t magic;
        uint8_t count;
        uint8_t flags1;
        uint8_t flags2;
    } hdr;
    
    // ������ ���������
    spi_flash_read(0, (uint32*)&hdr, sizeof(hdr));
    
    // ���������� ������ �����
    switch (hdr.flags2 >> 4)
    {
	case 3:
	    // 16 ����
	    f_size=2048*1024 - FFS_AT;
	    break;
	
	case 4:
	    // 32 ����
	    f_size=4096*1024 - FFS_AT;
	    break;
	
	case 2:
	default:
	    // 8 ����
	    f_size=1024*1024 - FFS_AT;
	    break;
    }
}


static void f_read(uint32_t pos, uint8_t *data, int size)
{
    ets_printf("f_read(0x%05lX, %d)\n", (unsigned long) pos, size);
    spi_flash_read(FFS_AT+pos, (uint32*)data, size);
}


static void f_write(uint32_t pos, const uint8_t *data, int size)
{
    ets_printf("f_write(0x%05lX, %d)\n", (unsigned long) pos, size);
    spi_flash_write(FFS_AT+pos, (const uint32*)data, size);
}


static void f_erase(uint32_t pos)
{
    ets_printf("f_erase(0x%05lX)\n", (unsigned long) pos);
    spi_flash_erase_sector((FFS_AT+pos) / 4096);
}


void ffs_cvt_name(const char *name, char *tmp)
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


void ffs_init(void)
{
    uint16_t n;
    uint16_t n_free=0, n_removed=0;
    
    // ������ ����
    f_init();
    f_size-=DATA_FIRST_ADR;	// ������� �� ������� ������ FAT
    f_size/=4096;		// ����� � ������� 4�
    f_size&=~0x07;		// ����� ������ ��� ������� 8
    
    
    // ������ FAT
    f_read(0, (uint8_t*)fat, sizeof(fat));
    
    // ��������� ������� FAT
    for (n=0; n<FAT_SIZE; n++)
    {
	bool is_free=true;
	uint8_t *data=(uint8_t*)&fat[n];
	uint8_t i;
	
	// ��������� - ����� ���� ������ ��������
	for (i=0; i<sizeof(FILE); i++)
	    if (data[i] != 0xff)
	    {
		is_free=false;
		break;
	    }
	
	if (is_free)
	{
	    n_free++;
	    continue;
	}
	
	// ��������� ����������� ����� � ��������� �����
	if ( (CRC8(CRC8_INIT, data, sizeof(FILE)) != CRC8_OK) ||
	     (fat[n].page >= f_size) ||
	     (fat[n].page + ((fat[n].size + 4095) / 4096) > f_size) )
	{
	    // ������ - ������� ������
	    fat[n].type=TYPE_REMOVED;
	    n_removed++;
	    continue;
	}
    }
    
    // ������� ������� ��������� ��������
    ets_memset(free_sect, 0xff, f_size / 8);
    for (n=0; n<FAT_SIZE; n++)
    {
	// ���������� ��������� � ������ ������
	if ( (fat[n].type == TYPE_REMOVED) ||
	     (fat[n].type == TYPE_FREE) )
	    continue;
	
	uint16_t page=fat[n].page;
	int size=fat[n].size;
	while (size > 0)
	{
	    free_sect[page >> 3]&=~(1<<(page & 0x07));
	    page++;
	    size-=4096;
	}
    }
    
    // ���� ������� ������� ������� - ������ �� � ���������� �� �����
    if ( (n_free < 32) && (n_removed > 0) )
    {
	// ������ �������
	for (n=0; n<FAT_SIZE; n++)
	{
	    if (fat[n].type==TYPE_REMOVED)
	    {
		ets_memset(&fat[n], 0xff, sizeof(FILE));
	    }
	}
	
	// ����������
	f_erase(0);
	f_write(0, (uint8_t*)fat, sizeof(fat));
    }
}


uint32_t ffs_image_at(void)
{
    return FFS_AT;
}


uint32_t ffs_image_size(void)
{
    return f_size*4096 + DATA_FIRST_ADR;
}


uint32_t ffs_size(void)
{
    return f_size*4096;
}


uint32_t ffs_free(void)
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


void ffs_read(uint16_t n, uint16_t offs, uint8_t *data, uint16_t size)
{
    f_read(DATA_FIRST_ADR + fat[n].page*4096 + offs, data, size);
}


int16_t ffs_create(const char *fname, uint8_t type, uint16_t size)
{
    uint16_t n;
    
    // ���� ��������� ������ � FAT
    for (n=0; n<FAT_SIZE; n++)
    {
	if (fat[n].type==TYPE_FREE) break;
    }
    if (n>=FAT_SIZE) return -1;	// ��� �������
    
    // ���� ��������� �����
    uint16_t s_count=(size+4095)/4096;	// �������� ���-�� ��������
    uint16_t s=0;
    while (s < f_size)
    {
	// ���������� ������� �������
	while ( (s < f_size) && (! (free_sect[s >> 3] & (1<<(s & 0x07)))) ) s++;
	if (s >= f_size) break;
	
	// ������� ������ ������� �����
	uint16_t begin=s++;	// ���������� ������ ������� �����
	while ( (s < f_size) && (s-begin < s_count) && (free_sect[s >> 3] & (1<<(s & 0x07))) ) s++;
	if (s-begin < s_count) continue;	// ���� ������� ��������
	
	// ���������� ���� ������
	s=begin;
	break;
    }
    if (s >= f_size) return -1;	// ��� �����
    
    // ��������� ������ FAT
    ffs_cvt_name(fname, fat[n].name);
    fat[n].page=s;
    fat[n].size=size;
    fat[n].type=type;
    fat[n].crc8=CRC8(CRC8_INIT, (uint8_t*)&fat[n], sizeof(FILE)-1);
    
    // ���������� FAT
    f_write(n*sizeof(FILE), (uint8_t*)&fat[n], sizeof(FILE));
    
    // �������� ������� ��� �������
    while (s_count--)
    {
	free_sect[s >> 3]&=~(1<<(s & 0x07));
	s++;
    }
    
    return n;
}


void ffs_writeData(uint16_t n, uint16_t offs, const uint8_t *data, uint16_t size)
{
    // ���������� � ��������������� ���������
    size=(size+3) & ~0x03;
    while (size > 0)
    {
	// �������� �����
	uint32_t addr=DATA_FIRST_ADR + fat[n].page*4096 + offs;
	
	// ������� ��������, ���� ����
	if ((addr & 4095)==0) f_erase(addr);
	
	// �������� ������ ������ (�� ������ ���� �� ����� ��������)
	int s=4096 - (addr & 4095);	// ������� �������� �� ����� ��������
	if (s > size) s=size;
	
	// ����������
	f_write(addr, data, s);
	offs+=s;
	data+=s;
	size-=s;
    }
}


bool ffs_write(const char *fname, uint8_t type, const uint8_t *data, uint16_t size)
{
    // ������� ����
    int16_t n=ffs_create(fname, type, size);
    if (n < 0) return false;
    
    // ���������� ������
    ffs_writeData(n, 0, data, size);
    
    return true;
}


int16_t ffs_find(const char *fname)
{
    uint16_t n;
    char tmp [16];
    
    ffs_cvt_name(fname, tmp);
    
    for (n=0; n<FAT_SIZE; n++)
    {
	int Count;
	if ( (fat[n].type == TYPE_REMOVED) || (fat[n].type == TYPE_FREE) ) continue;
    for (Count=0; Count<16; Count++)
    {
	if (to_upper(fat[n].name[Count]) != to_upper(tmp[Count])) break;
    }
    if (Count >= 16) return n;
    }
    
    return -1;
}


uint32_t ffs_flash_addr(uint16_t n)
{
    return FFS_AT + DATA_FIRST_ADR + fat[n].page * 4096;
}


void ffs_remove(uint16_t n)
{
    // �������� ������ ��� �������
    fat[n].type=TYPE_REMOVED;
    
    // ���������� FAT
    f_write(n*sizeof(FILE), (uint8_t*)&fat[n], sizeof(FILE));
    
    // ������� ������� �������
    uint16_t s_count=(fat[n].size+4095)/4096;	// �������� ���-�� ��������
    uint16_t s=fat[n].page;
    while (s_count--)
    {
	free_sect[s >> 3]|=(1<<(s & 0x07));
	s++;
    }
}


int16_t ffs_rename(uint16_t old_n, const char *fname)
{
    uint16_t n;
    
    // ���� ��������� ������ � FAT
    for (n=0; n<FAT_SIZE; n++)
    {
	if (fat[n].type==TYPE_FREE) break;
    }
    if (n>=FAT_SIZE) return -1;	// ��� �������
    
    // ��������� ����� ������ FAT
    fat[n]=fat[old_n];
    ffs_cvt_name(fname, fat[n].name);
    fat[n].crc8=CRC8(CRC8_INIT, (uint8_t*)&fat[n], sizeof(FILE)-1);
    
    // �������� ������ ������ ��� �������
    fat[old_n].type=TYPE_REMOVED;
    
    // ���������� FAT
    f_write(n*sizeof(FILE), (uint8_t*)&fat[n], sizeof(FILE));
    f_write(old_n*sizeof(FILE), (uint8_t*)&fat[old_n], sizeof(FILE));
    
    return n;
}


const char* ffs_name(uint16_t n)
{
    static char tmp[16+1];
    ets_memcpy(tmp, fat[n].name, 16);
    tmp[16]=0;
    return tmp;
}

const char* ffs_name_utf8 (uint16_t n)
{
    static char tmp[16*2+1];
    KOI8_To_UTF8(tmp, ffs_name(n));
    return tmp;
}
