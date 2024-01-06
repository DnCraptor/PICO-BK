// ScriptRunner.h: interface for the CScriptRunner class.
//

#pragma once

class CMotherBoard;

using ScriptArgList = CList<CString, CString &>;

class CScriptRunner
{
		CMotherBoard       *m_pBoard;

		int                 m_nScriptLinePos;
		int                 m_nScriptLineLen;
		DWORD               m_nScriptCurrTick;
	///	CStdioFile          m_fileScript;
		fs::path            m_strScriptFile;
		CString             m_strScriptLine;
		UINT                m_nTickDelay;
		bool                m_bStopButton;
		bool                m_bRus;
		bool                m_bIsAr2Press; // флаг нажатия АР2
		bool                m_bHasScript;

		ScriptArgList       m_listArgs;

		bool                CheckEscChar(TCHAR ch);
		void                ParseNextChar(TCHAR ch);

	public:
		CScriptRunner();
		virtual ~CScriptRunner();

		void                AttachBoard(CMotherBoard *pBoard)
		{
			m_pBoard = pBoard;
		}

		void                SetScript(const fs::path &strScriptPath, const fs::path &strScriptFileName, bool bXlat);
		void                StopScript();
		void                SetArgumentList(ScriptArgList &ArgList);
		void                SetArgument(CString &strArg);
		bool                RunScript();
};
