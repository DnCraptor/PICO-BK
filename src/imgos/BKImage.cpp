#include "pch.h"
//#include "resource.h"
#include "BKImage.h"
//#include "BKListCtrl.h"
#include "StringUtil.h"
#include "BKFloppyImage_ANDos.h"
#include "BKFloppyImage_MKDos.h"
#include "BKFloppyImage_RT11.h"
#include "BKFloppyImage_Csidos3.h"
#include "BKFloppyImage_HCDos.h"
#include "BKFloppyImage_AODos.h"
#include "BKFloppyImage_NORD.h"
#include "BKFloppyImage_MicroDos.h"
#include "BKFloppyImage_Optok.h"
#include "BKFloppyImage_Holography.h"
#include "BKFloppyImage_DaleOS.h"
#include "BKFloppyImage_MSDOS.h"

#pragma warning(disable:4996)

std::string g_AddOpErrorStr[] =
{
	"Успешно.",
	"Файл образа не открыт.",
	"Недостаточно места на диске.",
	"Операция отменена пользователем.",
	"", // ошибку смотри в nImageErrorNumber
	"" // на будущее
};


CBKImage::CBKImage()
	: m_pFloppyImage(nullptr)
	, m_bCheckUseBinStatus(false)
	, m_bCheckUseLongBinStatus(false)
	, m_bCheckLogExtractStatus(false)
{
}

CBKImage::~CBKImage()
{
	ClearImgVector();
}



uint32_t CBKImage::Open(PARSE_RESULT &pr, const bool bLogDisk)
{
	if (bLogDisk)
	{
		PushCurrentImg();
	}
	else
	{
		Close();
	}

	switch (pr.imageOSType)
	{
		case IMAGE_TYPE::ANDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_ANDos>(pr);
			break;

		case IMAGE_TYPE::DXDOS:
			// тоже использует андосный класс. (он теперь универсальный,
			// понимает любой формат фат12 за исключением правильных каталогов)
			// как только понадобится, на его основе можно будет сделать настоящий понимальщик мсдосного формата
			m_pFloppyImage = std::make_unique<CBKFloppyImage_ANDos>(pr); // !!! неверно, нужно исправить работу с подкаталогами
			break;

		case IMAGE_TYPE::MSDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MSDOS>(pr); // !!! Недоделано!
			break;

		case IMAGE_TYPE::MKDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MKDos>(pr);
			break;

		case IMAGE_TYPE::NCDOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_HCDos>(pr);
			break;

		case IMAGE_TYPE::AODOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_AODos>(pr);
			break;

		case IMAGE_TYPE::NORD:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Nord>(pr);
			break;

		case IMAGE_TYPE::MIKRODOS:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_MicroDos>(pr);
			break;

		case IMAGE_TYPE::CSIDOS3:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Csidos3>(pr);
			break;

		case IMAGE_TYPE::RT11:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_RT11>(pr);
			break;

		case IMAGE_TYPE::OPTOK:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Optok>(pr);
			break;

		case IMAGE_TYPE::HOLOGRAPHY:
			m_pFloppyImage = std::make_unique<CBKFloppyImage_Holography>(pr);
			break;

		case IMAGE_TYPE::DALE:
			m_pFloppyImage = std::make_unique < СBKFloppyImage_DaleOS > (pr);
			break;

		default:
			ASSERT(false);
	}

	if (m_pFloppyImage)
	{
		m_pFloppyImage->OpenFloppyImage();
	}

	return ReOpen();
}

uint32_t CBKImage::ReOpen()
{
	if (m_pFloppyImage)
	{
		ASSERT(m_pListCtrl);
		m_ListCtrl.SetSpecificColumn(m_pFloppyImage->HasSpecificData());
		return m_pFloppyImage->EnableGUIControls();
	}

	return 0;
}


void CBKImage::Close()
{
	if (m_pFloppyImage)
	{
		m_pFloppyImage->CloseFloppyImage();
		m_pFloppyImage.reset();
	}
}

const std::string CBKImage::GetImgFormatName(IMAGE_TYPE nType)
{
	if (nType == IMAGE_TYPE::UNKNOWN && m_pFloppyImage)
	{
		nType = m_pFloppyImage->GetImgOSType();
	}

	return CBKParseImage::GetOSName(nType);
}

void CBKImage::ClearImgVector()
{
	Close();

	for (auto &bkdi : m_vpImages)
	{
		bkdi.reset();
	}

	m_vpImages.clear();
	m_PaneInfo.clear();
}

void CBKImage::PushCurrentImg()
{
	m_vpImages.push_back(std::move(m_pFloppyImage));
}

bool CBKImage::PopCurrentImg()
{
	if (!m_vpImages.empty())
	{
		if (m_pFloppyImage)
		{
			m_pFloppyImage.reset();
		}

		m_pFloppyImage = std::move(m_vpImages.back());
		m_vpImages.pop_back();
		return true;
	}

	return false;
}

CBKImage::ItemPanePos CBKImage::GetTopItemIndex()
{
	CBKImage::ItemPanePos pp;
	pp.nTopItem = m_ListCtrl.GetTopIndex();
	pp.nFocusedItem = m_ListCtrl.GetNextItem(-1, LVIS_FOCUSED);

	if (pp.nFocusedItem < 0) // если такого нет
	{
		pp.nFocusedItem = pp.nTopItem; // то переходим на начало
	}

	return pp;
}

bool CBKImage::ReadCurrentDir(CBKImage::ItemPanePos pp)
{
	ASSERT(m_pListCtrl);

	if (m_pFloppyImage->ReadCurrentDir())
	{
		std::vector<BKDirDataItem> *pLS = m_pFloppyImage->CurrDirectory();
		int item = 0;

		for (auto &fr : *pLS)
		{
			std::string str;
			m_ListCtrl.InsertItem(item, fr.strName.c_str());  // сюда можно добавить иконку

			if (fr.nAttr & FR_ATTR::DIRECTORY)
			{
				// теперь выведем тип записи
				switch (fr.nRecType)
				{
					case BKDirDataItem::RECORD_TYPE::UP:
						str = g_strUp;
						break;

					case BKDirDataItem::RECORD_TYPE::DIRECTORY:
						str = g_strDir;
						break;

					case BKDirDataItem::RECORD_TYPE::LINK:
						str = g_strLink;
						break;

					default:
						str = "???";
						break;
				}

				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_TYPE_POS, str.c_str());
				str.clear();
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_BLK_SIZE_POS, str.c_str());
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_ADDRESS_POS, str.c_str());
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_SIZE_POS, str.c_str());
			}
			else
			{
				// теперь выведем тип записи
				switch (fr.nRecType)
				{
					case BKDirDataItem::RECORD_TYPE::LOGDSK:
						str = "LOG";
						break;

					case BKDirDataItem::RECORD_TYPE::FILE:
						str = "";
						break;

					default:
						str = "???";
						break;
				}

				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_TYPE_POS, str.c_str());
				char buff[32];
				_snwprintf(buff, 32, "%d\0", fr.nBlkSize);
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_BLK_SIZE_POS, buff);
				_snwprintf(buff, 32, "%o\0", fr.nAddress);
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_ADDRESS_POS, buff);
				_snwprintf(buff, 32, "%o\0", fr.nSize);
				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_SIZE_POS, buff);
			}

			// выводим атрибуты
			str.clear();

			if (fr.nAttr & FR_ATTR::DELETED)
			{
				str.push_back('x');
			}

			if (fr.nAttr & FR_ATTR::BAD)
			{
				str.push_back('B');
			}

			if (fr.nAttr & FR_ATTR::TEMPORARY)
			{
				str.push_back('T');
			}

			if (fr.nAttr & FR_ATTR::DIRECTORY)
			{
				str.push_back('D');
			}

			if (fr.nAttr & FR_ATTR::LOGDISK)
			{
				str.push_back('d');
			}

			if (fr.nAttr & FR_ATTR::LINK)
			{
				str.push_back('L');            // с этим в андос тоже не очень, там одновременно и D и L,
			}                                   // где-то надо усложнять логику работы с атрибутами

			//if (fr.nAttr & FR_ATTR::VOLUMEID) // не будем это отображать, андос этим злоупотребляет
			//{                                 // хотя я точно помню, что в какой-то ОС есть конкретная запись каталога
			//  str.push_back('V');            // которая используется как метка диска.
			//}                                 // Ага. Это было в HC-DOC, но там я эту запись просто игнорирую.
			// вот когда перестану игнорировать, тогда и подумаем над тем, что делать с этим.
			if (fr.nAttr & FR_ATTR::ARCHIVE)
			{
				str.push_back('A');
			}

			if (fr.nAttr & FR_ATTR::SYSTEM)
			{
				str.push_back('S');
			}

			if (fr.nAttr & FR_ATTR::HIDDEN)
			{
				str.push_back('H');
			}

			if (fr.nAttr & FR_ATTR::PROTECTED)
			{
				str.push_back('P');
			}

			if (fr.nAttr & FR_ATTR::READONLY)
			{
				str.push_back('R');
			}

			m_ListCtrl.SetItemText(item, CBKListCtrl::LC_ATTR_POS, str.c_str());
			BKDirDataItem *nfr = std::addressof(fr);

			if (m_pFloppyImage->HasSpecificData())
			{
				if (fr.nRecType == BKDirDataItem::RECORD_TYPE::UP)
				{
					str.clear();
				}
				else
				{
					str = m_pFloppyImage->GetSpecificData(nfr);
				}

				m_ListCtrl.SetItemText(item, CBKListCtrl::LC_SPECIFIC_POS, str.c_str());
			}
			m_ListCtrl.SetItemData(item, reinterpret_cast<DWORD_PTR>(nfr));
		}

		m_ListCtrl.SortItems(CBKListCtrl::MyCompareProc, reinterpret_cast<DWORD_PTR>(m_pListCtrl));
		m_ListCtrl.SetFocus();
		m_ListCtrl.SetItemState(pp.nFocusedItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_ListCtrl.SetHotItem(pp.nFocusedItem);

		// и делаем верхний элемент обратно верхним
		if (pp.nTopItem > 0)
		{
			CRect r;
			m_ListCtrl.GetItemRect(pp.nTopItem, &r, LVIR_BOUNDS);
			m_ListCtrl.Scroll(CSize(0, pp.nTopItem * r.Height()));
		}

		// и ещё убедимся, чтобы выделенный элемент был видимым
		if (!m_ListCtrl.EnsureVisible(pp.nFocusedItem, FALSE))
		{
			CRect r;
			m_ListCtrl.GetItemRect(pp.nFocusedItem, &r, LVIR_BOUNDS);
			m_ListCtrl.Scroll(CSize(0, r.Height()));
		}

		OutCurrFilePath();
		std::string str = m_pFloppyImage->GetImageInfo();
		AfxGetMainWnd()->SendMessage(WM_OUT_CURR_IMG_INFO, 0, reinterpret_cast<LPARAM>(&str));
	}
	else
	{
		AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, static_cast<WPARAM>(0), static_cast<LPARAM>(m_pFloppyImage->GetErrorNumber()));
	}

	return false;
}

void CBKImage::ItemProcessing(int nItem, BKDirDataItem *fr)
{
	if (nItem < 0)
	{
		return;
	}

	CBKImage::ItemPanePos pp = GetTopItemIndex();
	m_PaneInfo.nTopItem = pp.nTopItem;
	m_PaneInfo.nCurItem = nItem;

	if (fr->nAttr & FR_ATTR::DIRECTORY)
	{
		if (fr->nRecType == BKDirDataItem::RECORD_TYPE::UP)
		{
			// выходим из подкаталога
			if (!StepUptoDir(fr)) // если после выхода текущая директория стала -1, то это выход ТОЛЬКО из образа
			{
				// .. на корневой директории вызывает закрытие текущего открытого образа
				if (S_OK != AfxGetMainWnd()->SendMessage(WM_OUT_OF_IMAGE, WPARAM(0), LPARAM(0)))
				{
					return;
				}
			}
		}
		else
		{
			// заходим в директорию
			StepIntoDir(fr);
		}

		m_ListCtrl.DeleteAllItems();
		CBKImage::ItemPanePos pp(m_PaneInfo.nTopItem, m_PaneInfo.nCurItem);
		ReadCurrentDir(pp);
	}
	else if (fr->nAttr & FR_ATTR::LOGDISK)
	{
		StepIntoDir(fr); // нужно, чтобы имя логдиска отображалось в пути, при входе в него.
		AfxGetMainWnd()->SendMessage(WM_PUT_INTO_LD, static_cast<WPARAM>(m_pFloppyImage->GetBaseOffset() + fr->nStartBlock * BLOCK_SIZE), static_cast<LPARAM>(fr->nSize));
	}
	else
	{
		// кликнули на файле, передаём запись файла в просмотрщик.
		if (m_pFloppyImage->GetImgOSType() == IMAGE_TYPE::RT11)
		{
			ViewFileRT11(fr);
		}
		else
		{
			ViewFile(fr); // проверка, файл это или нет, делается внутри.
		}
	}
}

void CBKImage::ViewSelected(const bool bViewAsText)
{
	const size_t uSelectedCount = m_ListCtrl.GetSelectedCount();
	int nItem = -1;

	if (uSelectedCount > 0)
	{
		// если что-то выделенное есть
		for (size_t i = 0; i < uSelectedCount; ++i)
		{
			nItem = m_ListCtrl.GetNextItem(nItem, LVNI_SELECTED);
			const auto fr = reinterpret_cast<BKDirDataItem *>(m_ListCtrl.GetItemData(nItem));

			if (bViewAsText)
			{
				if (m_pFloppyImage->GetImgOSType() == IMAGE_TYPE::RT11)
				{
					ViewFileRT11(fr);
				}
				else
				{
					ViewFile(fr); // проверка, файл это или нет, делается внутри.
				}
			}
			else
			{
				ViewFileAsSprite(fr); // проверка, файл это или нет, делается внутри.
			}
		}
	}
}

void CBKImage::StepIntoDir(BKDirDataItem *fr)
{
	if (m_pFloppyImage->ChangeDir(fr))
	{
		m_vSelItems.push_back(m_PaneInfo); // сохраняем текущее
		// заполняем новое.
		std::string name = strUtil::replaceChar(fr->strName, '/', '_'); // меняем '/' на '_', чтобы не путалась.
		m_PaneInfo.strCurrPath += name + "/";
		m_PaneInfo.nCurDir = fr->nDirNum;
		m_PaneInfo.nParentDir = fr->nDirBelong;
		m_PaneInfo.nTopItem = 0;
		m_PaneInfo.nCurItem = 0;
		OutCurrFilePath();
	}
	else
	{
		AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), static_cast<LPARAM>(m_pFloppyImage->GetErrorNumber()));
	}
}

// выход: true - обычно
//      false - выход из лог диска или образа

bool CBKImage::StepUptoDir(BKDirDataItem *fr)
{
	bool bRet = true;

	if (m_pFloppyImage->ChangeDir(fr))
	{
		if (m_pFloppyImage->GetCurrDirNum() == -1)
		{
			bRet = false;
		}

		// различие выхода из образа и из лог.диска - не пустой вектор.
		// если вектор пуст - то выход из образа, иначе - выход из лог.диска
		if (!m_vSelItems.empty())
		{
			m_PaneInfo = m_vSelItems.back();
			m_vSelItems.pop_back();
		}
		else
		{
			m_PaneInfo.clear();
			m_PaneInfo.nCurDir = -1;
		}

		OutCurrFilePath();
	}
	else
	{
		AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), static_cast<LPARAM>(m_pFloppyImage->GetErrorNumber()));
	}

	return bRet;
}

void CBKImage::OutCurrFilePath()
{
	AfxGetMainWnd()->SendMessage(WM_OUT_CURR_FILE_PATH, 0, reinterpret_cast<LPARAM>(&m_PaneInfo.strCurrPath));
}

void CBKImage::ExtractSelected(const fs::path &strOutPath)
{
	const size_t uSelectedCount = m_ListCtrl.GetSelectedCount();
	int nItem = -1;

	if (uSelectedCount > 0)
	{
		// если что-то выделенное есть
		m_strStorePath = strOutPath;

		for (size_t i = 0; i < uSelectedCount; ++i)
		{
			nItem = m_ListCtrl.GetNextItem(nItem, LVNI_SELECTED);
			const auto fr = reinterpret_cast<BKDirDataItem *>(m_ListCtrl.GetItemData(nItem));

			if (fr->nRecType == BKDirDataItem::RECORD_TYPE::UP)
			{
				// если встретился выход в родительскую директорию, то
				// его проигнорируем.
			}
			else
			{
				if (!ExtractObject(fr)) // тут либо директория. либо файл. директорию надо рекурсивно извлечь целиком
				{
					AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), static_cast<LPARAM>(m_pFloppyImage->GetErrorNumber()));
				}
			}
		}
	}
}

void CBKImage::RenameRecord(BKDirDataItem *fr)
{
	CBKImage::ItemPanePos pp = GetTopItemIndex();
	// 1. конвертируем абстрактную запись в реальную. только имя
	// 2. Изменяем соответствующую запись в каталоге
	m_pFloppyImage->RenameRecord(fr);
	// 3. Перечитываем каталог заново.
	m_ListCtrl.DeleteAllItems();
	ReadCurrentDir(pp);
	m_ListCtrl.SetFocus();
}

void CBKImage::DeleteSelected()
{
	CBKImage::ItemPanePos pp = GetTopItemIndex();
	const size_t uSelectedCount = m_ListCtrl.GetSelectedCount();
	int nItem = -1;

	if (uSelectedCount > 0)
	{
		// если что-то выделенное есть
		for (size_t i = 0; i < uSelectedCount; ++i)
		{
			nItem = m_ListCtrl.GetNextItem(nItem, LVNI_SELECTED);
			const auto fr = reinterpret_cast<BKDirDataItem *>(m_ListCtrl.GetItemData(nItem));

			if (fr->nRecType == BKDirDataItem::RECORD_TYPE::UP)
			{
				// если встретился выход в родительскую директорию, то
				// его проигнорируем.
			}
			else
			{
				ADDOP_RESULT res = DeleteObject(fr); // тут либо директория. либо файл. директорию надо рекурсивно удалить целиком после подтверждения

				if (res.bFatal)
				{
l_fatal:            // если фатальная - то просто выведем сообщение.

					if (res.nError == ADD_ERROR::IMAGE_ERROR)
					{
						AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(MB_ICONERROR), static_cast<LPARAM>(res.nImageErrorNumber));
					}
					else
					{
						AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(MB_ICONERROR), static_cast<LPARAM>(0x10000 | static_cast<int>(res.nError)));
					}

					break;
				}

				if (res.nError != ADD_ERROR::OK_NOERROR)
				{
					if (fr->nRecType == BKDirDataItem::RECORD_TYPE::DIRECTORY)
					{
						// обрабатываем ситуацию с директорией
						if (res.nImageErrorNumber == IMAGE_ERROR::FS_DIR_NOT_EMPTY) // если удалить не получается потому что не пустая
						{
							LRESULT definite = AfxGetMainWnd()->SendMessage(WM_SEND_MESSAGEBOX, WPARAM(MB_YESNO | MB_ICONINFORMATION), reinterpret_cast<LPARAM>(_T("Директория не пуста. Удалить рекурсивно всё её содержимое?")));

							switch (definite)
							{
								case IDYES:
									DeleteRecursive(fr); // удалять всё!
									break;

								default:
								case IDNO:
									break;
							}
						}
						else
						{
							goto l_fatal;
						}
					}
					else
					{
						// обрабатываем ситуацию с файлом
						if (res.nImageErrorNumber == IMAGE_ERROR::FS_FILE_PROTECTED) // если удалить не получается из-за атрибутов
						{
							LRESULT definite = AfxGetMainWnd()->SendMessage(WM_SEND_MESSAGEBOX, WPARAM(MB_YESNO | MB_ICONINFORMATION), reinterpret_cast<LPARAM>(_T("Файл защищён. Всё равно удалить?")));

							switch (definite)
							{
								case IDYES:
								{
									ADDOP_RESULT res3 = DeleteObject(fr, true);  // удаляем с форсированием

									if (res3.nError != ADD_ERROR::OK_NOERROR) // если всё равно не удалилось
									{
										goto l_fatal;
									}

									break;
								}

								default:
								case IDNO:
									break;
							}
						}
						else
						{
							goto l_fatal;
						}
					}
				}
			}
		}

		m_ListCtrl.DeleteAllItems();
		ReadCurrentDir(pp);
		m_ListCtrl.SetFocus();
	}
}

// рекурсивно удаляем директорию и всё её содержимое
// на входе - запись удаляемой директории
bool CBKImage::DeleteRecursive(BKDirDataItem *fr)
{
	bool bRet = true;

	if (fr->nAttr & FR_ATTR::DIRECTORY)
	{
		BKDirDataItem efr = *fr; // запись для выхода из директории
		m_pFloppyImage->ChangeDir(fr);
		// если директория, надо в неё зайти и получить список всех записей.
		BKDirDataItem cur_fr;
		bool bEnd = m_pFloppyImage->GetStartFileName(&cur_fr);

		if (bEnd)
		{
			do
			{
				if (cur_fr.strName != "..")
				{
					bRet = DeleteRecursive(&cur_fr); // обрабатываем все записи

					if (!bRet)
					{
						break;
					}
				}
			}
			while (bEnd = m_pFloppyImage->GetNextFileName(&cur_fr));
		}

		// тут надо выйти из директории.
		efr.nDirNum = fr->nDirBelong;
		efr.nRecType = BKDirDataItem::RECORD_TYPE::UP;
		efr.strName = "..";
		m_pFloppyImage->ChangeDir(&efr);
		bRet = m_pFloppyImage->DeleteDir(fr); // и под конец удаляем и саму директорию
	}
	else
	{
		bRet = m_pFloppyImage->DeleteFile(fr, true); // если файл - удаляем с форсированием
	}

	return bRet;
}

void CBKImage::SetStorePath(const fs::path &str)
{
	m_strStorePath = str;
}

bool CBKImage::ExtractObject(BKDirDataItem *fr)
{
	bool bRet = true;

	// если /*плохой или удалённый или*/ ссылка - ничего не делаем
	if (fr->nAttr & FR_ATTR::LINK)
	{
		return true;
	}

	if (fr->nAttr & FR_ATTR::DIRECTORY)
	{
		// если директория, надо в неё зайти и получить список всех записей.
		BKDirDataItem efr = *fr; // запись для выхода из директории
		m_pFloppyImage->ChangeDir(fr);
		BKDirDataItem cur_fr;
		const fs::path tmpStorePath = m_strStorePath;
		const fs::path strDirName = imgUtil::SetSafeName(fr->strName);
		SetStorePath(m_strStorePath / strDirName);
		std::error_code ec;

		if (!fs::create_directory(m_strStorePath, ec))
		{
			AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), static_cast<LPARAM>(IMAGE_ERROR::FS_CANNOT_CREATE_DIR));
			// bRet = false;
		}

		bool bEnd = m_pFloppyImage->GetStartFileName(&cur_fr);

		if (bEnd)
		{
			do
			{
				if (cur_fr.strName != "..")
				{
					bRet = ExtractObject(&cur_fr);

					if (!bRet)
					{
						break;
					}
				}
			}
			while (bEnd = m_pFloppyImage->GetNextFileName(&cur_fr));
		}

		// тут надо выйти из директории.
		efr.nDirNum = fr->nDirBelong;
		efr.nRecType = BKDirDataItem::RECORD_TYPE::UP;
		efr.strName = "..";
		m_pFloppyImage->ChangeDir(&efr);
		m_strStorePath = tmpStorePath;
	}
	else
	{
		// если файл
		bRet = ExtractFile(fr);
	}

	return bRet;
}

// процедура извлечения файла
bool CBKImage::ExtractFile(BKDirDataItem *fr)
{
	IMAGE_ERROR nErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	bool bRet = true;
	uint16_t BinHeader[2] {};

	// если /*плохой или удалённый или*/ директория - ничего не делаем
	if (fr->nAttr & FR_ATTR::DIRECTORY)
	{
		return bRet;
	}

	const bool bLogDisk = !!(fr->nAttr & FR_ATTR::LOGDISK);
	AnalyseFileStruct AFS;
	AFS.nAddr = fr->nAddress;
	AFS.nLen = fr->nSize;
	AFS.strName = imgUtil::SetSafeName(fr->strName);
	imgUtil::UNICODEtoBK(fr->strName, AFS.OrigName, 16, true);

	if (bLogDisk) // логическим дискам принудительно выставляем расширение dsk
	{
		const size_t p = AFS.strName.rfind('.'); // посмотрим, есть ли расширение

		if (p == std::string::npos) // если расширения нет
		{
			AFS.strName += g_pstrExts[DSK_EXT_IDX]; // то просто добавим.
		}
		else // если расширение есть
		{
			std::string ext = strUtil::strToLower(AFS.strName.substr(p)); // сделаем мелкие буквы

			if (ext != g_pstrExts[DSK_EXT_IDX]) // если расширение уже и так dsk, то не добавляем
			{
				AFS.strName += g_pstrExts[DSK_EXT_IDX]; // а если какое-то другое - добавим.
			}
		}
	}

	m_pFloppyImage->OnExtract(fr, AFS.strName); // обработка имени, уникальная для конкретной ОС
	// сделаем защиту. формат бин для больших файлов не будем делать
	const bool bUseBinFile = m_bCheckUseBinStatus && (AFS.nLen < 65536) && !bLogDisk;

	if (bUseBinFile)
	{
		const size_t p = AFS.strName.rfind('.'); // посмотрим, есть ли расширение

		if (p == std::string::npos) // если расширения нет
		{
			AFS.strName += g_pstrExts[BIN_EXT_IDX]; // то просто добавим.
		}
		else // если расширение есть
		{
			std::string ext = strUtil::strToLower(AFS.strName.substr(p)); // сделаем мелкие буквы

			if (ext != g_pstrExts[BIN_EXT_IDX]) // если расширение уже и так бин, то не добавляем
			{
				AFS.strName += g_pstrExts[BIN_EXT_IDX]; // а если какое-то другое - добавим.
			}
		}
	}

	if ((AFS.file = _tfopen((m_strStorePath / AFS.strName).c_str(), _T("w+b"))) != nullptr)
	{
		const int nLen = fr->nSize;
		auto Buffer = std::vector<uint8_t>(m_pFloppyImage->EvenSizeByBlock(nLen));
		const auto pBuffer = Buffer.data();

		if (pBuffer)
		{
			bRet = m_pFloppyImage->ReadFile(fr, pBuffer);

			if (bRet)
			{
				if (bUseBinFile)
				{
					// стандартный заголовок бин файла - два слова.
					// с адресом всё ясно, а вот длина больших файлов урезается
					BinHeader[0] = static_cast<uint16_t>(fr->nAddress);
					BinHeader[1] = static_cast<uint16_t>(nLen);
					fwrite(BinHeader, 1, sizeof(BinHeader), AFS.file);

					if (m_bCheckUseLongBinStatus)
					{
						fwrite(AFS.OrigName, 1, 16, AFS.file);
					}
				}

				AFS.nCRC = imgUtil::CalcCRC(pBuffer, fr->nSize);
				size_t ret = fwrite(pBuffer, 1, fr->nSize, AFS.file);

				if (bUseBinFile && m_bCheckUseLongBinStatus)
				{
					fwrite(&AFS.nCRC, 1, sizeof(uint16_t), AFS.file);
				}

				fflush(AFS.file);

				if (m_bCheckLogExtractStatus)
				{
					bRet = AnalyseExportFile(&AFS);

					if (!bRet)
					{
						nErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
					}
				}
			}
			else
			{
				nErrorNumber = m_pFloppyImage->GetErrorNumber();
			}
		}
		else
		{
			nErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
			bRet = false;
		}

		fclose(AFS.file);
	}
	else
	{
		nErrorNumber = IMAGE_ERROR::FILE_CANNOT_CREATE;
		bRet = false;
	}

	if (!bRet)
	{
		AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), static_cast<LPARAM>(nErrorNumber));
	}

	return bRet;
}

/*
анализ экспортируемого файла
вход:
    m_bCheckUseBinStatus - флаг, что добавляем как бин
поля в структуре AnalyseFileStruct:
    file - указатель на открытый ранее файл
    strName - имя файла
    nAddr - адрес загрузки
    nLen - длина файла
чтобы не писать отдельные функции для каждой ОС, анализировать будем уже извлечённый файл.
выход: true - успешно
       false - ошибка
*/
bool CBKImage::AnalyseExportFile(AnalyseFileStruct *a)
{
	bool bRet = true;
	IMAGE_ERROR nErrorNumber = IMAGE_ERROR::OK_NOERRORS;
	int nStartAddr = a->nAddr; // в идеале вот так, адрес запуска == адресу загрузки
	struct autorun
	{
		uint16_t nWord770;
		uint16_t nWord772;
		uint16_t nWord774;
	};
	autorun sAuto = { 0, 0, 0 };
	// сделаем защиту. формат бин для больших файлов не будем использовать
	const bool bUseBinFile = m_bCheckUseBinStatus && (a->nLen < 65536);

	if (0300 <= a->nAddr && a->nAddr < 01000)  // если есть блок автозапуска, или что-то похожее
	{
		if ((a->nAddr <= 0770) && (a->nAddr + a->nLen >= 0776)) // если файл перекрывает область, откуда брать адрес запуска
			// ведь это может быть просто кусок с адресом загрузки 0400 и длиной 0200
		{
			int nOffset = 0770 - a->nAddr; // адрес запуска будем брать из ячейки 0774 (хотя возможно надо из другой)

			// если содержимое ячеек 770, 772 и 774 равно между собой
			if (a->nAddr & 1)
			{
				nOffset++;    // на всякий случай, если адрес загрузки нечётный, скорректируем
			}

			if (bUseBinFile)
			{
				nOffset += 4;

				if (m_bCheckUseLongBinStatus)
				{
					nOffset += 16;
				}
			}

			if (0 == fseek(a->file, nOffset, SEEK_SET))
			{
				if (sizeof(sAuto) == fread(&sAuto, 1, sizeof(sAuto), a->file))
				{
					if ((sAuto.nWord770 == sAuto.nWord772) && (sAuto.nWord770 == sAuto.nWord774))
					{
						nStartAddr = int(sAuto.nWord774);
					}
				}
				else
				{
					bRet = false;
				}
			}
			else
			{
				bRet = false;
			}
		}
	}

	if (a->nAddr < 0100)
	{
		nStartAddr = 0;
	}

	FILE *f = _wfopen((m_strStorePath / "extractlog.txt").c_str(), "at");

	if (f)
	{
		std::string origName = imgUtil::BKToUNICODE(a->OrigName, 16, m_pFloppyImage->m_pKoi8tbl);
		fwprintf(f, "%-20s:\t%s\tload:%06o\tlen:%06o\tstart:%06o\n", a->strName.c_str(), origName.c_str(), a->nAddr, a->nLen, nStartAddr);
		fclose(f);
	}

	return bRet;
}



// возвращаем коды состояния и ошибок вызывающей функции. Она должна заниматься обработкой
// разных возникающих ситуаций
// сюда принимаем структуру sendfiledata и делаем из неё AbstractFileRecord
// bExistDir - флаг, когда мы пытаемся создать уже существующую директорию, - игнорировать ошибку
ADDOP_RESULT CBKImage::AddObject(const fs::path &findFile, bool bExistDir)
{
	ADDOP_RESULT ret;

	if (!m_pFloppyImage)
	{
		// файл образа не открыт
		ret.bFatal = true;
		ret.nError = ADD_ERROR::IMAGE_NOT_OPEN;
		return ret;
	}

	BKDirDataItem AFR;
	AFR.strName = findFile.filename().string();
	// в <filesystem> нет функционала для получения виндозных атрибутов файла
	// поэтому сделаем так:
	CFileStatus fst;

	if (CFile::GetStatus(findFile.c_str(), fst))
	{
		if (fst.m_attribute & CFile::Attribute::readOnly)
		{
			AFR.nAttr |= FR_ATTR::READONLY;
		}

		if (fst.m_attribute & CFile::Attribute::hidden)
		{
			AFR.nAttr |= FR_ATTR::HIDDEN;
		}

		if (fst.m_attribute & CFile::Attribute::system)
		{
			AFR.nAttr |= FR_ATTR::PROTECTED;
		}

		AFR.timeCreation = fst.m_ctime.GetTime();
	}

	// мы не можем сформировать данные:
	// nDirBelong - потому, что это внутренние данные образа
	// nBlkSize - потому, что это данные, зависящие от формата образа
	// nStartBlock - потому, что эти данные ещё неизвестны.

	if (fs::is_directory(findFile))
	{
		AFR.nAttr |= FR_ATTR::DIRECTORY;
		AFR.nRecType = BKDirDataItem::RECORD_TYPE::DIRECTORY;

		if (m_pFloppyImage->CreateDir(&AFR) || bExistDir)
		{
			if (!m_pFloppyImage->ChangeDir(&AFR))
			{
				// ошибка изменения директории:
				// IMAGE_ERROR::FS_NOT_SUPPORT_DIRS
				// IMAGE_ERROR::IS_NOT_DIR
				ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
				// обрабатывать теперь
				ret.nError = ADD_ERROR::IMAGE_ERROR;

				switch (ret.nImageErrorNumber)
				{
					case IMAGE_ERROR::FS_NOT_SUPPORT_DIRS:
						ret.bFatal = false;
						break;

					case IMAGE_ERROR::FS_IS_NOT_DIR:
					default:
						ret.bFatal = true;
						break;
				}
			}
		}
		else
		{
			// ошибка создания директории:
			// IMAGE_ERROR::FS_NOT_SUPPORT_DIRS - два варианта: игнорировать, остановиться
			// IMAGE_ERROR::CANNOT_WRITE_FILE
			// IMAGE_ERROR::FS_DIR_EXIST - такая директория уже существует, два варианта: игнорировать, остановиться
			ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
			// обрабатывать теперь
			ret.nError = ADD_ERROR::IMAGE_ERROR;

			switch (ret.nImageErrorNumber)
			{
				case IMAGE_ERROR::FS_NOT_SUPPORT_DIRS:
				case IMAGE_ERROR::FS_DIR_EXIST:
					ret.bFatal = false;
					break;

				default:
					ret.bFatal = true;
					break;
			}
		}
	}
	else
	{
		AFR.nRecType = BKDirDataItem::RECORD_TYPE::FILE;
		AFR.nSize = fs::file_size(findFile);

		if (AFR.nSize > m_pFloppyImage->GetImageFreeSpace())
		{
			// файл слишком большой
			ret.bFatal = true;
			ret.nError = ADD_ERROR::FILE_TOO_LARGE;
		}
		else
		{
			const auto nBufferSize = m_pFloppyImage->EvenSizeByBlock(AFR.nSize);
			auto Buffer = std::vector<uint8_t>(nBufferSize);
			const auto pBuffer = Buffer.data();

			if (pBuffer)
			{
				memset(pBuffer, 0, nBufferSize);
				AnalyseFileStruct AFS;

				if ((AFS.file = _tfopen(findFile.c_str(), _T("rb"))) != nullptr)
				{
					AFS.strName = findFile.stem();
					AFS.strExt = findFile.extension();
					AFS.nAddr = 01000;
					AFS.nLen = AFR.nSize;
					AFS.nCRC = 0;

					if (imgUtil::AnalyseImportFile(&AFS))
					{
						// если обнаружился формат .bin
						// если в заголовке бин было оригинальное имя файла
						if (AFS.OrigName[0])
						{
							// то восстановим оригинальное
							AFR.strName = strUtil::trim(imgUtil::BKToUNICODE(AFS.OrigName, 16, m_pFloppyImage->m_pKoi8tbl));
						}
						else
						{
							AFR.strName = AFS.strName + AFS.strExt;
						}
					}

					AFR.nAddress = AFS.nAddr;
					AFR.nSize = AFS.nLen;
					fread(pBuffer, 1, AFR.nSize, AFS.file);

					if (AFS.bIsCRC)
					{
						fread(&AFS.nCRC, 1, 2, AFS.file);

						if (AFS.nCRC != imgUtil::CalcCRC(pBuffer, AFR.nSize))
						{
							TRACE("CRC Mismatch!\n");
						}
					}
					else
					{
						AFS.nCRC = imgUtil::CalcCRC(pBuffer, AFR.nSize);
					}

					fclose(AFS.file);
					constexpr int MAX_SQUEEZE_ITERATIONS = 3;
					int nSqIter = MAX_SQUEEZE_ITERATIONS;
					bool bNeedSqueeze = false;
l_sque_retries:

					if (!m_pFloppyImage->WriteFile(&AFR, pBuffer, bNeedSqueeze))
					{
						// ошибка записи файла:
						// IMAGE_ERROR::CANNOT_WRITE_FILE
						// IMAGE_ERROR::FS_DISK_FULL
						// IMAGE_ERROR::FS_FILE_EXIST - Файл существует. два варианта: остановиться, удалить старый и перезаписать новый файл
						// IMAGE_ERROR::FS_STRUCT_ERR
						// IMAGE_ERROR::FS_CAT_FULL
						// IMAGE_ERROR::FS_DISK_NEED_SQEEZE - нужно сделать сквизирование, но от него отказались
						// и прочие ошибки позиционирования и записи
						ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
						// предполагаем ошибку безусловно фатальную
						ret.bFatal = true;

						// если нужно делать сквизирование
						if (bNeedSqueeze)
						{
							// если попытки не кончились ещё
							if (nSqIter > 0)
							{
								// выведем сообщение
								CString str;
								str.Format(_T("Попытка %d из %d.\n"), nSqIter, MAX_SQUEEZE_ITERATIONS);
								str += (m_pFloppyImage->GetImgOSType() == IMAGE_TYPE::OPTOK) ?
								       _T("Попробовать выполнить уплотнение?\nПоможет только если есть удалённые файлы.") :
								       _T("Диск сильно фрагментирован. Выполнить сквизирование?");
								LRESULT definite = AfxGetMainWnd()->SendMessage(WM_SEND_MESSAGEBOX, WPARAM(MB_YESNO | MB_ICONINFORMATION), reinterpret_cast<LPARAM>(str.GetString()));
								// если согласны
								nSqIter--;

								if (definite == IDYES)
								{
									goto l_sque_retries;
								}

								// если отказ, то сразу выход с фатальной ошибкой - диск полон
							}
							else
							{
								// итерации кончились, но так ничего и не получилось, то фатальная ошибка
								if (ret.nImageErrorNumber == IMAGE_ERROR::FS_DISK_NEED_SQEEZE)
								{
									ret.nImageErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
								}
							}

							// у сквизирования есть 2 причины: в каталоге нет места для записи, надо делать уплотнение
							// или место есть, но нету дырки нужного размера, тоже надо делать уплотнение
							// если и после этого записать ничего не получится, то диск точно полон.
							// обычно это происходит в случае, когда каталог заполнен и ничего не помогает.
						}
						else if (ret.nImageErrorNumber == IMAGE_ERROR::FS_FILE_EXIST)
						{
							// если файл существует, то ошибка не фатальная, нужно спросить
							// перезаписывать файл или нет
							ret.bFatal = false;
						}

						ret.nError = ADD_ERROR::IMAGE_ERROR;
					}
				}
			}
			else
			{
				// недостаточно памяти
				ret.bFatal = true;
				ret.nError = ADD_ERROR::IMAGE_ERROR;
				ret.nImageErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
			}
		}
	}

	ret.afr = AFR;
	return ret;
}
// нужна функция выхода из директории, когда добавляется новая директория.
void CBKImage::OutFromDirObject(BKDirDataItem *fr)
{
	fr->strName = "..";
	fr->nRecType = BKDirDataItem::RECORD_TYPE::UP;
	fr->nDirNum = fr->nDirBelong;
	m_pFloppyImage->ChangeDir(fr);
}


ADDOP_RESULT CBKImage::DeleteObject(BKDirDataItem *fr, bool bForce)
{
	ADDOP_RESULT ret;

	if (m_pFloppyImage == nullptr)
	{
		// файл образа не открыт
		ret.bFatal = true;
		ret.nError = ADD_ERROR::IMAGE_NOT_OPEN;
		return ret;
	}

	if (fr->nAttr & FR_ATTR::DIRECTORY)
	{
		const bool bRes = (bForce) ? DeleteRecursive(fr) : m_pFloppyImage->DeleteDir(fr);

		if (!bRes)
		{
			ret.nError = ADD_ERROR::IMAGE_ERROR;
			ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
			ret.bFatal = !(ret.nImageErrorNumber == IMAGE_ERROR::FS_DIR_NOT_EMPTY);
			ret.afr = *fr;
		}
	}
	else
	{
		if (!m_pFloppyImage->DeleteFile(fr, bForce))
		{
			ret.nError = ADD_ERROR::IMAGE_ERROR;
			ret.nImageErrorNumber = m_pFloppyImage->GetErrorNumber();
			ret.bFatal = !(ret.nImageErrorNumber == IMAGE_ERROR::FS_FILE_PROTECTED);
			ret.afr = *fr;
		}
	}

	return ret;
}


