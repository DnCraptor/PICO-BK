#ifndef FFS_H
#define FFS_H

#include <stdint.h>

enum
{
    TYPE_REMOVED=0,
    
    TYPE_TAPE,
    TYPE_PROG,
    TYPE_GAME,
    TYPE_UTIL,
    TYPE_ROM,
    
    TYPE_FREE=0xff,
};


typedef struct FILE
{
    uint16_t page;
    uint16_t size;
    char     name [16];
    uint16_t reserved;
    uint8_t  type;
    uint8_t  crc8;

} FILE;

#define FFS_AT              0x80000
#define FAT_SIZE            (4096 / sizeof (FILE))
#define FFS_DATA_FIRST_PAGE ((sizeof (FILE) * FAT_SIZE + 0xFFFUL) / 0x1000UL)
#define FFS_DATA_FIRST_ADR  (FFS_DATA_FIRST_PAGE * 0x1000UL)

extern uint8_t  free_sect [128];  // максимум 4мб
extern uint32_t f_size;

//extern FILE fat[FAT_SIZE];

#endif
