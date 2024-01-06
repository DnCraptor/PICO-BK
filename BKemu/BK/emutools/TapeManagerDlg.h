#pragma once

// TapeManagerDlg.h : header file
//

#include <afxmt.h>
#include <Mmsystem.h>

#include <mutex>
#include <condition_variable>
#include <queue>
#include <list>
#include "BaseDialog.h"

#include "Tape.h"
#include "TapeUnit.h"

/////////////////////////////////////////////////////////////////////////////
// CTapeManagerDlg dialog

#define DIRECTSOUND_VERSION 0x1000

constexpr auto CAPTURE_BUFFERS_NUM = 5;

class CTapeManagerDlg : public CBaseDialog
{
		DECLARE_DYNAMIC(CTapeManagerDlg)

		enum TMLIST : int
		{
			COL_TYPE = 0,
			COL_NAME,
			COL_ADDR,
			COL_LEN,
			COL_TIME,
			COL_WAVELEN,
			COL_CRC,
			COL_PATH
		};
		enum TMCOMBO : int
		{
			NONE = 0,
			SAVE_AS_MSF,
			SAVE_AS_WAV,
			SAVE_AS_BIN
		};

		int                 m_nSilenceLength;

		int                 m_nTapePos;
		int                 m_nTapeCount;
		CString             m_strCurrentFolder;

		CComboBox           m_comboSaveAs;
		CComboBox           m_comboDevices;
		CListCtrl           m_list;

		CCriticalSection    m_csModifyBuffer;
		std::list<std::unique_ptr<SAMPLE_INT[]>> m_arCaptureBuffers;
		SAMPLE_INT         *m_pCurrCaptureBuffer; // указатель на текущий буфер из m_arCaptureBuffers

		bool                m_bCaptureMode;
		size_t              m_nCaptureOffset;   // в сэмплах
		size_t              m_nCaptureLength;   // в сэмплах

		bool                m_bRun;
		HWAVEIN             m_hWaveIn;
		WAVEHDR             m_WHDR[CAPTURE_BUFFERS_NUM];
		WAVEFORMATEX        m_wfx;

		CImageList          m_imgList;
		CTape               m_TapeReader;
		CFile               m_captureFile;
		fs::path             m_strCaptureFileName;
		CList<fs::path, fs::path &> m_strCaptureFileNameList; // список временных файлов, которые надо удалить при закрытии диалога

		std::thread         m_thrStatus;
		bool                m_bStatusExit;      // флаг завершения потока
		std::queue<CTapeUnit *> m_queueStat;    // очередь, для передачи данных потоку
		std::mutex          m_mutModifyStatus;  // мутекс для синхронизации
		std::mutex          m_mutThr;           // мутекс потока. залочен пока поток живой
		std::condition_variable m_cvStatusNew;  // условная переменная, чтоб будить поток, когда надо

		bool                m_bCanceled;        // флаг выполнения завершающих операций

		// Capture methods
		UINT                FillDevices();
		bool                OpenDevice();
		void                CloseDevice();
		void                UnInit();
		void                PrepareBuffers();
		void                UnPrepareBuffers();
		void                RemoveBuffers();
		void                InitBuffers();
		void                ProcessHeader(WAVEHDR *pHdr);

		void                StatusThread();

		inline size_t       GetBufferSampleLength() // возвращает длину буфера в сэмплах
		{
			return size_t(m_wfx.nSamplesPerSec);
		}

		void                ClearBuffers();

		void                DebugDraw(SAMPLE_INT *pBuffer, size_t nSizeInSamples);
		void                OutString(const CString &str);

		fs::path            GetTmpFilePath() const;
		bool                WriteBuffer(SAMPLE_INT *pBuff, size_t nSizeInSamples);
		bool                IsCaptureEnd(SAMPLE_INT *pBuff, size_t nSizeInSamples) const;

		// Get file status methods
		void                InitListCtrl();
		void                InitComboCtrl();
		void                InsertNewUnit(CTapeUnit *pUnit);
		void                RetrieveInfo(CTapeUnit *pUnit);
		void                FileListChanged(int nTapeCount);
		void                SetFolder(const CString &strFolder);
		void                DeleteUnits(bool bSelected);
		void                SaveUnitsAs(CTapeUnit::TAPETYPE type);

// Construction
	public:
		CTapeManagerDlg(CWnd *pParent = nullptr);   // standard constructor
		virtual ~CTapeManagerDlg() override;

// Dialog Data
		enum { IDD = IDD_TAPE_MANAGER };


// Overrides
	protected:
		virtual void DoDataExchange(CDataExchange *pDX) override;    // DDX/DDV support

// Implementation
	protected:
		virtual BOOL OnInitDialog() override;
		virtual void OnCancel() override;
		afx_msg LRESULT OnBufferReady(WPARAM, LPARAM);
		afx_msg LRESULT OnInfoReady(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnWaveInProc(WPARAM wParam, LPARAM lParam);
		afx_msg void OnCbnSelchangeTmDevices();
		afx_msg void OnTmLoad();
		afx_msg void OnGetdispinfoTmList(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnTmBrowse();
		afx_msg void OnTmRemove();
		afx_msg void OnUpdateTmRemove(CCmdUI *pCmdUI);
		afx_msg void OnSelendokTmSaveAs();
		afx_msg void OnUpdateSelendokTmSaveAs(CCmdUI *pCmdUI);
		afx_msg void OnRclickTmList(NMHDR *pNMHDR, LRESULT *pResult);
		afx_msg void OnTmSelectall();
		afx_msg void OnTmUnselectall();
		afx_msg void OnDestroy();
		afx_msg void OnClose();
		DECLARE_MESSAGE_MAP()
};

