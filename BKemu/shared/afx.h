// MS AFX replacement
#pragma once

#include "pch.h"
#include "CFile.h"

// TODO: some "vision"

#define TRACE0(x)

/*
 * MessageBox() Flags
 */
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#if(WINVER >= 0x0500)
#define MB_CANCELTRYCONTINUE        0x00000006L
#endif /* WINVER >= 0x0500 */

#define DECLARE_DYNAMIC(x)
#define DECLARE_MESSAGE_MAP(x)

typedef void *LPVOID;
typedef unsigned long HINSTANCE, HMENU;
typedef HINSTANCE HMODULE;      /* HMODULEs can be used in place of HINSTANCEs */
typedef struct tagCREATESTRUCTA {
    LPVOID      lpCreateParams;
    HINSTANCE   hInstance;
    HMENU       hMenu;
    HWND        hwndParent;
    int         cy;
    int         cx;
    int         y;
    int         x;
    LONG        style;
    LPCSTR      lpszName;
    LPCSTR      lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCTA, *LPCREATESTRUCTA;
typedef LPCREATESTRUCTA LPCREATESTRUCT;
typedef CREATESTRUCTA CREATESTRUCT;

typedef __int64 INT_PTR, *PINT_PTR;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

typedef const RECT *LPCRECT;

typedef struct _RECTL       /* rcl */
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECTL, *PRECTL, *LPRECTL;

typedef const RECTL *LPCRECTL;

/* Types use for passing & returning polymorphic values */
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;
typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, *NPPOINT, *LPPOINT;

/*
 * Message structure
 */
typedef struct tagMSG {
    HWND        hwnd;
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       time;
    POINT       pt;
#ifdef _MAC
    DWORD       lPrivate;
#endif
} MSG, *PMSG, *NPMSG, *LPMSG;

typedef unsigned long HBRUSH;

class CObject
{
public:
// Object model (types, destruction, allocation)
//	virtual CRuntimeClass* GetRuntimeClass() const;
	virtual ~CObject() {}  // virtual destructors are necessary
};

#define NULL 0

typedef struct tagNMHDR
{
    HWND      hwndFrom;
    UINT_PTR  idFrom;
    UINT      code;         // NM_ code
}   NMHDR;

class CPoint : public tagPOINT {
public:
	CPoint(int initX, int initY) { x = initX; y = initY; }
};
class CFont: CObject {};

class CEdit : CObject {};

#define afx_msg
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

/*============================================================================*/
// The device context

class CDC : public CObject {
};

class CRect : public CObject {
};

class CGdiObject : public CObject {};

class CRgn : public CGdiObject {
	DECLARE_DYNAMIC(CRgn)

public:
///	static CRgn* PASCAL FromHandle(HRGN hRgn);
///	operator HRGN() const;

// Constructors
	CRgn();
	BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
	BOOL CreateRectRgnIndirect(LPCRECT lpRect);
	BOOL CreateEllipticRgn(int x1, int y1, int x2, int y2);
	BOOL CreateEllipticRgnIndirect(LPCRECT lpRect);
	BOOL CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode);
///	BOOL CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts,	int nCount, int nPolyFillMode);
	BOOL CreateRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3);
	BOOL CreateFromPath(CDC* pDC);
///	BOOL CreateFromData(const XFORM* lpXForm, int nCount, const RGNDATA* pRgnData);

// Operations
	void SetRectRgn(int x1, int y1, int x2, int y2);
	void SetRectRgn(LPCRECT lpRect);
	int CombineRgn(const CRgn* pRgn1, const CRgn* pRgn2, int nCombineMode);
	int CopyRgn(const CRgn* pRgnSrc);
	BOOL EqualRgn(const CRgn* pRgn) const;
	int OffsetRgn(int x, int y);
	int OffsetRgn(POINT point);
	int GetRgnBox(LPRECT lpRect) const;
	BOOL PtInRegion(int x, int y) const;
	BOOL PtInRegion(POINT point) const;
	BOOL RectInRegion(LPCRECT lpRect) const;
///	int GetRegionData(LPRGNDATA lpRgnData, int nCount) const;

// Implementation
	virtual ~CRgn();
};

class CBrush : public CGdiObject
{
	DECLARE_DYNAMIC(CBrush)

public:
///	static CBrush* PASCAL FromHandle(HBRUSH hBrush);

// Constructors
	CBrush();
	CBrush(COLORREF crColor);             // CreateSolidBrush
	CBrush(int nIndex, COLORREF crColor); // CreateHatchBrush
///	explicit CBrush(CBitmap* pBitmap);          // CreatePatternBrush

	BOOL CreateSolidBrush(COLORREF crColor);
	BOOL CreateHatchBrush(int nIndex, COLORREF crColor);
///	BOOL CreateBrushIndirect(const LOGBRUSH* lpLogBrush);
///	BOOL CreatePatternBrush(CBitmap* pBitmap);
	BOOL CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage);
	BOOL CreateDIBPatternBrush(const void* lpPackedDIB, UINT nUsage);
	BOOL CreateSysColorBrush(int nIndex);

// Attributes
	operator HBRUSH() const;
///	int GetLogBrush(LOGBRUSH* pLogBrush);

// Implementation
public:
	virtual ~CBrush();
#ifdef _DEBUG
	void Dump(CDumpContext& dc) const override;
#endif
};

struct AFX_CMDHANDLERINFO;
class CCmdTarget : public CObject {
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
};

/*============================================================================*/
// Implementation of command routing
#define AFX_MSG_CALL
struct AFX_CMDHANDLERINFO
{
	CCmdTarget* pTarget;
	void (AFX_MSG_CALL CCmdTarget::*pmf)(void);
};

/*
 * 32 bit Dialog item template.
 */
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    short x;
    short y;
    short cx;
    short cy;
    WORD id;
} DLGITEMTEMPLATE;

struct _AFX_OCC_DIALOG_INFO
{
	DLGTEMPLATE* m_pNewTemplate;
	DLGITEMTEMPLATE** m_ppOleDlgItems;

	unsigned m_cItems;
	struct ItemInfo
	{
		unsigned nId;
		BOOL bAutoRadioButton;
	};
	ItemInfo *m_pItemInfo;
};

class CWnd : public CCmdTarget {
public:
	// special pre-creation and window rect adjustment hooks
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    CWnd* GetParent() const;
    CWnd* GetParentOwner() const;
    HWND GetSafeHwnd() const;
	// for translating Windows messages in main message pump
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	virtual BOOL DestroyWindow();
};

class CListCtrl : public CWnd {};
class CSpinButtonCtrl : public CWnd {};
class CComboBox : public CWnd {};
class CButton : public CWnd {};

class CView : public CWnd {
protected:
    virtual void OnInitialUpdate(); // called first time after construct
    virtual void OnDraw(CDC* pDC) = 0;
};

class CBasePane : public CWnd {
	virtual void AdjustLayout() {}
	virtual void RecalcLayout() {}
};

class CScreen;

class CPane : public CBasePane {
public:
	virtual BOOL OnShowControlBarMenu(CPoint point);
protected:
    virtual void OnAfterFloat();
};

typedef int AFX_DOCK_METHOD;
const int DM_UNKNOWN = 0;

class CDockablePane : public CPane {
protected:
    virtual void OnAfterDock(CBasePane* /*pBar*/, LPCRECT /*lpRect*/, AFX_DOCK_METHOD /*dockMethod*/);
    virtual void OnAfterDockFromMiniFrame() { OnAfterDock(this, NULL, DM_UNKNOWN); }
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

class CPaneDialog: public CDockablePane {

};

class CMFCToolTipCtrl: public CObject {
};

/*============================================================================*/
// Basic types
// abstract iteration position
struct __POSITION {};
typedef __POSITION* POSITION;

/*============================================================================*/
// CArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject
{
public:
// Construction
	CArray();

// Attributes
	INT_PTR GetSize() const;
	INT_PTR GetCount() const;
	BOOL IsEmpty() const;
	INT_PTR GetUpperBound() const;
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	const TYPE& GetAt(INT_PTR nIndex) const;
	TYPE& GetAt(INT_PTR nIndex);
	void SetAt(INT_PTR nIndex, ARG_TYPE newElement);
	const TYPE& ElementAt(INT_PTR nIndex) const;
	TYPE& ElementAt(INT_PTR nIndex);

	// Direct Access to the element data (may return NULL)
	const TYPE* GetData() const;
	TYPE* GetData();

	// Potentially growing the array
	void SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement);
	INT_PTR Add(ARG_TYPE newElement);
	INT_PTR Append(const CArray& src);
	void Copy(const CArray& src);

	// overloaded operator helpers
	const TYPE& operator[](INT_PTR nIndex) const;
	TYPE& operator[](INT_PTR nIndex);

	// Operations that move elements around
	void InsertAt(INT_PTR nIndex, ARG_TYPE newElement, INT_PTR nCount = 1);
	void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1);
	void InsertAt(INT_PTR nStartIndex, CArray* pNewArray);

// Implementation
protected:
	TYPE* m_pData;   // the actual array of data
	INT_PTR m_nSize;     // # of elements (upperBound - 1)
	INT_PTR m_nMaxSize;  // max allocated
	INT_PTR m_nGrowBy;   // grow amount

public:
	~CArray();
///	void Serialize(CArchive&) override;
#ifdef _DEBUG
	void Dump(CDumpContext&) const override;
	void AssertValid() const override;
#endif
};

/*============================================================================*/
// CList<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject
{
protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		TYPE data;
	};
public:
// Construction
	explicit CList(INT_PTR nBlockSize = 10);

// Attributes (head and tail)
	// count of elements
	INT_PTR GetCount() const;
	INT_PTR GetSize() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	TYPE& GetHead();
	const TYPE& GetHead() const;
	TYPE& GetTail();
	const TYPE& GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	TYPE RemoveHead();
	TYPE RemoveTail();

	// add before head or after tail
	POSITION AddHead(ARG_TYPE newElement);
	POSITION AddTail(ARG_TYPE newElement);

	// add another list of elements before head or after tail
	void AddHead(CList* pNewList);
	void AddTail(CList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	TYPE& GetNext(POSITION& rPosition); // return *Position++
	const TYPE& GetNext(POSITION& rPosition) const; // return *Position++
	TYPE& GetPrev(POSITION& rPosition); // return *Position--
	const TYPE& GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	TYPE& GetAt(POSITION position);
	const TYPE& GetAt(POSITION position) const;
	void SetAt(POSITION pos, ARG_TYPE newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
	POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const;
		// defaults to starting at the HEAD, return NULL if not found
	POSITION FindIndex(INT_PTR nIndex) const;
		// get the 'nIndex'th element (may return NULL)

// Implementation
protected:
	CNode* m_pNodeHead;
	CNode* m_pNodeTail;
	INT_PTR m_nCount;
	CNode* m_pNodeFree;
	struct CPlex* m_pBlocks;
	INT_PTR m_nBlockSize;

	CNode* NewNode(CNode*, CNode*);
	void FreeNode(CNode*);

public:
	~CList();
///	void Serialize(CArchive&) override;
#ifdef _DEBUG
	void Dump(CDumpContext&) const override;
	void AssertValid() const override;
#endif
};

typedef struct tagWINDOWPLACEMENT {
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
#ifdef _MAC
    RECT  rcDevice;
#endif
} WINDOWPLACEMENT;
typedef WINDOWPLACEMENT *PWINDOWPLACEMENT, *LPWINDOWPLACEMENT;

class CMenu : public CObject {};

class CCmdUI        // simple helper class
{
public:
// Attributes
	UINT m_nID;
	UINT m_nIndex;          // menu item or other index

	// if a menu item
	CMenu* m_pMenu;         // NULL if not a menu
	CMenu* m_pSubMenu;      // sub containing menu item
							// if a popup sub menu - ID is for first in popup

	// if from some other window
	CWnd* m_pOther;         // NULL if a menu or not a CWnd

// Operations to do in ON_UPDATE_COMMAND_UI
	virtual void Enable(BOOL bOn = TRUE);
	virtual void SetCheck(int nCheck = 1);   // 0, 1 or 2 (indeterminate)
	virtual void SetRadio(BOOL bOn = TRUE);
	virtual void SetText(LPCTSTR lpszText);

// Advanced operation
	void ContinueRouting();

// Implementation
	CCmdUI();
	BOOL m_bEnableChanged;
	BOOL m_bContinueRouting;
	UINT m_nIndexMax;       // last + 1 for iterating m_nIndex

	CMenu* m_pParentMenu;   // NULL if parent menu not easily determined
							//  (probably a secondary popup menu)

	BOOL DoUpdate(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
};

class CScrollBar : public CWnd {};

/*
 * Struct pointed to by WM_GETMINMAXINFO lParam
 */
typedef struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

#define ASSERT(f)
 //         DEBUG_ONLY((void) ((f) || !::AfxAssertFailedLine(THIS_FILE, __LINE__) || (AfxDebugBreak(), 0)))
