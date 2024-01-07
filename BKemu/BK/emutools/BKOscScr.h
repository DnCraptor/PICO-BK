
// BKOscScr.h : файл заголовка
//
#pragma once
#include "BKSound_Defines.h"
#include "BKOscScr_Shared.h"
#include "Config.h"
#include <mutex>

// диалоговое окно CBKOscScr
class CBKOscScr : public CWnd
{
		DECLARE_DYNAMIC(CBKOscScr)

		BKOSCSCRHANDLE m_pSharedFunc;
		HMODULE m_hModule;
	///	using GETBKOSC = BKOSCSCRHANDLE(WINAPI *)();

	protected:
		HWND                m_hwndScreen;   // Screen View window handle
		SAMPLE_INT*        *m_inBuf;
		int                 m_inLen;        // размер буфера в сэмплах
		int                 m_inLenByte;    // размер буфера в байтах
		std::mutex          m_lockBusy;
		CString             m_strDllName;

// Создание
	public:
		CBKOscScr(CONF_OSCILLOSCOPE_RENDER nRenderType);    // стандартный конструктор
		virtual ~CBKOscScr() override;
		void SetBuffer(int buffer_len);
		void FillBuffer(SAMPLE_INT *inBuf);

// Реализация
	protected:
		// Созданные функции схемы сообщений
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnDestroy();
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC *pDC);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		DECLARE_MESSAGE_MAP()
};
