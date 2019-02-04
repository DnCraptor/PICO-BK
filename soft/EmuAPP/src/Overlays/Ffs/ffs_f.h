#ifndef FFS_F_H
#define FFS_F_H

#include <stdint.h>

uint32_t        ffs_image_at   (void);
uint32_t        ffs_image_size (void);

uint32_t        ffs_size       (void);
uint32_t        ffs_free       (void);

void            ffs_read       (uint16_t n, uint16_t offs, uint8_t *data, uint16_t size);
uint_fast8_t    ffs_write      (const char *fname, uint8_t type, const uint8_t *data, uint16_t size);

int16_t         ffs_create     (const char *fname, uint8_t type, uint16_t size);
void            ffs_writeData  (uint16_t n, uint16_t offs, const uint8_t *data, uint16_t size);

int16_t         ffs_find       (const char *fname);
uint32_t        ffs_flash_addr (uint16_t n);

void            ffs_remove     (uint16_t n);
int16_t         ffs_rename     (uint16_t n, const char *fname);

void            ffs_cvt_name   (const char *name, char *tmp);

#endif
