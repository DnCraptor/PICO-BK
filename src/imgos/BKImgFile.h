#pragma once
#include "ff.h"

class CBKImgFile
{
		uint8_t         m_nCylinders;
		uint8_t         m_nHeads;
		uint8_t         m_nSectors;

		FILE           *m_f;        // если открываем образ - то это его файл
	////	FIL          m_h;        // если открываем устройство - то это его хэндл
		fs::path        m_strName;  // имя образа или устройства

		struct CHS
		{
			uint8_t c, h, s;
			CHS() : c(0), h(0), s(0) {}
		};

		CHS             ConvertLBA(const UINT lba) const;   // LBA -> CHS
		UINT            ConvertCHS(const CHS chs) const;    // CHS -> LBA
		UINT            ConvertCHS(const uint8_t c, const uint8_t h, const uint8_t s) const;    // CHS -> LBA
		// операция ввода вывода, т.к. оказалось, что fdraw не умеет блочный обмен мультитрековый,
		// да и позиционирование нужно делать вручную
	////	bool            IOOperation(const DWORD cmd, FD_READ_WRITE_PARAMS *rwp, void *buffer, const UINT numSectors) const;

	public:
		CBKImgFile();
		CBKImgFile(const fs::path &strName, const bool bWrite);
		~CBKImgFile();
		bool            Open(const fs::path &pathName, const bool bWrite);
		void            Close();

		// установка новых значений chs
		// если какое-то значение == 255, то заданное значение не меняется
		void            SetGeometry(const uint8_t c, const uint8_t h, const uint8_t s);
		/*
		buffer - куда читать/писать, о размере должен позаботиться пользователь
		cyl - номер дорожки
		head - номер головки
		sector - номер сектора
		numSectors - количество читаемых/писаемых секторов
		*/
		bool            ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const;
		bool            WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const;
		bool            ReadLBA(void *buffer, const UINT lba, const UINT numSectors) const;
		bool            WriteLBA(void *buffer, const UINT lba, const UINT numSectors) const;
		long            GetFileSize() const;
		bool            SeekTo00() const;
		bool            IsFileOpen() const;
};

