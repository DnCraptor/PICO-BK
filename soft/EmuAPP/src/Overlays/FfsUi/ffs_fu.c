#include <string.h>

#include "spi_flash.h"
#include "ffs.h"
#include "crc8.h"

#include "ffs_fu.h"
#include "crc8_fu.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl2_fu.text")))

FILE __attribute__ ((aligned (4))) ffs_File;
uint16_t ffs_iFile = 0xFFFF;
char     ffs_TempFileName [16+1];

void AT_OVL ffs_GetFile (uint_fast16_t iFile)
{
    uint_fast8_t Count;

    if (ffs_iFile == iFile) return;

    ffs_iFile = iFile;

    spi_flash_Read (FFS_AT + sizeof (FILE) * iFile, (uint32_t *) &ffs_File, sizeof (FILE));

    // Проверяем - может быть ячейка свободна
    for (Count = 0; (Count < sizeof (FILE)) && (((uint8_t*) &ffs_File) [Count] == 0xff); Count++);

    if (Count < sizeof (FILE))
    {
        // Проверяем контрольную сумму и параметры файла
        if
        (
                (CRC8 (CRC8_INIT, (uint8_t*) &ffs_File, sizeof (FILE)) != CRC8_OK)
            ||  (ffs_File.page >= f_size)
            ||  (ffs_File.page + ((ffs_File.size + 4095) / 4096) > f_size)
        )
        {
            // Ошибка - очищаем запись
            ffs_File.type = TYPE_REMOVED;
        }
    }
}

const char* AT_OVL ffs_name (uint_fast16_t n)
{
    ffs_GetFile (n);

    memcpy (ffs_TempFileName, ffs_File.name, 16);

    ffs_TempFileName [16] = 0;

    return ffs_TempFileName;
}
