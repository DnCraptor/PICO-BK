#pragma once
#include "SafeReleaseDefines.h"

#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLL_API __declspec(dllexport)
#else
#define BKSCRDLL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
#define BKEXTERN_C     extern "C"
#else
#define BKEXTERN_C
#endif // __cplusplus

// структура, в которой собраны все индивидуальные параметры текстуры экрана
BKEXTERN_C struct BKScreen_t
{
	uint32_t   *pTexture;       // указатель на выделенную память текстуры.
	uint32_t    nTextureSize;   // размер текстуры в байтах
	uint32_t    nPitch;
	int         nWidth;         // реальная ширина текстуры в пикселах
	int         nHeight;        // реальная ширина текстуры в пикселах
	RECT        rcFSDim;        // текущее разрешение экрана
	RECT        rcFSViewPort;   // размер viewporta в полноэкранном режиме
	// left, top - координаты экрана, right, bottom - размеры, а не координаты!!!

	BKScreen_t()
		: pTexture(nullptr)
		, nTextureSize(0)
		, nPitch(0)
		, nWidth(0)
		, nHeight(0)
		, rcFSDim(RECT())
		, rcFSViewPort(RECT())
	{}

// Вот так делать нельзя, почему-то память портится.
//  ~BKScreen_t()
//  {
//      SAFE_DELETE_ARRAY(pTexture);
//  }

	bool createTexture(int w, int h)
	{
		nWidth = w;
		nHeight = h;
		int nBitSize = w * h;
		nTextureSize = nBitSize * sizeof(uint32_t);
		nPitch = w * sizeof(uint32_t);
		pTexture = new uint32_t[nBitSize];
		return !!(pTexture);
	}
};

BKEXTERN_C class CBKScreen_Shared
{
	protected:
		HWND                m_hwndScreen;       // хэндл окна оконного режима
		CWnd               *m_pwndMain;         // указатель на главное окно
		CWnd               *m_pwndParent;       // указатель на родительское окно (View)
		CWnd               *m_pwndScreen;       // указатель на окно оконного режима
		bool                m_bFullScreen;      // флаг работы в полноэкранном режиме
		BKScreen_t          m_screen;

		WINDOWPLACEMENT     m_windowedPlacement;
		WINDOWPLACEMENT     m_mainPlacement;

	public:


		virtual HRESULT     BKSS_ScreenView_Init(BKScreen_t *pScPar, CWnd *pwndScr)
		{
			// заполняем хэндлы и указатели на окна
			m_pwndScreen    = pwndScr;
			m_pwndParent    = m_pwndScreen->GetParent();
			m_pwndMain      = m_pwndParent->GetParentOwner();
			m_hwndScreen    = m_pwndScreen->GetSafeHwnd();
			m_screen        = *pScPar;
			return S_OK;
		}

		virtual HRESULT     BKSS_ScreenView_ReInit(BKScreen_t *pScPar)
		{
			m_screen = *pScPar;
			return S_OK;
		}

		virtual void        BKSS_ScreenView_Done() {}
		virtual void        BKSS_DrawScreen() {}
		virtual void        BKSS_RestoreFullScreen() {}
		virtual bool        BKSS_SetFullScreenMode()
		{
			return false;
		}
		virtual bool        BKSS_SetWindowMode()
		{
			return false;
		}
		virtual CWnd       *BKSS_GetBackgroundWindow()
		{
			return nullptr;
		}

		virtual void        BKSS_SetSmoothing(bool bSmoothing) {}
		virtual void        BKSS_SetColorMode() {}
		virtual void        BKSS_OnEraseBackground() {}
		virtual void        BKSS_OnSize(int cx, int cy) {}

		CBKScreen_Shared()
			: m_hwndScreen(nullptr)
			, m_pwndMain(nullptr)
			, m_pwndParent(nullptr)
			, m_pwndScreen(nullptr)
			, m_bFullScreen(false)
		{}
		virtual ~CBKScreen_Shared()
		    = default;

		virtual bool BKSS_GetReverseFlag()
		{
			return false;
		}

		virtual bool BKSS_IsFullScreenMode()
		{
			return m_bFullScreen;
		}
};

using BKSCREENHANDLE = CBKScreen_Shared *;

