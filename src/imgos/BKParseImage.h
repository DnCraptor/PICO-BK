#pragma once
#include "BKImgFile.h"
#include "disk_img.h"

struct PARSE_RESULT
{
	unsigned long   nImageSize;     // размер файла в байтах
	unsigned long   nBaseOffset;    // смещение до логического диска (раздела) в образе файла
	int             nPartLen;       // размер раздела в блоках (если возможно получить)
	IMAGE_TYPE      imageOSType;    // результат парсинга
	bool            bImageBootable; // флаг загрузочного образа
	fs::path        strName;        // имя файла образа

	PARSE_RESULT()
	{
		clear();
	}

	~PARSE_RESULT()
	{
		strName.clear();
	}

	void clear()
	{
		nImageSize = nBaseOffset = nPartLen = 0;
		imageOSType = IMAGE_TYPE::UNKNOWN;
		bImageBootable = false;
		strName.clear();
	}

	PARSE_RESULT(const PARSE_RESULT *src)
	{
		nImageSize = src->nImageSize;
		nBaseOffset = src->nBaseOffset;
		nPartLen = src->nPartLen;
		imageOSType = src->imageOSType;
		bImageBootable = src->bImageBootable;
		strName = src->strName;
	}

	PARSE_RESULT(const PARSE_RESULT_C &src)
	{
		nImageSize = src.nImageSize;
		nBaseOffset = src.nBaseOffset;
		nPartLen = src.nPartLen;
		imageOSType = src.imageOSType;
		bImageBootable = src.bImageBootable;
		strName = src.strName;
	}

	PARSE_RESULT &operator = (const PARSE_RESULT &src)
	    = default;
};

class CBKParseImage
{
		static const char *strID_Andos;
		static const char *strID_DXDOS;
		static const char *strID_FAT12;
		static const char *strID_Aodos;
		static const char *strID_Nord;
		static const char *strID_Nord_Voland;
		static const char *strID_RT11;
		static const char *strID_RT11Q;
	public:
		CBKParseImage();
		~CBKParseImage();
		PARSE_RESULT    ParseImage(const fs::path &fname, unsigned long nBaseOffset) const;
		static const std::string GetOSName(const IMAGE_TYPE t);

	protected:
		// сука, не получается эту функцию из ImgUtil здесь применять, поэтому тупо продублируем
		uint16_t        CalcCRC(uint8_t *buffer, size_t len) const;
		bool            charcompare(uint8_t a, uint8_t b) const;
		bool            substrfind(uint8_t *bufs, size_t len_bufs, uint8_t *buff, size_t len_buff) const;
		int             IsCSIDOS3(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             IsRT11_old(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             IsRT11(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             AnalyseMicrodos(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             IsOPTOK(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             IsHolography(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
		int             IsDaleOS(CBKImgFile *f, unsigned long nBaseOffsetLBA, uint8_t *buf) const;
};

