// DropTarget.h: interface for the CDropTarget class.
//
#pragma once

//#include <afxole.h>

class CDropTarget ///: public COleDropTarget
{
	public:
		CDropTarget() {};
		virtual ~CDropTarget() {}; /// override {};
/**
		virtual DROPEFFECT OnDropEx(CWnd *pWnd, COleDataObject *pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point) override
		{
			if (pDataObject->IsDataAvailable(CF_HDROP))
			{
				HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_HDROP);

				if (hGlobal)
				{
					auto p = reinterpret_cast<uint8_t *>(::GlobalLock(hGlobal));

					if (p)
					{
						auto pDropFiles = reinterpret_cast<LPDROPFILES>(p);
						p += pDropFiles->pFiles;
						CString strName;

						if (pDropFiles->fWide)
						{
							CString str(reinterpret_cast<LPCWSTR>(p));
							strName = str;
						}
						else
						{
							CString str(reinterpret_cast<LPCSTR>(p));
							strName = str;
						}

						::SendMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_DROP_TARGET, 0, reinterpret_cast<LPARAM>(&strName));
						::GlobalUnlock(hGlobal);
					}
				}

				return DROPEFFECT_COPY;
			}

			return DROPEFFECT_NONE;
		}

		virtual DROPEFFECT OnDragOver(CWnd *pWnd, COleDataObject *pDataObject, DWORD dwKeyState, CPoint point) override
		{
			if (pDataObject->IsDataAvailable(CF_HDROP))
			{
				return DROPEFFECT_COPY;
			}

			return DROPEFFECT_NONE;
		}***/
};
