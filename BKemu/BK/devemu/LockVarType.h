#pragma once
#include "pch.h"

#ifdef _WIN64
#define DWORD_ALIGN    __declspec(align(8))
#else
#define DWORD_ALIGN    __aligned(4)
#endif
#define LOCK_VAR_TYPE  DWORD_ALIGN DWORD volatile

#ifdef BKSCRDLL_EXPORTS
#define BKSCRDLLC_API __declspec(dllexport)
#else
#define BKSCRDLLC_API
#endif

#ifdef __cplusplus
#define BKEXTERN_C     extern "C"
#else
#define BKEXTERN_C
#endif // __cplusplus


// Вот пример класса LockVarType, создающего объекты быстрой межпоточной синхронизации размером в одно слово.
// by Patron

BKEXTERN_C class BKSCRDLLC_API LockVarType
{
	public:

		LockVarType() : dwLockVar(0)
		{
		}

		inline DWORD IsLocked()
		{
			return dwLockVar;
		}

		inline void Lock(int nSleepMS = 0)
		{
		//	while (InterlockedCompareExchange(&dwLockVar, 1, 0))
			{
		/// TODO:?		Sleep(nSleepMS);
			}
		}

		inline void UnLock()
		{
			dwLockVar = 0;
		}

		inline BOOL TimedLock(DWORD uTimeOut_MS, int nSleepMS = 0)
		{
		///	DWORD uTC = GetTickCount();

		///	while (InterlockedCompareExchange(&dwLockVar, 1, 0))
			{
		///		if (GetTickCount() - uTC >= uTimeOut_MS)
				{
					return FALSE;
				}

		///		Sleep(nSleepMS);
			}

			return TRUE;
		}

		inline BOOL TryLock()
		{
		//	return !InterlockedCompareExchange(&dwLockVar, 1, 0);
		}

	protected:
		LOCK_VAR_TYPE dwLockVar;

};
