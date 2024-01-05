#pragma once


//#define __T(x)      L ## x
#define _T(x)  (const char*)x
#include "string.h"
#define MAX_CSTR_LEN 252

class CString { // string container from MicrosoftFM
    char m_pszData[MAX_CSTR_LEN]; // TODO: dynamic
	int sz;
public:
    CString() { m_pszData[0] = 0; sz = 0; }
	CString(const char* x) { sz = strlen(x); strncpy(m_pszData, x, MAX_CSTR_LEN); if (sz > MAX_CSTR_LEN) sz = MAX_CSTR_LEN; }
    bool IsEmpty() const { return !sz; }
	void MakeLower() { /*TODO*/ }
	int GetLength() const { return sz; }
	char operator[](int iChar) const { return m_pszData[iChar]; } // TODO: assetions/throw
	int Compare(const CString &psz) const { return strncmp(m_pszData, psz.m_pszData, MAX_CSTR_LEN); } // TODO:
	int Find(const char* x) const {
		const char* p = strnstr(m_pszData, x, MAX_CSTR_LEN);
		if (!p) return -1;
		return p - m_pszData;
	}
	char GetAt(int x) const { return m_pszData[x]; }
	bool LoadString(unsigned int nID) { /*TODO*/ return false; }; // A Windows string resource ID.
	int CollateNoCase(const char* psz) const { return strncmp(m_pszData, psz, MAX_CSTR_LEN); } // TODO: no case
	void Format(const char* f, int i) const { /*TODO*/ }
	void Format(const char* f, float flt) const { /*TODO*/ }
	void Format(const char* f, double flt) const { /*TODO*/ }
	friend CString operator+(const CString& str1, const char * psz2) {
		CString strResult( str1 );
		int sz = strlen(psz2);
		int j = str1.sz;
        for (int i = 0; i < sz && j < MAX_CSTR_LEN - 1; ++i) {
			strResult.m_pszData[j++] = psz2[i];
		}
		strResult.m_pszData[j] = 0;
		strResult.sz = j;
		return( strResult );
	}
	friend CString operator+(const CString& str1, const CString& psz2) {
		CString strResult( str1 );
		int j = str1.sz;
        for (int i = 0; i < psz2.sz && j < MAX_CSTR_LEN - 1; ++i) {
			strResult.m_pszData[j++] = psz2[i];
		}
		strResult.m_pszData[j] = 0;
		strResult.sz = j;
		return( strResult );
	}
	bool operator==(const CString& x) const { return Compare(x) == 0; }
	void Empty() { m_pszData[0] = 0; sz = 0; }
    // TODO:
};

int _ttoi(const CString& str);
float _ttof(const CString& str);

namespace fs {
    class path { // TODO:
	public:
        bool empty() const { return true; }
	};
};

typedef const char *LPCSTR, *PCSTR;
typedef char TCHAR;
#include "stdint.h"
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef unsigned long DWORD;
typedef unsigned int  UINT;
typedef DWORD   COLORREF;
typedef bool BOOL;

#define FALSE false
#define TRUE true
