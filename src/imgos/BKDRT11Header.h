#pragma once
#include <string.h>
#include <stdint.h>
#include <string>

#pragma pack(push)
#pragma pack(1)
struct RT11SegmentHdr
{
	uint16_t segments_num;          // число сегментов, отведённых под каталог
	uint16_t next_segment;          // номер следующего сегмента каталога, если 0 - то этот сегмент последний
	uint16_t used_segments;         // счётчик сегментов, имеющих записи, есть только в первом сегменте
	uint16_t filerecord_add_bytes;  // число дополнительных БАЙТОВ! в записи о файле в каталоге
	uint16_t file_block;            // номер блока, с которого начинается самый первый файл, описанный в данном сегменте

	RT11SegmentHdr()
	{
		memset(this, 0, sizeof(RT11SegmentHdr));
	}
	RT11SegmentHdr &operator = (const RT11SegmentHdr &src)
	{
		memcpy(this, &src, sizeof(RT11SegmentHdr));
		return *this;
	}
	RT11SegmentHdr &operator = (const RT11SegmentHdr *src)
	{
		memcpy(this, src, sizeof(RT11SegmentHdr));
		return *this;
	}
};
#pragma pack(pop)

// размер сегмента в словах
#define SEGMENT_SIZE 512

#define RT11_FILECLASS_PREFIX 020

#define RT11_FILESTATUS_TEMPORARY 1
#define RT11_FILESTATUS_UNUSED 2
#define RT11_FILESTATUS_PERMANENT 4
#define RT11_FILESTATUS_EOS 010
#define RT11_FILESTATUS_READONLY 0100
#define RT11_FILESTATUS_PROTECTED 0200
