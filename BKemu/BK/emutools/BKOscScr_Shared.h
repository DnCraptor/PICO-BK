#pragma once
#include "BKSound_Defines.h"
#include "SafeReleaseDefines.h"

#define NULL 0

#ifdef BKOSCDLL_EXPORTS
#define BKOSCDLL_API __declspec(dllexport)
#else
#define BKOSCDLL_API 
#endif

#ifdef __cplusplus
#define BKEXTERN_C     extern "C"
#else
#define BKEXTERN_C
#endif // __cplusplus

BKEXTERN_C class BKOSCDLL_API CBKOScScr_Shared
{
	public:
		virtual void BKOSC_OnDisplay(SAMPLE_INT *inBuf, int inLen) = NULL;
		virtual HRESULT BKOSC_Screen_Init(CWnd *pWndScreen) = NULL;
		virtual void BKOSC_Screen_Done() = NULL;
		virtual void BKOSC_OnSize(int cx, int cy) = NULL;
		virtual HRESULT BKOSC_OnSetBuffer(int nLen) = NULL;

		CBKOScScr_Shared() {};
		virtual ~CBKOScScr_Shared() {};
};

using BKOSCSCRHANDLE = CBKOScScr_Shared *;

