#include <stdint.h>
#include <string.h>

#include "AnyMem.h"
#include "ffs.h"
#include "spi_flash.h"
#include "crc8.h"

#include "ffs_i.h"
#include "crc8_i.h"
#include "spi_flash_i.h"

#define AT_OVL     __attribute__((section(".ovl3_i.text")))
#define BSS_AT_OVL __attribute__((section(".ovl3_i.bss")))

static FILE BSS_AT_OVL fat [FAT_SIZE];

void AT_OVL ffs_init (void) {
    /*
    uint_fast16_t n;
    uint_fast16_t n_free    = 0;
    uint_fast16_t n_removed = 0;
    struct rom_header {
        uint8_t magic;
        uint8_t count;
        uint8_t flags1;
        uint8_t flags2;
    } hdr;
    // Инитим флэш
    // Читаем заголовок
    spi_flash_Read (0, (uint32_t *) &hdr, sizeof (hdr));
    // Определяем размер флэша
    switch (hdr.flags2 >> 4) {
        case 3:
            // 16 Мбит
            f_size = 2048 * 1024 - FFS_AT;
            break;
        case 4:
            // 32 Мбит
            f_size = 4096 * 1024 - FFS_AT;
            break;
        case 2:
        default:
            // 8 Мбит
            f_size = 1024 * 1024 - FFS_AT;
            break;
    }
    f_size -= FFS_DATA_FIRST_ADR; // убираем из размера размер FAT
    f_size /= 4096;               // сразу в секторы 4к
    f_size &= ~0x07;              // чтобы размер был кратным 8
    memset (free_sect, 0xff, f_size / 8);
    // Читаем FAT
    spi_flash_Read (FFS_AT, (uint32_t *) &fat [0], sizeof (fat));
    // Проверяем таблицу FAT
    for (n = 0; n < FAT_SIZE; n++) {
        uint_fast8_t Count;
        // Проверяем - может быть ячейка свободна
        for (Count = 0; (Count < sizeof (FILE)) && (AnyMem_r_u8 (&((uint8_t*) &fat [n]) [Count]) == 0xff); Count++);
        if (Count >= sizeof (FILE)) {
            n_free++;
        }
        else {
            uint_fast16_t Page = AnyMem_r_u16 (&fat [n].page);
            int_fast32_t  Size = AnyMem_r_u16 (&fat [n].size);
            // Проверяем контрольную сумму и параметры файла
            if (    (CRC8_i (CRC8_INIT, (uint8_t*) &fat [n], sizeof (FILE)) != CRC8_OK)
                ||  (Page >= f_size)
                ||  (Page + ((Size + 4095) / 4096) > f_size)
            ) {
                // Ошибка - очищаем запись
                AnyMem_w_u8 (&fat [n].type, TYPE_REMOVED);
            }
            if (AnyMem_r_u8 (&fat [n].type) == TYPE_REMOVED) {
                n_removed++;
            }
            else {
                // Создаем таблицу свободных секторов
                while (Size > 0) {
                    free_sect [Page >> 3] &= ~(1 << (Page & 0x07));
                    Page++;
                    Size -= 4096;
                }
            }
        }
    }
    // Если таблица слишком грязная - чистим ее и записываем на место
    if ((n_free < 32) && (n_removed > 32)) {
        // Чистим таблицу
        for (n = 0; n < FAT_SIZE; n++) {
            if (AnyMem_r_u8 (&fat [n].type) == TYPE_REMOVED) {
                AnyMem_memset (&fat [n], 0xff, sizeof (FILE));
            }
        }
        // Записываем
        spi_flash_EraseSector_i (FFS_AT / 4096);
        spi_flash_Write_i       (FFS_AT, (const uint32_t *) &fat [0], sizeof (fat));
    }
    */
}
