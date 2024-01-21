#pragma once
#ifndef DISK_H
#define DISK_H

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

struct PARSE_RESULT
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

#endif
