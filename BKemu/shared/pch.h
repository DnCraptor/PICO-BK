#pragma once
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
DECLARE_HANDLE            (HWND);
typedef void * HANDLE;
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HWAVEOUT);
DECLARE_HANDLE            (HHOOK);
typedef HANDLE              HGLOBAL;

#define FAR
#define NEAR
#define CALLBACK
#define CONST const

typedef const char *LPCSTR, *PCSTR, *LPCTSTR, *PCXSTR;
typedef char *LPTSTR, *PXSTR;
typedef char *NPSTR, *LPSTR, *PSTR;
typedef char TCHAR;
typedef int64_t LONGLONG, __int64;
typedef uint64_t ULONGLONG, __uint64;
typedef unsigned long DWORD;
typedef unsigned int  UINT;
typedef DWORD COLORREF;
typedef bool BOOL;
typedef DWORD LCID;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef unsigned int UINT_PTR, *PUINT_PTR;
typedef long LONG, LONG_PTR, *PLONG_PTR;
typedef LONG_PTR LRESULT;
typedef LONG HRESULT;

#define S_OK    ((HRESULT)0L)

/*
 * original NT 32 bit dialog template:
 */
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE;
typedef CONST DLGTEMPLATE *LPCDLGTEMPLATEA;
typedef LPCDLGTEMPLATEA LPCDLGTEMPLATE;

typedef uint64_t ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

#define FALSE false
#define TRUE true
#define LOCALE_NOUSEROVERRIDE         0x80000000   // Not Recommended - do not use user overrides
#define _T(x)  (const char*)x
#define MAX_CSTR_LEN 252

class CString { // string container from MicrosoftFM
    char m_pszData[MAX_CSTR_LEN]; // TODO: dynamic
	int sz;
public:
    CString() { m_pszData[0] = 0; sz = 0; }
	CString(const char* x) { sz = strlen(x); strncpy(m_pszData, x, MAX_CSTR_LEN); if (sz > MAX_CSTR_LEN) sz = MAX_CSTR_LEN; }
    CString(const char* x, int _sz) : sz(_sz > MAX_CSTR_LEN ? MAX_CSTR_LEN : _sz) { strncpy(m_pszData, x, sz); m_pszData[sz] = 0; }
    bool IsEmpty() const { return !sz; }
	void MakeLower() { /*TODO*/ }
	int GetLength() const { return sz; }
	char operator[](int iChar) const { return m_pszData[iChar]; } // TODO: assetions/throw
	int Compare(const CString &psz) const { return strncmp(m_pszData, psz.m_pszData, MAX_CSTR_LEN); } // TODO:
	int Find(const char* x, int iStart = 0) const {
		const char* p = strnstr(m_pszData + iStart, x, MAX_CSTR_LEN);
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
    CString& operator+=(PCXSTR psz2) {
        int sz = strlen(psz2);
		int j = this->sz;
        for (int i = 0; i < sz && j < MAX_CSTR_LEN - 1; ++i) {
			m_pszData[j++] = psz2[i];
		}
		m_pszData[j] = 0;
		this->sz = j;
        return( *this );
    }
    CString& operator+=(const CString& psz2) {
		int j = this->sz;
        for (int i = 0; i < psz2.sz && j < MAX_CSTR_LEN - 1; ++i) {
			m_pszData[j++] = psz2[i];
		}
		m_pszData[j] = 0;
		this->sz = j;
        return( *this );
    }
	bool operator==(const CString& x) const { return Compare(x) == 0; }
	void Empty() { m_pszData[0] = 0; sz = 0; }
    PCXSTR GetString() const throw() { return( m_pszData ); }
    PXSTR GetBufferSetLength(int nLength) { sz = nLength; return m_pszData; } // TODO if more?
    PXSTR GetBuffer() { return m_pszData; }
    void ReleaseBuffer() { sz = strnlen(m_pszData, MAX_CSTR_LEN); } // TODO: ensure
    CString& Trim() { while(m_pszData[sz - 1] == ' ' || m_pszData[sz - 1] == '\t') { m_pszData[--sz] = 0; } return *this; } // TODO: trimLeft
    // Return the substring starting at index 'iFirst'
    CString Mid(int iFirst) const {  return CString(m_pszData + iFirst, sz - iFirst); }
    // Return the substring starting at index 'iFirst', with length 'nCount'
    CString Mid(int iFirst, int nCount) const { return CString(m_pszData + iFirst, nCount); }
    // TODO:
};

inline static int _ttoi(const CString& str) { return atoi(str.GetString()); }
inline static float _ttof(const CString& str) { return atof(str.GetString()); }
inline static unsigned long _tcstoul(const CString& str, char** p, int x) { return strtoul(str.GetString(), p, x); }

namespace fs {
    class path { // TODO:
	public:
        bool empty() const { return true; }
        void clear() noexcept { // set *this to the empty path
           // _Text._Orphan_all();
           // _Text.clear();
        }
	};
};

class COleDateTime {
public:
    inline bool ParseDateTime(
        LPCTSTR lpszDate,
	    DWORD dwFlags,
	    LCID lcid
    ) throw() {
        // TODO:
        return false;
    }
    inline CString Format(LPCTSTR pFormat) const {
        return CString("TBA");
    }
};

#include <string>
#include <stdexcept>

#if _HAS_NODISCARD
    #define _NODISCARD [[nodiscard]]
#else // ^^^ CAN HAZ [[nodiscard]] / NO CAN HAZ [[nodiscard]] vvv
    #define _NODISCARD
#endif // _HAS_NODISCARD

#define _CSTD ::
#define _THROW(x) // throw x (no exceptions?)
[[noreturn]] inline void _Throw_range_error(const char* const _Message) {
    _THROW(std::range_error{_Message});
}

namespace std {
    class mutex {};
    class condition_variable {};
    class thread {};
};

namespace std {
    template <class _Codecvt, class _Elem = wchar_t, class _Walloc = allocator<_Elem>, class _Balloc = allocator<char>>
    class wstring_convert {
    private:
        enum { _BUF_INC = 8, _BUF_MAX = 16 };
        void _Init(const _Codecvt* _Pcvt_arg = new _Codecvt) { // initialize the object
            _State = state_type{};
            _Pcvt  = _Pcvt_arg;
//            _Loc   = locale{_Loc, _Pcvt};
            _Nconv = 0;
        }
    public:
    using byte_string = basic_string<char, char_traits<char>, _Balloc>;
    using wide_string = basic_string<_Elem, char_traits<_Elem>, _Walloc>;
    using state_type  = typename _Codecvt::state_type;
    using int_type    = typename wide_string::traits_type::int_type;

    wstring_convert() : _Has_state(false), _Has_berr(false), _Has_werr(false) { // construct with no error strings
        _Init();
    }

    explicit wstring_convert(const _Codecvt* _Pcvt_arg)
        : _Has_state(false), _Has_berr(false), _Has_werr(false) { // construct with no error strings and codecvt
        _Init(_Pcvt_arg);
    }

    wstring_convert(const _Codecvt* _Pcvt_arg, state_type _State_arg)
        : _Has_state(true), _Has_berr(false), _Has_werr(false) { // construct with no error strings, codecvt, and state
        _Init(_Pcvt_arg);
        _State = _State_arg;
    }

    explicit wstring_convert(const byte_string& _Berr_arg)
        : _Berr(_Berr_arg), _Has_state(false), _Has_berr(true), _Has_werr(false) { // construct with byte error string
        _Init();
    }

    wstring_convert(const byte_string& _Berr_arg, const wide_string& _Werr_arg)
        : _Berr(_Berr_arg), _Werr(_Werr_arg), _Has_state(false), _Has_berr(true),
          _Has_werr(true) { // construct with byte and wide error strings
        _Init();
    }

    virtual ~wstring_convert() noexcept {}

    _NODISCARD size_t converted() const noexcept { // get conversion count
        return _Nconv;
    }

    _NODISCARD state_type state() const {
        return _State;
    }

    _NODISCARD wide_string from_bytes(char _Byte) { // convert a byte to a wide string
        return from_bytes(&_Byte, &_Byte + 1);
    }

    _NODISCARD wide_string from_bytes(const char* _Ptr) { // convert a NTBS to a wide string
        return from_bytes(_Ptr, _Ptr + _CSTD strlen(_Ptr));
    }

    _NODISCARD wide_string from_bytes(const byte_string& _Bstr) { // convert a byte string to a wide string
        const char* _Ptr = _Bstr.c_str();
        return from_bytes(_Ptr, _Ptr + _Bstr.size());
    }

    _NODISCARD wide_string from_bytes(
        const char* _First, const char* _Last) { // convert byte sequence [_First, _Last) to a wide string
        wide_string _Wbuf;
        wide_string _Wstr;
        const char* _First_sav = _First;

        if (!_Has_state) {
            _State = state_type{}; // reset state if not remembered
        }

        _Wbuf.append(_BUF_INC, _Elem{});
        for (_Nconv = 0; _First != _Last; _Nconv = static_cast<size_t>(_First - _First_sav)) {
            // convert one or more bytes
            _Elem* _Dest = &_Wbuf[0];
            _Elem* _Dnext;

            // test result of converting one or more bytes
            switch (_Pcvt->in(_State, _First, _Last, _First, _Dest, _Dest + _Wbuf.size(), _Dnext)) {
            case _Codecvt::partial:
            case _Codecvt::ok:
                if (_Dest < _Dnext) {
                    _Wstr.append(_Dest, static_cast<size_t>(_Dnext - _Dest));
                } else if (_Wbuf.size() < _BUF_MAX) {
                    _Wbuf.append(_BUF_INC, _Elem{});
                } else if (_Has_werr) {
                    return _Werr;
                } else {
                    _Throw_range_error("bad conversion");
                }

                break;

            case _Codecvt::noconv:
                for (; _First != _Last; ++_First) {
                    _Wstr.push_back(static_cast<_Elem>(static_cast<unsigned char>(*_First)));
                }

                break; // no conversion, just copy code values

            default:
                if (_Has_werr) {
                    return _Werr;
                } else {
                    _Throw_range_error("bad conversion");
                }
            }
        }
        return _Wstr;
    }

    _NODISCARD byte_string to_bytes(_Elem _Char) { // convert a wide char to a byte string
        return to_bytes(&_Char, &_Char + 1);
    }

    _NODISCARD byte_string to_bytes(const _Elem* _Wptr) { // convert a NTWCS to a byte string
        const _Elem* _Next = _Wptr;
        while (*_Next != 0) {
            ++_Next;
        }

        return to_bytes(_Wptr, _Next);
    }

    _NODISCARD byte_string to_bytes(const wide_string& _Wstr) { // convert a wide string to a byte string
        const _Elem* _Wptr = _Wstr.c_str();
        return to_bytes(_Wptr, _Wptr + _Wstr.size());
    }

    _NODISCARD byte_string to_bytes(
        const _Elem* _First, const _Elem* _Last) { // convert wide sequence [_First, _Last) to a byte string
        byte_string _Bbuf;
        byte_string _Bstr;
        const _Elem* _First_sav = _First;

        if (!_Has_state) {
            _State = state_type{}; // reset state if not remembered
        }

        _Bbuf.append(_BUF_INC, '\0');
        for (_Nconv = 0; _First != _Last; _Nconv = static_cast<size_t>(_First - _First_sav)) {
            // convert one or more wide chars
            char* _Dest = &_Bbuf[0];
            char* _Dnext;

            // test result of converting one or more wide chars
            switch (_Pcvt->out(_State, _First, _Last, _First, _Dest, _Dest + _Bbuf.size(), _Dnext)) {
            case _Codecvt::partial:
            case _Codecvt::ok:
                if (_Dest < _Dnext) {
                    _Bstr.append(_Dest, static_cast<size_t>(_Dnext - _Dest));
                } else if (_Bbuf.size() < _BUF_MAX) {
                    _Bbuf.append(_BUF_INC, '\0');
                } else if (_Has_berr) {
                    return _Berr;
                } else {
                    _Throw_range_error("bad conversion");
                }

                break;

            case _Codecvt::noconv:
                for (; _First != _Last; ++_First) {
                    _Bstr.push_back(static_cast<char>(static_cast<int_type>(*_First)));
                }

                break; // no conversion, just copy code values

            default:
                if (_Has_berr) {
                    return _Berr;
                } else {
                    _Throw_range_error("bad conversion");
                }
            }
        }
        return _Bstr;
    }

    wstring_convert(const wstring_convert&)            = delete;
    wstring_convert& operator=(const wstring_convert&) = delete;

private:
    const _Codecvt* _Pcvt; // the codecvt facet
//    locale _Loc; // manages reference to codecvt facet
    byte_string _Berr;
    wide_string _Werr;
    state_type _State; // the remembered state
    bool _Has_state;
    bool _Has_berr;
    bool _Has_werr;
    size_t _Nconv;
    };
};

