
// BK.h : главный файл заголовка для приложения BK
//
#pragma once

#ifndef __AFXWIN_H__
#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы
#include "afxwinappex.h"
#include <clocale>

// CBKApp:
// О реализации данного класса см. BK.cpp
//
// ----мои-сообщения-----------------------------------------------------------
constexpr auto QUESTION_PRIME_HWND = 0xBE;
constexpr auto ANSWER_PRIME_HWND = 0xDA;
// ----------------------------------------------------------------------------

class CBKApp : public CWinAppEx
{
		CMutex              m_mtInstance;           // мутекс, предназначенный для запуска только одной копии программы (пока не функционирует как надо)
		bool                m_bIsCopy;              // признак копии проги

	public:
		CBKApp();
		virtual ~CBKApp() override;

	protected:
		void    MakeTitleVersion();
// Переопределение
	public:
		virtual BOOL InitInstance() override;
		virtual int ExitInstance() override;

// Реализация
		UINT            m_nInterAppGlobalMsg;   // индекс юзерского сообщения, которое будет зарегистрировано
		UINT            m_nAppLook;
		bool            m_bHiColorIcons;
		bool            m_bNewConfig;           // флаг, что не было ини файла и он был создан по умолчанию
		CString         m_strProgramTitleVersion;   //сделаем публичную переменную, куда поместим при инициализации
		//имя проги и версию, чтоб один раз сформировать, и много раз использовать
		CString         m_strProgramName;           //имя проги, с теми же целями.
		CString         m_strCompileVersion;

	protected:
		virtual void PreLoadState() override;
		virtual void LoadCustomState() override;
		virtual void SaveCustomState() override;
		virtual BOOL PreTranslateMessage(MSG *pMsg) override;
		virtual int Run() override;

		afx_msg void OnAppAbout();
		DECLARE_MESSAGE_MAP()
};

extern CBKApp theApp;

