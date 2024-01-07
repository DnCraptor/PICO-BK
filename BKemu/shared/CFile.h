#pragma once

extern "C" {
#include "ff.h"
}

class CFile {
    FIL fil;
public:
    struct SeekPosition {
        static const int begin = 0;
    };
    void* m_hFile;
    CFile(): m_hFile(0) { }
	virtual ~CFile() { }
    bool Open(const char* path, int mode);
    FSIZE_t Seek(FSIZE_t off, int mode);
    void SeekToBegin() { f_lseek(&fil, 0); }
    FSIZE_t Read(void* buff, FSIZE_t sz);
    FSIZE_t Write(void* buff, FSIZE_t sz);
    FSIZE_t GetLength() { return f_size(&fil); }
    void Close();
    static const void* hFileNull;
	static const int shareDenyWrite = 1;
    static const int shareDenyNone = 2;
    static const int modeReadWrite = FA_WRITE | FA_READ;
    static const int modeRead = FA_READ;
    static const int modeWrite = FA_WRITE;
    static const int modeCreate = FA_CREATE_ALWAYS;
    static const int typeBinary = 0;
    static const int osRandomAccess = 0;
};


