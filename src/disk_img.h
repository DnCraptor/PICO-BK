#pragma once
#ifndef DISK_H
#define DISK_H

extern "C" {
#include "string.h"
}

enum class IMAGE_TYPE : int
{
	ERROR_NOIMAGE = -1,
	UNKNOWN = 0,
	ANDOS,
	MKDOS,
	AODOS,
	NORD,
	MIKRODOS,
	CSIDOS3,
	RT11,
	NCDOS,
	DXDOS,
	OPTOK,
	HOLOGRAPHY,
	DALE,
	MSDOS
};

struct PARSE_RESULT_C
{
	unsigned long   nImageSize;     // размер файла в байтах
	unsigned long   nBaseOffset;    // смещение до логического диска (раздела) в образе файла
	int             nPartLen;       // размер раздела в блоках (если возможно получить)
	IMAGE_TYPE      imageOSType;    // результат парсинга
	bool            bImageBootable; // флаг загрузочного образа
	char            strName[256];        // имя файла образа
};

// размер блока по умолчанию. == размер сектора
constexpr auto BLOCK_SIZE = 512;

extern "C" void detect_os_type(const char* path, char* os_type, size_t sz);
#if EXT_DRIVES_MOUNT
extern "C" bool mount_img(const char* path);
#endif

#endif
