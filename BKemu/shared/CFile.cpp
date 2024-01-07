#include "CFile.h"

const void* CFile::hFileNull = 0;

bool CFile :: Open(const char* path, int mode) {
    FRESULT result = f_open(&fil, path, mode);
    if (result != FR_OK) {
        return false;
    }
    m_hFile = &fil;
    return true;
}

FSIZE_t CFile :: Seek(FSIZE_t off, int ignore) {
    FRESULT result = f_lseek(&fil, off);
    if (result != FR_OK) {
        return 0;
    }
    return off;
}

FSIZE_t CFile :: Read(void* buff, FSIZE_t sz) {
    UINT br;
    FRESULT result = f_read(&fil, buff, sz, &br);
    if (result != FR_OK) {
        return 0;
    }
    return br;
}

FSIZE_t CFile :: Write(void* buff, FSIZE_t sz) {
    UINT br;
    FRESULT result = f_write(&fil, buff, sz, &br);
    if (result != FR_OK) {
        return 0;
    }
    return br;
}

void CFile :: Close() {
    m_hFile = 0;
    f_close(&fil);
}
