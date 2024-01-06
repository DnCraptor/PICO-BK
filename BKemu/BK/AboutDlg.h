/*
 * Copyright (C) 2022 SCALOlaz
 * 1,2,3 pages for ABOUT
 */
#pragma once
#include "resource.h"       // основные символы

#include "Config.h"
#include "pch.h"
#include "StaticLink.h"
#include <afxhtml.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 1 page
class AboutDlg : public CDialogEx
{
		DECLARE_DYNAMIC(AboutDlg)

		CStaticLink m_staticHomePage, m_staticHomePage2;
		CStaticLink m_staticMail, m_staticMail2;

	public:
		enum { IDD = IDD_ABOUTBOX_1 };

		AboutDlg(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~AboutDlg() override;

		// Реализация
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		DECLARE_MESSAGE_MAP()
};

// 2 page
class AboutLinks: public CDialogEx
{
		DECLARE_DYNAMIC(AboutLinks)
		CStaticLink m_staticGames;
		CStaticLink m_staticForum;
		CStaticLink m_staticSVN, m_staticSVN2;
		CStaticLink m_staticSVN3, m_staticSVN4, m_staticSVN5;
		// and Some Other Links

	public:
		enum { IDD = IDD_ABOUTBOX_2 };

		AboutLinks(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~AboutLinks() override;

		// Реализация
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		DECLARE_MESSAGE_MAP()
};

//Тут нужно свой создать класс на основе CFrameWnd, и в нём уже делать CHtmlView
class CHtmlWnd : public CFrameWnd
{
		DECLARE_DYNAMIC(CHtmlWnd)

		CHtmlView *m_Ie;

	public:
		CHtmlWnd();
		virtual ~CHtmlWnd() override;

	protected:
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		DECLARE_MESSAGE_MAP()
};

// 3 page
class AboutThx : public CDialogEx
{
		DECLARE_DYNAMIC(AboutThx)
	public:
		enum { IDD = IDD_ABOUTBOX_3 };

		AboutThx(CWnd *pParent = nullptr);   // стандартный конструктор
		virtual ~AboutThx() override;

	private:
		CHtmlWnd *m_pFrmWnd;
		static LPCTSTR	m_lpClassName;
		// Реализация
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // поддержка DDX/DDV
		virtual BOOL OnInitDialog() override;
		DECLARE_MESSAGE_MAP()
};