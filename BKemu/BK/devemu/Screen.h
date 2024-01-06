// Screen.h: Файл заголовка
//

#pragma once
#include "LockVarType.h"
#include "Screen_Shared.h"
#include "Screen_Sizes.h"
#include "Config.h"

#include <mutex>

// CScreen
// вывод в лог времени фреймов
#define DBG_OUT_SCREENFRAMELENGTH 0
//#define WINAPI      __stdcall

class CScreen : public CWnd
{
	///	DECLARE_DYNAMIC(CScreen)

		BKSCREENHANDLE      m_pscrSharedFunc;
		HMODULE             m_hModule;
	///	using GETBKSCR = BKSCREENHANDLE(WINAPI *)();

		union BCOLOR
		{
			uint32_t dw;
			uint8_t bb[sizeof(uint32_t)];
		};

		int                 m_nViewWidth, m_nViewHeight;    // текущий viewport экрана
		double              m_dAspectRatio;                 // соотношение сторон экрана.

		// 16kPage
		uint8_t            *m_pBuffer;
		uint16_t            m_nBufSize;

		bool                m_bReverseScreen; // флаг перевёрнутого экрана.
		/* DrawDIB в отличие всех остальных рисует вверх ногами.
		или наоборот, все остальные в отличие от DrawDIB рисуют вверх ногами. Поэтому для определения
		направления введён этот флаг. */
		CString             m_strDllName;

	protected:
		CWnd               *m_pwndParent;       // указатель на родительское окно (View)
		mutable int         m_nFrame;           // счётчик фреймов
		int                 m_nCurFPS;          // текущее значение FPS
		mutable std::mutex  m_mutFPS;
		uint8_t             m_nOfs;             // смещение экрана. Используется только в отладочном методе прорисовки экрана
		size_t              m_nPaletteNum_m256; // номер палитры умноженный на 256 для быстроты
		bool                m_bSmoothing;       // флаг использования сглаживания при увеличении картинки
		bool                m_bColorMode;       // флаг работы в цветном режиме
		bool                m_bAdapt;           // флаг работы в ч/б адаптивном режиме
		bool                m_bExtended;        // флаг расширенного режима.
		bool                m_bLuminoforeEmul;  // флаг режима плавного затухания люминофора
		mutable LockVarType m_lockChangeMode;   // флаг, пока переключаются режимы экрана,чтобы в это время не рисовался экран
		mutable LockVarType m_lockPrepare;
		mutable LockVarType m_lockBusy;

		std::unique_ptr<uint32_t[]> m_pColTable32;
		std::unique_ptr<uint32_t[]> m_pMonoTable32;

		BKScreen_t          m_BKScreen;
		BKScreen_t          m_AZScreen;
		BKScreen_t         *m_pCurrentScreen;
		int                 m_nCurrentScreen;

#if (_DEBUG && DBG_OUT_SCREENFRAMELENGTH)
		DWORD m_nTickCounter;
		FILE *dbgFile;
#endif // _DEBUG

		// захват видео
		bool                m_bCaptureProcessed;
		bool                m_bCaptureFlag;
		HANDLE              m_hChildStd_IN_Rd;
		HANDLE              m_hChildStd_IN_Wr;
		mutable std::mutex  m_mutCapture;
		void                PrepareCapture(const CString &strUniq);
		void                CancelCapture();
		void                WriteToPipe() const;

		// мышь марсианка
		int                 m_nPointX;
		int                 m_nPointY;
		uint16_t            m_MouseValue; // новые значения параметров, выдаваемых мышью
		bool                m_bMouseMove;
		bool                m_bMouseOutEna; // флаг разрешения выдачи данных мышой
		uint16_t            m_nMouseEnaStrobe; // строб формирования разрешения выдачи

	public:
		CScreen(CONF_SCREEN_RENDER nRenderType);
		CScreen(CONF_SCREEN_RENDER nRenderType, uint8_t *buffer, uint16_t size); // 16kpage
		virtual ~CScreen() override;

		int                 SetScreenViewport(int w)
		{
			if (w <= 0)
			{
				w = 1024;
			}

			m_nViewWidth = w;
			m_nViewHeight = static_cast<int>(w / m_dAspectRatio);
			return m_nViewHeight;
		}
		CPoint              GetScreenViewport() const
		{
			return {m_nViewWidth, m_nViewHeight};
		}
		void                SetAspectRatio(double d)
		{
			if (d <= 0.005) //некорректные значения заменим значением по умолчанию.
			{
				d = 4.0 / 3.0;
			}

			m_dAspectRatio = d;
		}
		double              getAspectRatio() const
		{
			return          m_dAspectRatio;
		}

		uint16_t            GetMouseStatus();
		void                SetMouseStrobe(uint16_t data);

		void                ChangeBuffer(uint8_t *buffer, uint16_t size) // 16kpage
		{
			m_pBuffer = buffer;
			m_nBufSize = size;
		}
		void                InitVars(CONF_SCREEN_RENDER nRenderType);
		void                ClearObjects();

		void                ReDrawScreen() // 16kpage
		{
			PrepareScreenRGB32(m_pBuffer);
			DrawScreen();
		};
		void                DrawScreen(const bool bCheckFPS = false, const int screen = 0) const;

		void                PrepareScreenLineByteRGB32(int nLineNum, int nByteNum, uint8_t b) const;
		void                PrepareScreenLineWordRGB32(int nLineNum, int nByteNum, uint16_t w) const;
#ifdef _DEBUG
		void                PrepareScreenLineByte_Debug(int nLineNum, int nByteNum) const;
#endif
		void                PrepareScreenRGB32(uint8_t *ScreenBuffer) const;
		void                AdjustLayout(int nPW, int nPH);
		// Screen own methods
		void                SetRegister(uint16_t w)
		{
			SetOffset(LOBYTE(w));
			SetExtendedMode((w & 01000) == 0);
		}
		inline void         SetOffset(uint8_t nOfs)
		{
			m_nOfs = nOfs;
		}
		inline uint8_t      GetOffset() const
		{
			return m_nOfs;
		}
		inline void         SetExtendedMode(bool bFlag)
		{
			m_bExtended = bFlag;
		}
		inline bool         GetExtendedMode() const
		{
			return m_bExtended;
		}
		inline void         SetLuminoforeEmuMode(bool bFlag)
		{
			m_bLuminoforeEmul = bFlag;
		}
		inline bool         GetLuminoforeEmuMode() const
		{
			return m_bLuminoforeEmul;
		}
		inline int          GetFPS() const
		{
			return m_nCurFPS;
		}
		inline int          GetPalette() const
		{
			return static_cast<int>(m_nPaletteNum_m256 >> 8);
		}
		void                SetPalette(int palette)
		{
			m_nPaletteNum_m256 = (static_cast<size_t>(palette) & 0xf) << 8;
		}

		void                SetAdaptMode(bool bFlag)
		{
			m_bAdapt = bFlag;
			InitColorTables();
		}
		inline bool         IsAdaptMode() const
		{
			return m_bAdapt;
		}

		void                SetSmoothing(bool bSmoothing);
		inline bool         IsSmoothing() const
		{
			return m_bSmoothing;
		}
		void                SetColorMode(bool bColorMode = true);
		inline bool         IsColorMode() const
		{
			return m_bColorMode;
		}

		// захват видео
		void                SetCaptureStatus(bool bCapture, const CString &strUniq);
		bool                IsCapture() const
		{
			return m_bCaptureProcessed;
		}

		void                RestoreFS();
		bool                SetFullScreenMode();
		bool                SetWindowMode();
		bool                IsFullScreenMode() const;

		inline CWnd        *GetBackgroundWindow()
		{
			return this;
		}
		HBITMAP             GetScreenshot() const;

		bool                InitColorTables();

		void                ChangeScreen(int nScr);
		uint32_t           *getAZBKTexture() const
		{
			return m_AZScreen.pTexture;
		}

	protected:

		virtual BOOL PreCreateWindow(CREATESTRUCT &cs) override;
		virtual BOOL PreTranslateMessage(MSG *pMsg) override;

		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg BOOL OnEraseBkgnd(CDC *pDC);
		afx_msg void OnDestroy();
		afx_msg void OnTimer(UINT_PTR nIDEvent);

		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	///	DECLARE_MESSAGE_MAP()
};


