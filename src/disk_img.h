/**
 * Partly migration from https://gid.pdp-11.ru/bkemuc.html?custom=3.13.2401.10604
*/
#pragma once
#ifndef DISK_H
#define DISK_H

extern "C" {
#include "string.h"
#include "stdint.h"
int m_add_file_ext(const char* fname, bool dir, int dir_num);
}

enum FR_ATTR : uint32_t {
	READONLY    = 0x001,
	HIDDEN      = 0x002,
	PROTECTED   = 0x004,    // везде, имеет смысл как RO, но в RT-11 есть оба атрибута, и RO, и P
	SYSTEM      = 0x008,
	ARCHIVE     = 0x010,
	DIRECTORY   = 0x020,
	VOLUMEID    = 0x040,
	TEMPORARY   = 0x080,
	LOGDISK     = 0x100,
	LINK        = 0x200,
	BAD         = 0x400,
	DELETED     = 0x800
};

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
extern "C" bool mount_img(const char* path, int curr_dir_num);
#endif

#endif
