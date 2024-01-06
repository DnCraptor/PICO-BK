#pragma once
//#include <atlimage.h>


enum class BKKeyType : int
{
	RESERVED,
	REGULAR,
	LSHIFT,
	RSHIFT,
	CTRL,
	ALT,
	ZAGL,
	STR,
	STOP,
	ENDARRAY
};

struct BKKey
{
	BKKeyType nType;
	int x1;
	int y1;
	int x2;
	int y2;
	uint8_t nScanCode;
	uint8_t nInterrupt;
	uint8_t nUniqueNum;
};

class CBKKbdButn : public CWnd
{
		DECLARE_DYNAMIC(CBKKbdButn)

	protected:
		HWND        m_hwndParent; // хэндл родительского окна
		CWnd       *m_cwndParent; // указатель на родительское окно

	//	CImage      m_Img;
	//	CImage      m_Img_p;   // Для битмапа-нажиматора
	//	CImage      m_ImgScr;

		int         m_cx;
		int         m_cy;
		int         m_imgW;
		int         m_imgH;
		int         m_nIdx;

		int         m_nAR2Index;
		int         m_nSUIndex;
		int         m_nLShiftIndex;
		int         m_nRShiftIndex;
		int         m_nArraySize;

		// Клавиатура
		bool        m_bAR2Pressed;     // флаг нажатия АР2
		bool        m_bSUPressed;      // флаг нажатия СУ
		bool        m_bShiftPressed;   // флаг нажатия шифта
		bool        m_bRShiftPressed;  // флаг нажатия правого шифта
		bool        m_bZaglPressed;    // true - загл (капслок включен), false - стр (капслок выключен)
		bool        m_bXlatMode;       // true - рус, false - лат

		bool        m_bControlKeyPressed;
		bool        m_bRegularKeyPressed;

		BKKey      *m_pBKKeyboardArray;

		static const BKKey m_ButnKbdKeys[];
		static const BKKey m_PlenKbdKeys[];

	public:
		CBKKbdButn(UINT nID = 0);
		virtual ~CBKKbdButn() override;
		virtual BOOL DestroyWindow() override;
		void        AdjustLayout();
		void        SetID(const UINT nID);
		inline int  GetWidth() const
		{
			return m_imgW;
		}
		inline int  GetHeihgt() const
		{
			return m_imgH;
		}

		inline void SetAR2Status(const bool b)
		{
			m_bAR2Pressed = b;
		}
		inline bool GetAR2Status() const
		{
			return m_bAR2Pressed;
		}
		inline void SetShiftStatus(const bool b)
		{
			m_bShiftPressed = b;
			// правый шифт не трогаем, чтобы не нажимались они одновременно
		}
		inline bool GetShiftStatus() const
		{
			return m_bShiftPressed || m_bRShiftPressed;
		}
		inline void SetSUStatus(const bool b)
		{
			m_bSUPressed = b;
		}
		inline bool GetSUStatus() const
		{
			return m_bSUPressed;
		}
		inline void SetCapitalStatus(bool b)
		{
			m_bZaglPressed = b;
		}
		inline bool GetCapitalStatus() const
		{
			return m_bZaglPressed;
		}
		inline void SetXLatStatus(const bool b)
		{
			m_bXlatMode = b;
		}
		inline bool GetXLatStatus() const
		{
			return m_bXlatMode;
		}

		uint8_t     GetUniqueKeyNum(const uint8_t nScancode) const;

	protected:
		virtual BOOL PreCreateWindow(CREATESTRUCT &cs) override;
		int         GetKeyIndex(const int x, const int y) const;
		int         GetKeyIndexById(const BKKeyType nType) const;
		int         GetArraySize() const;
		void       _FocusPressedkey(const int nIdx);
		uint8_t     TranslateScanCode(uint8_t nScanCode) const;
		void        ControlKeysUp();
		void        ClearObj();

		afx_msg LRESULT OnRealKeyDown(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnRealKeyUp(WPARAM wParam, LPARAM lParam);
		afx_msg void OnPaint();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg BOOL OnEraseBkgnd(CDC *pDC);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		DECLARE_MESSAGE_MAP()
};

