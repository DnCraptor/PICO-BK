#ifndef FFS_H
#define FFS_H


#include "ovl.h"
#include "ets.h"

#ifdef __cplusplus
extern "C" {
#endif


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

#define FAT_SIZE	(4096/sizeof(FILE))
extern FILE fat[FAT_SIZE];


#define ffs_init_ovln OVL_NUM_FS
#define ffs_init_ovls OVL_SEC_FS
void ffs_init(void);

#define ffs_image_at_ovln   OVL_NUM_FS
#define ffs_image_at_ovls   OVL_SEC_FS
#define ffs_image_size_ovln OVL_NUM_FS
#define ffs_image_size_ovls OVL_SEC_FS
uint32_t ffs_image_at(void);
uint32_t ffs_image_size(void);

#define ffs_size_ovln OVL_NUM_FS
#define ffs_size_ovls OVL_SEC_FS
#define ffs_free_ovln OVL_NUM_FS
#define ffs_free_ovls OVL_SEC_FS
uint32_t ffs_size(void);
uint32_t ffs_free(void);

#define ffs_read_ovln OVL_NUM_FS
#define ffs_read_ovls OVL_SEC_FS
#define ffs_write_ovln OVL_NUM_FS
#define ffs_write_ovls OVL_SEC_FS
void ffs_read(uint16_t n, uint16_t offs, uint8_t *data, uint16_t size);
bool ffs_write(const char *fname, uint8_t type, const uint8_t *data, uint16_t size);

#define ffs_create_ovln OVL_NUM_FS
#define ffs_create_ovls OVL_SEC_FS
#define ffs_writeData_ovln OVL_NUM_FS
#define ffs_writeData_ovls OVL_SEC_FS
int16_t ffs_create(const char *fname, uint8_t type, uint16_t size);
void ffs_writeData(uint16_t n, uint16_t offs, const uint8_t *data, uint16_t size);

#define ffs_find_ovln OVL_NUM_FS
#define ffs_find_ovls OVL_SEC_FS
#define ffs_flash_addr_ovln OVL_NUM_FS
#define ffs_flash_addr_ovls OVL_SEC_FS
int16_t ffs_find(const char *fname);
uint32_t ffs_flash_addr(uint16_t n);

#define ffs_remove_ovln OVL_NUM_FS
#define ffs_remove_ovls OVL_SEC_FS
#define ffs_rename_ovln OVL_NUM_FS
#define ffs_rename_ovls OVL_SEC_FS
void ffs_remove(uint16_t n);
int16_t ffs_rename(uint16_t n, const char *fname);

#define ffs_cvt_name_ovln OVL_NUM_FS
#define ffs_cvt_name_ovls OVL_SEC_FS
#define ffs_name_ovln OVL_NUM_FS
#define ffs_name_ovls OVL_SEC_FS
void ffs_cvt_name(const char *name, char *tmp);
const char* ffs_name(uint16_t n);	// в статической переменной

#ifdef __cplusplus
}
#endif


#endif
