#include "utils.h"
#include <shobjidl.h> //COM
#include <fstream> //file stream input output
#include <algorithm> //sort

std::wstring ReadRegistry(HKEY root, std::wstring key, std::wstring name)
{
	HKEY hKey;
	if (RegOpenKeyEx(root, key.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return 0;

	DWORD type;
	DWORD cbData;
	if (RegQueryValueEx(hKey, name.c_str(), NULL, &type, NULL, &cbData) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return 0;
	}

	if (type != REG_SZ)
	{
		RegCloseKey(hKey);
		return 0;
	}

	std::wstring value(cbData / sizeof(wchar_t), L'\0');
	if (RegQueryValueEx(hKey, name.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&value[0]), &cbData) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return 0;
	}

	RegCloseKey(hKey);

	size_t firstNull = value.find_first_of(L'\0');
	if (firstNull != std::string::npos)
		value.resize(firstNull);

	return value;
}

int CALLBACK AlphComp(LPARAM lp1, LPARAM lp2, LPARAM sortParam)
{
	int Column, PreviousColumn;
	bool Ascending = sortParam > 0;

	BreakLPARAM(abs(sortParam) - 1, PreviousColumn, Column);

	std::wstring s1(64, '\0');
	std::wstring s2(64, '\0');

	ListView_GetItemText(GetDlgItem(hReport, IDC_BLIST), lp1, Column, (LPWSTR)s1.c_str(), 64);
	ListView_GetItemText(GetDlgItem(hReport, IDC_BLIST), lp2, Column, (LPWSTR)s2.c_str(), 64);
	return Column == 1 ? (Ascending ? CompareBolts(s1, s2) : CompareBolts(s2, s1)) : (Ascending ? CompareStrs(s1, s2) : CompareStrs(s2, s1));
}

void OnSortHeader(LPNMLISTVIEW pLVInfo)
{
	static int Column = 0;
	static BOOL SortAscending = TRUE;
	LPARAM lParamSort;

	if (pLVInfo->iSubItem != INT_MAX)
	{
		if (pLVInfo->iSubItem == Column)
			SortAscending = !SortAscending;
		else
		{
			Column = pLVInfo->iSubItem;
			SortAscending = TRUE;
		}
	}

	lParamSort = MakeLPARAM(0, Column);
	lParamSort++;

	if (!SortAscending)
		lParamSort = -lParamSort;

	ListView_SortItems(pLVInfo->hdr.hwndFrom, AlphComp, lParamSort);
	UpdateBListParams(pLVInfo->hdr.hwndFrom);
}

BOOL GetLastWriteTime(LPTSTR pszFilePath, SYSTEMTIME &stUTC)
{
	FILETIME ftCreate, ftAccess, ftWrite;

	HANDLE hFile = CreateFileW(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
			return FALSE;
		FileTimeToSystemTime(&ftWrite, &stUTC);
		CloseHandle(hFile);
	}
	else return FALSE;
	return TRUE;
}

LPARAM MakeLPARAM(const unsigned int &i, const unsigned int &j, const bool &negative)
{
	int k;
	negative ? k = -1 : k = 1;
	return (LPARAM)((i * 10000 + j) * k);
}

void BreakLPARAM(const LPARAM &lparam, int &i, int &j)
{
	if (lparam > 9999)
	{
		i = lparam / 10000;
		j = lparam - (i * 10000);
	}
	else
	{
		i = 0;
		j = lparam;
	}
}

int CompareStrs(const std::wstring &str1, const std::wstring &str2)
{
	unsigned int max = str1.size();
	if (str1.size() > str2.size())
		max = str2.size();

	for (unsigned int i = 0; i < max; i++)
	{
		if (str1[i] > str2[i]) return 1;
		if (str1[i] < str2[i]) return -1;
	}
	return (str1.size() > max ? -1 : 0);
}

int CompareBolts(const std::wstring &str1, const std::wstring &str2)
{
	int diff1, diff2, bolts1, bolts2, maxbolts1, maxbolts2;
	std::string::size_type pos;

	pos = str1.find('/');
	if (pos != std::string::npos)
	{

		bolts1 = static_cast<int>(::strtol(WStringToString(str1.substr(0, pos - 1)).c_str(), NULL, 10));
		maxbolts1 = static_cast<int>(::strtol(WStringToString(str1.substr(pos + 1)).c_str(), NULL, 10));
		diff1 = maxbolts1 - bolts1;
	}
	else if (str2.size() > 1) return 1;

	pos = str2.find('/');
	if (pos != std::string::npos)
	{
		bolts2 = static_cast<int>(::strtol(WStringToString(str2.substr(0, pos - 1)).c_str(), NULL, 10));
		maxbolts2 = static_cast<int>(::strtol(WStringToString(str2.substr(pos + 1)).c_str(), NULL, 10));
		diff2 = maxbolts2 - bolts2;
	}
	else return str1.size() > 1 ? -1 : 0;

	if (diff1 == diff2)
	{
		if (maxbolts1 == maxbolts2) return 0;
		return maxbolts1 > maxbolts2 ? -1 : 1;

	}
	return diff1 > diff2 ? -1 : 1;
}

void UpdateBListParams(HWND &hList)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;

	unsigned int max = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);

	for (unsigned int i = 0; i < max; i++)
	{
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.lParam = i;
		ListView_SetItem(hList, (LPARAM)&lvi);
	}
}

bool DatesMatch(SYSTEMTIME stUTC)
{
	if (filedateinit)
	{
		if (filedate.wMilliseconds != stUTC.wMilliseconds) return FALSE;
		if (filedate.wSecond != stUTC.wSecond) return FALSE;
		if (filedate.wMinute != stUTC.wMinute) return FALSE;
		if (filedate.wHour != stUTC.wHour) return FALSE;
		if (filedate.wDay != stUTC.wDay) return FALSE;
		if (filedate.wMonth != stUTC.wMonth) return FALSE;
		if (filedate.wYear != stUTC.wYear) return FALSE;
		return TRUE;
	}
	else return TRUE;
}

void OpenFileDialog()
{
	std::wstring fpath;
	SYSTEMTIME stUTC;
	BOOL date;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		// Try to set default folder to LocalLow
		IShellItem *DefaultFolder = NULL;
		std::wstring defaultpath;
		wchar_t *buffer;
		errno_t err = _wdupenv_s(&buffer, NULL, _T("USERPROFILE"));

		if (!err)
		{
			defaultpath = buffer;
			defaultpath += _T("\\AppData\\LocalLow\\Amistech\\My Summer Car\\");
			HRESULT hrr = SHCreateItemFromParsingName((PCWSTR)defaultpath.c_str(), NULL, IID_PPV_ARGS(&DefaultFolder));

			if (SUCCEEDED(hrr))
			{
				pFileOpen->SetDefaultFolder(DefaultFolder);
				DefaultFolder->Release();
			}
		}
		free(buffer);
		buffer = NULL;

		/*
		IKnownFolderManager *fm;
		HRESULT hrr = CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fm));
		if (SUCCEEDED(hrr))
		{
		IKnownFolder *LocalLow = NULL;
		IShellItem *DefaultFolder = NULL;

		SHCreateItemFromParsingName((PCWSTR)defaultpath.c_str(), nullptr, IID_PPV_ARGS(&DefaultFolder));

		hrr = fm->GetFolder(FOLDERID_LocalAppDataLow, &LocalLow);
		if (SUCCEEDED(hrr))
		{
		hrr = LocalLow->GetShellItem(0, IID_IShellItem, (void**)&DefaultFolder);
		if (SUCCEEDED(hrr))
		{
		pFileOpen->SetFolder(DefaultFolder);
		DefaultFolder->Release();
		}
		LocalLow->Release();
		}
		fm->Release();
		}
		*/

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					fpath = pszFilePath;
					date = GetLastWriteTime(pszFilePath, stUTC);
					CoTaskMemFree(pszFilePath);
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();

		if (!fpath.empty())
		{
			filepath = fpath;
			if (date)
			{
				filedate = stUTC;
				filedateinit = TRUE;
			}
			else
			{
				MessageBox(hDialog, GLOB_STRS[6].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
				filedateinit = FALSE;
			}
			InitMainDialog(hDialog);
		}
	}
}

void UnloadFile()
{
	SetWindowText(hDialog, (LPCWSTR)Title);
	HWND hList1 = GetDlgItem(hDialog, IDC_List);
	HWND hList2 = GetDlgItem(hDialog, IDC_List2);
	SendMessage(hList1, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(hList2, LVM_DELETEALLITEMS, 0, 0);
	ListView_SetBkColor(hList1, (COLORREF)GetSysColor(COLOR_MENU));
	ListView_SetBkColor(hList2, (COLORREF)GetSysColor(COLOR_MENU));
	ListView_DeleteColumn(hList1, 0);
	ListView_DeleteColumn(hList2, 1);
	ListView_DeleteColumn(hList2, 0);

	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT1), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT1), WM_SETTEXT, 0, (LPARAM)_T(""));
	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT2), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT2), WM_SETTEXT, 0, (LPARAM)_T(""));
	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT3), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)_T(""));
	entries.clear();
	variables.clear();
	indextable.clear();
	carparts.clear();

	HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
	EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 4), MF_GRAYED);
}

bool CanClose()
{
	if (WasModified())
	{
		TCHAR buffer[128];
		memset(buffer, 0, 128);
		swprintf(buffer, 128, GLOB_STRS[7].c_str(), filepath.c_str());

		switch (MessageBox(NULL, buffer, Title, MB_YESNOCANCEL | MB_ICONWARNING))
		{
		case IDNO:
			return TRUE;
		case IDYES:
			if (!SaveFile())
			{
				MessageBox(hDialog, GLOB_STRS[6].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		case IDCANCEL:
			return FALSE;
		default:
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

//checks if values were modified
inline bool WasModified()
{
	for (unsigned int i = 0; i < variables.size(); i++)
	{
		if (variables[i].modified)
		{
			return TRUE;
		}
	}
	return FALSE;
}

int SaveFile()
{
	using namespace std;

	SYSTEMTIME stUTC;
	GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC);
	if (!DatesMatch(stUTC)) return 5;

	ifstream iwc(filepath, ifstream::binary);
	if (!iwc.is_open()) return 1;

	iwc.seekg(0, iwc.end);
	unsigned int length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	std::string buffer(length + 1, '\0');
	iwc.read((char*)buffer.data(), length);

	if (iwc.bad() || iwc.fail())
	{
		iwc.close();
		return 1;
	}
	iwc.close();

	for (unsigned int i = 0; i < variables.size(); i++)
	{
		if (variables[i].modified)
		{
			std::string str;
			switch (variables[i].type)
			{
			case ID_STRING:
			case ID_STRINGL:
				char intstr[4];
				for (unsigned int j = 0; j < 4; ++j)
				{
					intstr[j] = (char)buffer[variables[i].pos + j];
				}
				int old_length;
				old_length = *((int*)&intstr);

				FormatValue(str, to_string(variables[i].value.size() + ValIndizes[variables[i].type][0] + 1), ID_INT);
				buffer.replace(variables[i].pos, str.size(), str);
				buffer.replace(variables[i].pos + 4 + ValIndizes[variables[i].type][0], old_length - ValIndizes[variables[i].type][0] - 1, variables[i].value);
				break;
			case ID_FLOAT:
			case ID_BOOL:
			case ID_COLOR:
			case ID_INT:
			case ID_TRANSFORM:
				buffer.replace(variables[i].pos + 4 + ValIndizes[variables[i].type][0], ValIndizes[variables[i].type][1], variables[i].value);
			}
		}
	}
	string bla = WStringToString(filepath);
	const char* bbla = bla.c_str();

	if (MakeBackup)
	{
		std::wstring fpath = filepath;
		size_t found = fpath.find_last_of(_T("."));
		if (found == std::string::npos) fpath += _T("_backup01");
		else fpath = fpath.insert(found, _T("_backup01"));

		string bfstr = WStringToString(fpath);
		const char* bcstr = bfstr.c_str();

		if (::rename(bbla, bcstr) != 0)
		{
			if (errno != 17) return 2;
			while (errno == 17)
			{
				std::string _tmp = to_string(stoi(bfstr.substr(found + 7, 2), nullptr, 10) + 1);
				if (_tmp.size() == 1) _tmp.insert(0, "0");
				bfstr.replace(found + 7, 2, _tmp);
				bcstr = bfstr.c_str();
				::rename(bbla, bcstr);
			}
		}
	}
	else
	{
		if (::remove(bbla) != 0) return 3;
	}

	ofstream owc(filepath, ofstream::binary);
	if (!owc.is_open()) return 4;

	owc.write(buffer.c_str(), buffer.size());
	if (owc.bad() || owc.fail())
	{
		owc.close();
		return 4;
	}
	owc.close();

	if (GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC))
	{
		filedate = stUTC;
		filedateinit = TRUE;
	}

	return 0;
}

void InitMainDialog(HWND hwnd)
{
	UnloadFile();
	if (!ScoopSavegame())
	{
		MessageBox(hDialog, _T("Could not load savefile!"), _T("Error"), MB_OK | MB_ICONERROR);
	}
	else
	{
		TCHAR buffer[128];
		memset(buffer, 0, 128);
		swprintf(buffer, 128, _T("%s - [%s]"), Title, filepath.c_str());
		SetWindowText(hDialog, (LPCWSTR)buffer);

		LVCOLUMN lvc;

		ListView_SetBkColor(GetDlgItem(hwnd, IDC_List), (COLORREF)GetSysColor(COLOR_WINDOW));
		ListView_SetBkColor(GetDlgItem(hwnd, IDC_List2), (COLORREF)GetSysColor(COLOR_WINDOW));

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 0; lvc.pszText = _T(""); lvc.cx = 230; lvc.fmt = LVCFMT_LEFT;
		SendMessage(GetDlgItem(hwnd, IDC_List), LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		unsigned int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
		std::wstring str(size, '\0');
		GetWindowText(GetDlgItem(hwnd, IDC_FILTER), (LPWSTR)str.data(), size);
		str.resize(size - 1);
		UpdateList(str);

		RECT rekt;
		GetWindowRect(GetDlgItem(hwnd, IDC_List2), &rekt);
		long width = rekt.right - rekt.left - 4;

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 1; lvc.pszText = _T("Key"); lvc.cx = 150; lvc.fmt = LVCFMT_LEFT;
		SendMessage(GetDlgItem(hwnd, IDC_List2), LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 0; lvc.pszText = _T("Value"); lvc.cx = (width - 150); lvc.fmt = LVCFMT_LEFT;
		SendMessage(GetDlgItem(hwnd, IDC_List2), LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
		EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_ENABLED);
		EnableMenuItem(menu, GetMenuItemID(menu, 4), MF_ENABLED);

		memset(buffer, 0, 128);
		swprintf(buffer, 128, GLOB_STRS[11].c_str(), 0);
		SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)buffer);
	}
}

inline bool PartIsStuck(std::wstring &stuckStr, const std::vector<unsigned int> &boltlist, const unsigned int &tightness)
{
	unsigned int boltstate = 0;
	for (unsigned int k = 0; k < boltlist.size(); k++)
	{
		boltstate += boltlist[k];
	}

	if (tightness > boltstate)
	{
		TCHAR buffer[32];
		memset(buffer, 0, 32);
		swprintf(buffer, 32, _T(" (%d != %d)"), boltstate, tightness);
		stuckStr = BListSymbols[1];
		stuckStr += buffer;
		return TRUE;
	}
	return FALSE;
}

void PopulateBList(HWND hwnd, const CarPart *part, unsigned int &item, Overview *ov)
{
	HWND hList3 = GetDlgItem(hwnd, IDC_BLIST);

	std::wstring stuckStr = BListSymbols[0];
	std::wstring boltStr = BListSymbols[0];
	LVITEM lvi;

	if (part->iBolts != UINT_MAX && part->iTightness != UINT_MAX)
	{
		unsigned int bolts = 0, maxbolts = 0;
		std::vector<unsigned int> boltlist;

		if (BinToBolts(variables[part->iBolts].value, bolts, maxbolts, boltlist))
		{
			unsigned int tightness = static_cast<unsigned int>(BinToFloat(variables[part->iTightness].value));
			TCHAR buffer[32];
			memset(buffer, 0, 32);
			swprintf(buffer, 32, _T("%d / %d"), bolts, maxbolts);
			boltStr = buffer;

			ov->numMaxBolts += maxbolts;
			ov->numBolts += bolts;

			bool invalid = (part->iInstalled == UINT_MAX);
			if (!invalid)
				invalid = (variables[part->iInstalled].value[0] == 0x01);
			if (invalid)
			{
				ov->numLooseBolts += maxbolts - bolts;

				if (maxbolts == bolts)
					ov->numFixed++;
				else
					ov->numLoose++;
			}

			// tightness special case

			if (!partSCs.empty())
			{
				for (unsigned int j = 0; j < partSCs.size(); j++)
				{
					if (partSCs[j].id == 0)
					{
						if (partSCs[j].str == part->name)
						{
							int offset = static_cast<int>(::strtol((WStringToString(partSCs[j].param)).c_str(), NULL, 10));
							tightness >= 8 ? tightness += -offset : tightness = 0;
							break;
						}
					}
				}
			}

			if (PartIsStuck(stuckStr, boltlist, tightness)) ov->numStuck++;
		}
	}
	else
	{
		bool invalid = (part->iInstalled == UINT_MAX);
		if (!invalid)
			invalid = (variables[part->iInstalled].value[0] == 0x01);
		if (invalid)
			ov->numFixed++;
	}

	ov->numParts++;

	bool invalid = (part->iInstalled == UINT_MAX);
	if (!invalid)
		invalid = (variables[part->iInstalled].value[0] == 0x01);
	if (invalid)
		ov->numInstalled++;


	if (part->iDamaged != UINT_MAX)
		if (variables[part->iDamaged].value[0] == 0x01)
			ov->numDamaged++;

	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
	lvi.iItem = item; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)part->name.c_str(), lvi.lParam = item;
	SendMessage(hList3, LVM_INSERTITEM, 0, (LPARAM)&lvi);

	lvi.mask = LVIF_TEXT | LVIF_STATE; lvi.state = 0; lvi.stateMask = 0;
	lvi.iItem = item; lvi.iSubItem = 1; lvi.pszText = (LPWSTR)boltStr.c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);

	lvi.iItem = item; lvi.iSubItem = 2; lvi.pszText = (LPWSTR)((part->iDamaged == UINT_MAX) ? BListSymbols[0] : ((variables[part->iDamaged].value[0] == 0x01) ? BListSymbols[1] : BListSymbols[0])).c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);

	lvi.iItem = item; lvi.iSubItem = 3; lvi.pszText = (LPWSTR)((part->iInstalled != UINT_MAX) ? ((variables[part->iInstalled].value[0] == 0x01) ? BListSymbols[1] : BListSymbols[0]) : BListSymbols[2]).c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);

	lvi.iItem = item; lvi.iSubItem = 4; lvi.pszText = (LPWSTR)stuckStr.c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);
	item++;
}

void UpdateBDialog()
{
	unsigned int item = 0;
	Overview ov;
	HWND hList3 = GetDlgItem(hReport, IDC_BLIST);
	SendMessage(hList3, WM_SETREDRAW, 0, 0);
	SendMessage(hList3, LVM_DELETEALLITEMS, 0, 0);

	for (unsigned int i = 0; i < carparts.size(); i++)
	{
		PopulateBList(hReport, &carparts[i], item, &ov);
	}

	SendMessage(hList3, WM_SETREDRAW, 1, 0);

	NMHDR nmhdr = { hList3 , 0 , 0 };
	NMLISTVIEW nmlv = { nmhdr , 0 , INT_MAX , 0 , 0 , 0 , 0 , 0 };
	LPNMLISTVIEW pLVInfo = &nmlv;
	OnSortHeader(pLVInfo);

	//overview
	UpdateBOverview(hReport, &ov);
}

void UpdateBOverview(HWND hwnd, Overview *ov)
{
	int statics[] = { IDC_BT1 , IDC_BT2 , IDC_BT3 , IDC_BT4, IDC_BT5, IDC_BT6, IDC_BT7, IDC_BT8 };

	for (unsigned int i = 0; i < 8; i++)
	{
		ClearStatic(GetDlgItem(hwnd, statics[i]), hwnd);
	}

	TCHAR buffer[128];
	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[18].c_str(), ov->numMaxBolts);
	SendMessage(GetDlgItem(hwnd, statics[0]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[19].c_str(), ov->numBolts);
	SendMessage(GetDlgItem(hwnd, statics[1]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[20].c_str(), ov->numLooseBolts);
	SendMessage(GetDlgItem(hwnd, statics[2]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[21].c_str(), ov->numInstalled, ov->numParts);
	SendMessage(GetDlgItem(hwnd, statics[3]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[22].c_str(), ov->numFixed);
	SendMessage(GetDlgItem(hwnd, statics[4]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[23].c_str(), ov->numLoose);
	SendMessage(GetDlgItem(hwnd, statics[5]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[24].c_str(), ov->numDamaged);
	SendMessage(GetDlgItem(hwnd, statics[7]), WM_SETTEXT, 0, (LPARAM)buffer);

	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[25].c_str(), ov->numStuck);
	SendMessage(GetDlgItem(hwnd, statics[6]), WM_SETTEXT, 0, (LPARAM)buffer);
}

void UpdateParent(const int &group)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	unsigned int max = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);

	for (unsigned int i = 0; i < max; i++)
	{
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		ListView_GetItem(hList, (LPARAM)&lvi);

		if (lvi.lParam >= 10000) lvi.lParam += -10000;
		if (lvi.lParam == group)
		{
			bool modified = FALSE;
			for (unsigned int j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == group && variables[j].modified)
				{
					modified = TRUE;
					break;
				}
			}
			if (modified) lvi.lParam += 10000;

			ListView_SetItem(hList, (LPARAM)&lvi);
		}
	}
}

void UpdateChild(const int &vIndex, std::string &str)
{
	variables[vIndex].value = str;
	for (unsigned int i = 0; i < indextable.size(); i++)
	{
		if (indextable[i].index2 == vIndex)
		{
			std::wstring out;
			FormatString(out, str, variables[vIndex].type);
			ListView_SetItemText(GetDlgItem(hDialog, IDC_List2), indextable[i].index1, 0, (LPWSTR)out.c_str());
			break;
		}
	}
}

void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin)
{
	std::string str;
	FormatValue(str, viewstr, variables[vIndex].type);
	if (bin.size() != 0) str = bin;

	if (variables[vIndex].static_value == str)
	{
		variables[vIndex].modified = FALSE;
		if (variables[vIndex].value != str)
		{
			UpdateChild(vIndex, str);
			UpdateParent(variables[vIndex].group);
			/*
			variables[vIndex].value = str;
			std::wstring out;
			FormatString(out, str, variables[vIndex].type);
			ListView_SetItemText(GetDlgItem(hDialog, IDC_List2), lIndex, 0, (LPWSTR)out.c_str());

			HWND hList = GetDlgItem(hDialog, IDC_List);

			int ListIndex = ListView_GetSelectionMark(hList);
			LVITEM lvi;
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ListIndex;
			ListView_GetItem(hList, (LPARAM)&lvi);
			if (lvi.lParam >= 10000) lvi.lParam += -10000;
			ListView_SetItem(hList, (LPARAM)&lvi);
			//UpdateWindow(hList);
			//ListView_RedrawItems(hList, ListIndex, ListIndex);
			*/
		}
	}
	else
	{
		variables[vIndex].modified = TRUE;

		UpdateChild(vIndex, str);
		UpdateParent(variables[vIndex].group);
		/*
		variables[vIndex].value = str;
		std::wstring out;
		FormatString(out, str, variables[vIndex].type);
		ListView_SetItemText(GetDlgItem(hDialog, IDC_List2), lIndex, 0, (LPWSTR)out.c_str());



		int index = ListView_GetSelectionMark(GetDlgItem(hDialog, IDC_List));
		ListView_RedrawItems(GetDlgItem(hDialog, IDC_List), index, index);

		HWND hList = GetDlgItem(hDialog, IDC_List);
		int ListIndex = ListView_GetSelectionMark(hList);
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iItem = ListIndex;
		ListView_GetItem(hList, (LPARAM)&lvi);
		if (lvi.lParam <= 10000) lvi.lParam += 10000;
		ListView_SetItem(hList, (LPARAM)&lvi);

		//UpdateWindow(hList);
		//ListView_RedrawItems(hList, ListIndex, ListIndex);
		*/
	}

	unsigned int num = 0;
	for (unsigned int i = 0; i < variables.size(); i++)
	{
		if (variables[i].modified)
		{
			num++;
		}
	}
	HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
	num > 0 ? EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_ENABLED) : EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);
	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT3), hDialog);
	TCHAR buffer[128];
	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[11].c_str(), num);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)buffer);

	SYSTEMTIME stUTC;
	GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC);
	if (!DatesMatch(stUTC))
	{
		MessageBox(hDialog, GLOB_STRS[17].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
	}

}

void UpdateList(const std::wstring &str)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	SendMessage(hList, WM_SETREDRAW, 0, 0);
	SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);

	LVITEM lvi;

	if (str.size() == 0)
	{
		SendMessage(hList, LVM_SETITEMCOUNT, entries.size(), 0);
		for (unsigned int i = 0; i < entries.size(); i++)
		{
			unsigned int param = i;
			unsigned int j;
			for (j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == i) break;
			}
			for (j; j < variables.size(); j++)
			{
				if (variables[j].group != i) break;
				if (variables[j].modified)
				{
					param += 10000;
					break;
				}
			}
			lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
			lvi.iItem = i; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)entries[i].c_str(); lvi.lParam = (LPARAM)param;
			SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);
		}
	}
	else
	{
		unsigned int* indizes = new unsigned int[variables.size()];
		unsigned int index = 0;

		for (unsigned int i = 0; i < variables.size(); i++)
		{
			if (ContainsStr(variables[i].key, str))
			{
				if (index == 0)
				{
					indizes[index] = variables[i].group;
					index++;
				}
				else if (indizes[index - 1] != variables[i].group)
				{
					indizes[index] = variables[i].group;
					index++;
				}
			}
		}
		SendMessage(hList, LVM_SETITEMCOUNT, index, 0);

		for (unsigned int i = 0; i < index; i++)
		{
			unsigned int param = indizes[i];
			unsigned int j;
			for (j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == indizes[i]) break;
			}
			for (j; j < variables.size(); j++)
			{
				if (variables[j].group != indizes[i]) break;
				if (variables[j].modified)
				{
					param += 10000;
					break;
				}
			}
			lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
			lvi.iItem = i; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)entries[indizes[i]].c_str(); lvi.lParam = (LPARAM)param;
			SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&lvi);
		}

		delete[] indizes;
	}
	SendMessage(hList, WM_SETREDRAW, 1, 0);

	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT1), hDialog);

	TCHAR buffer[128];
	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[10].c_str(), variables.size(), SendMessage(hList, LVM_GETITEMCOUNT, 0, 0));
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT1), WM_SETTEXT, 0, (LPARAM)buffer);
}

void ClearStatic(HWND hStatic, HWND hDlg)
{
	RECT rekt;
	GetClientRect(hStatic, &rekt);
	InvalidateRect(hStatic, &rekt, TRUE);
	MapWindowPoints(hStatic, hDlg, (POINT *)&rekt, 2);
	RedrawWindow(hDlg, &rekt, NULL, RDW_ERASE | RDW_INVALIDATE);
}

//check if vector is valid
//0 == valid, 1 == wrong amount of elements, 2 == float NaN, 3 == negative floats , 4 == not 0><1 , 5 not -1><1 , 29 not a valid angle
unsigned int VectorStrToBin(std::wstring &str, const unsigned int &size, std::string &bin, const bool allownegative, const bool normalized, const bool eulerconvert, const QTRN *oldq, const std::string &oldbin)
{
	std::string::size_type found = 0;
	int *indizes = new int[size + 1]{ -1 };
	indizes[size] = str.size();
	unsigned int seperators = 0;

	//kill shitespaces
	for (unsigned int i = 0; i <= str.size(); i++)
	{
		if (isspace(str[i])) str.erase(i, 1);
	}

	//wrong amount of elements?
	while (TRUE)
	{
		std::string::size_type _i = str.substr(found).find(_T(","));
		if (_i == std::string::npos)
		{
			break;
		}
		else
		{
			found += _i;
			seperators++;
			indizes[seperators] = found;
			found++;
		}
	}
	if (seperators != size - 1) return 1;

	//any illegal characters?
	for (unsigned int i = 0; i != size; i++)
	{
		if (!IsValidFloatStr(str.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1))) return 2;
		if (allownegative == 0)
		{
			if (str.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1)[0] == 45) return 3;
		}
		if (normalized)
		{
			float x = static_cast<float>(::strtod(WStringToString(str.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1)).c_str(), NULL));
			if (allownegative)
			{
				if (x > 1 || x < -1) return 5;
			}
			else
			{
				if (x > 1 || x < 0) return 4;
			}
		}
	}
	if (eulerconvert)
	{
		ANGLES a;
		QTRN q;
		a.yaw = static_cast<double>(::strtod(WStringToString(str.substr(indizes[0] + 1, indizes[1] - indizes[0] - 1)).c_str(), NULL));
		a.pitch = static_cast<double>(::strtod(WStringToString(str.substr(indizes[1] + 1, indizes[2] - indizes[1] - 1)).c_str(), NULL));
		a.roll = static_cast<double>(::strtod(WStringToString(str.substr(indizes[2] + 1, indizes[3] - indizes[2] - 1)).c_str(), NULL));
		if (a.yaw > 180 || a.yaw < -180 || a.pitch > 180 || a.pitch < -180 || a.roll > 180 || a.roll < -180) return 29;
		q = EulerToQuat(&a);

		if (!QuatEqual(&q, oldq))
		{
			double list[] = { q.x, q.y, q.z, q.w };
			for (unsigned int i = 0; i < 4; i++)
			{
				std::string element = std::to_string(list[i]);
				TruncFloatStr(element);
				element = FloatStrToBin(element);
				bin += element;
			}
		}
		else
		{
			bin += oldbin;
		}
	}
	else
	{
		//assemble binary
		std::string element;
		for (unsigned int i = 0; i != size; i++)
		{
			element = WStringToString(str.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1));
			TruncFloatStr(element);
			element = FloatStrToBin(element);
			bin += element;
		}
	}
	delete[] indizes;
	return 0;
}
/*
template <typename TSTRING>
bool FetchDataFileParameters(const TSTRING &str, std::string::size_type &startStr, std::string::size_type &endStr)
{
	startStr = str.find('"');
	if (startStr == std::string::npos) return FALSE;
	startStr++;
	if (startStr >= str.size()) return FALSE;

	endStr = str.substr(startStr).find('"');
	if (endStr == std::string::npos || endStr == 0) return FALSE;
	return TRUE;
}
*/

void BatchProcessUninstall()
{
	for (unsigned int i = 0; i < carparts.size(); i++)
	{
		if (carparts[i].iInstalled != UINT_MAX)
		{
			UpdateValue(bools[0], carparts[i].iInstalled);
		}

		if (carparts[i].iBolted != UINT_MAX)
		{
			UpdateValue(bools[0], carparts[i].iBolted);
		}

		if (carparts[i].iTightness != UINT_MAX)
		{
			UpdateValue(_T("0"), carparts[i].iTightness);
		}

		if (carparts[i].iBolts != UINT_MAX)
		{
			unsigned int bolts = 0, maxbolts = 0;
			std::vector<unsigned int> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				for (unsigned int j = 0; j < boltlist.size(); j++)
				{
					boltlist[j] = 0;
				}
				UpdateValue(_T(""), carparts[i].iBolts, BoltsToBin(boltlist));
			}
		}

	}
}

void BatchProcessStuck()
{
	for (unsigned int i = 0; i < carparts.size(); i++)
	{
		if ((carparts[i].iBolts != UINT_MAX) && (carparts[i].iTightness != UINT_MAX))
		{
			unsigned int bolts = 0, maxbolts = 0;
			std::vector<unsigned int> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				unsigned int boltstate = 0;
				unsigned int tightness = static_cast<unsigned int>(BinToFloat(variables[carparts[i].iTightness].value));

				for (unsigned int j = 0; j < boltlist.size(); j++)
				{
					boltstate += boltlist[j];
				}

				// adjust boltstate for special cases, if there are any
				if (!partSCs.empty())
				{
					for (unsigned int j = 0; j < partSCs.size(); j++)
					{
						if (partSCs[j].id == 0)
						{
							if (partSCs[j].str == carparts[i].name)
							{
								int offset = static_cast<int>(::strtol((WStringToString(partSCs[j].param)).c_str(), NULL, 10));
								boltstate >= 8 ? boltstate += offset : boltstate = 0;
								break;
							}
						}
					}
				}

				if (tightness > boltstate)
					UpdateValue(std::to_wstring(boltstate), carparts[i].iTightness);
			}
		}
	}
}

void BatchProcessDamage(bool all)
{
	for (unsigned int i = 0; i < carparts.size(); i++)
	{
		if (carparts[i].iDamaged != UINT_MAX)
		{
			bool invalid = (carparts[i].iInstalled == UINT_MAX);
			if (!invalid)
			{
				invalid = (variables[carparts[i].iInstalled].value[0] == 0x01);
			}
			if (invalid || all)
			{
				if (variables[carparts[i].iDamaged].value[0] == 0x01)
					UpdateValue(bools[0], carparts[i].iDamaged);
			}
		}
	}
}

void BatchProcessBolts(bool fix)
{
	for (unsigned int i = 0; i < carparts.size(); i++)
	{
		if (carparts[i].iTightness != UINT_MAX && carparts[i].iBolts != UINT_MAX)
		{
			bool invalid = carparts[i].iInstalled == UINT_MAX;
			if (!invalid)
			{
				invalid = (variables[carparts[i].iInstalled].value[0] == 0x01);
			}
			if (invalid || !fix)
			{
				unsigned int bolts = 0, maxbolts = 0;
				std::vector<unsigned int> boltlist;

				if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
				{
					if (maxbolts != bolts || !fix)
					{
						int boltstate = fix ? 8 : 0;
						std::vector<unsigned int> boltlist;
						for (unsigned int j = 0; j < maxbolts; j++)
						{
							boltlist.push_back(boltstate);
						}
						int tightness = boltlist.size() * boltstate;

						// adjust tightness for special cases, if there are any
						if (!partSCs.empty() && fix)
						{
							for (unsigned int j = 0; j < partSCs.size(); j++)
							{
								if (partSCs[j].id == 0)
								{
									if (partSCs[j].str == carparts[i].name)
									{
										int offset = static_cast<int>(::strtol((WStringToString(partSCs[j].param)).c_str(), NULL, 10));
										tightness >= 8 ? tightness += offset : tightness = 0;
										break;
									}
								}
							}
						}

						if (carparts[i].iBolted != UINT_MAX) UpdateValue(bools[fix], carparts[i].iBolted);
						UpdateValue(_T(""), carparts[i].iBolts, BoltsToBin(boltlist));
						UpdateValue(std::to_wstring(tightness), carparts[i].iTightness);
					}
				}
			}
		}
	}
}

bool BinToBolts(const std::string &str, unsigned int &bolts, unsigned int &maxbolts, std::vector<unsigned int> &boltlist)
{
	std::string::size_type start = 0, end = 0, offset = 0;
	bool valid = TRUE;
	while (valid)
	{
		start = str.substr(offset).find("(");
		end = str.substr(offset).find(")");
		valid = start != std::string::npos || end != std::string::npos;
		offset++;
		if (valid)
		{
			unsigned int boltstate = static_cast<unsigned int>(::strtol(str.substr(offset + start, end - start).c_str(), NULL, 10));
			boltlist.push_back(boltstate);
			bolts += (boltstate == 8 ? 1 : 0);
			maxbolts++;
		}
		offset += end;
	}
	return (maxbolts != 0) ? TRUE : FALSE;
}

std::string BoltsToBin(std::vector<unsigned int> &bolts)
{
	if (bolts.size() == 0) return "";
	std::string bin;
	bin += IntToBin(bolts.size());

	for (unsigned int i = 0; i < bolts.size(); i++)
	{
		std::string s = "int(" + std::to_string(bolts[i]) + ")";
		char c = char(s.length());
		bin += c + s;
	}

	return bin;
}

bool IsValidFloatStr(const std::wstring &str)
{
	for (unsigned int i = 0; i < str.size(); i++)
	{
		if (!isdigit(str[i]))
		{
			if (str[i] != 46 && str[i] != 45 && !isspace(str[i]))
				return FALSE;
		}
	}

	float f;
	return swscanf_s(str.c_str(), _T("%f%f"), &f, &f) == 1 ? TRUE : FALSE;
}

void TruncFloatStr(std::wstring &str)
{
	std::string::size_type found = str.find(_T("."));
	if (found != std::string::npos)
	{
		found = str.find_last_not_of(_T("0\t\f\v\n\r"));
		if (found != std::string::npos)
		{
			if (str[found] == 46) found--;
			str.resize(found + 1);
		}
		else
			str.clear();
	}
}

void TruncFloatStr(std::string &str)
{
	std::string::size_type found = str.find(".");
	if (found != std::string::npos)
	{
		found = str.find_last_not_of("0\t\f\v\n\r");
		if (found != std::string::npos)
		{
			if (str[found] == 46) found--;
			str.resize(found + 1);
		}
		else
			str.clear();
	}
}

//format variables vector value for display
template <typename TSTRING>
void FormatString(std::wstring &str, const TSTRING &value, const unsigned int &type)
{
	str = StringToWString(value);

	switch (type)
	{
	case ID_FLOAT:
	{
		str = BinToFloatStr(str);
		TruncFloatStr(str);
		break;
	}
	case ID_BOOL:
	{
		(str.c_str())[0] == 0x00 ? str = _T("false") : str = _T("true");
		break;
	}
	case ID_COLOR:
	{
		std::wstring tstr;
		for (unsigned int i = 0; i < 4; i++)
		{
			std::wstring astr;
			astr = BinToFloatStr(str.substr(0 + (4 * i), 4));
			TruncFloatStr(astr);
			tstr.append(astr + _T(", "));
		}
		tstr.resize(tstr.size() - 2);
		str = tstr;
		break;
	}
	case ID_TRANSFORM:
	{
		std::wstring tstr;
		for (unsigned int i = 0; i < 3; i++)
		{
			std::wstring astr;
			astr = BinToFloatStr(str.substr(0 + (4 * i), 4));
			TruncFloatStr(astr);
			tstr.append(astr + _T(", "));
		}
		tstr.resize(tstr.size() - 2);
		str = tstr;
		break;
	}
	case ID_STRING:
	{
		str = str.substr(1);
		break;
	}
	case ID_STRINGL:
	{
		if (str.empty())
		{
			str = _T("<empty>");
		}
		else
		{
			char fuck[4];
			for (unsigned int i = 0; i < 4; ++i)
				fuck[i] = (char)value[i];
			int i;
			i = *((int*)&fuck);

			TCHAR buffer[128];
			memset(buffer, 0, 128);

			swprintf(buffer, 128, GLOB_STRS[9].c_str(), i);
			str = buffer;
		}
		break;
	}
	case ID_INT:
	{
		char fuck[4];
		for (unsigned int i = 0; i < 4; ++i)
		{
			fuck[i] = (char)value[i];
		}
		int i;
		i = *((int*)&fuck);
		str = std::to_wstring(i);
		break;
	}
	default:
		str = _T("");
	}
}

//format string to store in variables vector
template <typename TSTRING>
void FormatValue(std::string &str, const TSTRING &value, const unsigned int &type)
{
	str = WStringToString(value);

	switch (type)
	{
	case ID_FLOAT:
	{
		str = FloatStrToBin(str);

		break;
	}
	case ID_BOOL:
	{
		char abool;
		str[0] == 102 ? abool = char(0) : abool = char(1);
		str = abool;
		break;
	}
	case ID_INT:
	{
		str = IntStrToBin(str);
		break;
	}
	case ID_STRING:
	{
		int i = str.size();
		if (i > 255) i = 255;
		str.insert(0, std::string(1, char(i)));
		break;
	}
	}
}

inline double rad2deg(const double &rad)
{
	return (rad * 180) / pi;
}

inline double deg2rad(const double &degrees)
{
	return (degrees * pi) / 180;
}

bool QuatEqual(const QTRN *a, const QTRN *b)
{
	if (abs(a->x) - abs(b->x) > kindasmall) return FALSE;
	if (abs(a->y) - abs(b->y) > kindasmall) return FALSE;
	if (abs(a->z) - abs(b->z) > kindasmall) return FALSE;
	if (abs(a->w) - abs(b->w) > kindasmall) return FALSE;
	return TRUE;
}

ANGLES QuatToEuler(const QTRN *q)
{
	ANGLES angles;

	double ysqr = q->y * q->y;
	double t0 = -2.0f * (ysqr + q->z * q->z) + 1.0f;
	double t1 = +2.0f * (q->x * q->y - q->w * q->z);
	double t2 = -2.0f * (q->x * q->z + q->w * q->y);
	double t3 = +2.0f * (q->y * q->z - q->w * q->x);
	double t4 = -2.0f * (q->x * q->x + ysqr) + 1.0f;

	t2 = t2 > 1.0f ? 1.0f : t2;
	t2 = t2 < -1.0f ? -1.0f : t2;

	double pitch_r = std::asin(t2);
	double roll_r = std::atan2(t3, t4);
	double yaw_r = std::atan2(t1, t0);

	angles.pitch = rad2deg(pitch_r);
	angles.roll = rad2deg(roll_r);
	angles.yaw = rad2deg(yaw_r);

	return angles;
}

QTRN EulerToQuat(const ANGLES *angles)
{
	QTRN q;

	double pitch_r = deg2rad(angles->pitch);
	double roll_r = deg2rad(angles->roll);
	double yaw_r = deg2rad(angles->yaw);

	double t0 = std::cos(yaw_r * 0.5f);
	double t1 = std::sin(yaw_r * 0.5f);
	double t2 = std::cos(roll_r * 0.5f);
	double t3 = std::sin(roll_r * 0.5f);
	double t4 = std::cos(pitch_r * 0.5f);
	double t5 = std::sin(pitch_r * 0.5f);

	q.w = t0 * t2 * t4 + t1 * t3 * t5;
	q.x = t0 * t3 * t4 - t1 * t2 * t5;
	q.y = t0 * t2 * t5 + t1 * t3 * t4;
	q.z = t1 * t2 * t4 - t0 * t3 * t5;

	return q;
}

std::string ExtractString(const unsigned int start, const unsigned int end, const char* buffer)
{
	std::string str;
	for (unsigned int i = start; i < end; ++i)
	{
		str += buffer[i];
	}
	return str;
}

/*
void ReplaceString(const unsigned int start, std::string &buffer, const std::string &value)
{
std::string str;
for (unsigned int i = 0; i < value.size(); ++i)
{
buffer[start + i] = value[i];
}
}
*/

/*
template <typename TSTRING>
std::string WStringToString(const TSTRING &s)
{
	std::string wsTmp(s.begin(), s.end());
	return wsTmp;
}

template <typename TSTRING>
std::wstring StringToWString(const TSTRING &s)
{
	std::wstring wsTmp(s.begin(), s.end());
	return wsTmp;
}
*/
/*
//toggles hexadecimal float representation between little endian and big endian

void StrFlipEndian(std::string &str)
{
for (unsigned int j = 0; j < (str.size() / 4); j++)
{
std::string holdmybeer = str.substr(j * 4, 4);
unsigned int size = holdmybeer.size();
for (unsigned int i = 0; i < size; i++)
{
str[(j * 4) + i] = holdmybeer[size - i - 1];
}
}
}
*/

float BinToFloat(const std::string &str)
{
	if (str.size() != 4) return 0;
	char fuck[4];
	for (unsigned int i = 0; i < 4; ++i)
	{
		fuck[i] = (char)str[i];
	}
	float f;
	f = *((float*)&fuck);

	return f;
}

std::string BinToFloatStr(const std::string &str)
{
	if (str.size() != 4) return "0";
	char fuck[4];
	for (unsigned int i = 0; i < 4; ++i)
	{
		fuck[i] = (char)str[i];
	}
	float f;
	f = *((float*)&fuck);

	std::string s(32, '\0');
	auto written = std::snprintf(&s[0], s.size(), "%.10f", f);
	s.resize(written);
	return s;
}

std::wstring BinToFloatStr(const std::wstring &str)
{
	if (str.size() != 4) return L"0";
	char fuck[4];
	for (unsigned int i = 0; i < 4; ++i)
	{
		fuck[i] = (char)str[i];
	}
	float f;
	f = *((float*)&fuck);

	std::string s(32, '\0');
	auto written = std::snprintf(&s[0], s.size(), "%.10f", f);
	s.resize(written);
	return StringToWString(s);
}

std::string FloatStrToBin(const std::string &str)
{
	std::string out;
	float x = static_cast<float>(::strtod(str.c_str(), NULL));
	char *a;
	a = (char *)&x;

	for (int i = 0; i < 4; ++i)
	{
		out += a[i];
	};
	return out;
}

std::string IntStrToBin(const std::string &str)
{
	std::string out;
	int x = static_cast<int>(::strtol(str.c_str(), NULL, 10));
	char *a;
	a = (char *)&x;

	for (int i = 0; i < 4; ++i)
	{
		out += a[i];
	};
	return out;
}

std::string IntToBin(int x)
{
	std::string out;
	char *a;
	a = (char *)&x;

	for (int i = 0; i < 4; ++i)
	{
		out += a[i];
	};
	return out;
}

bool ContainsStr(const std::wstring &target, const std::wstring &str)
{
	unsigned int strsize = str.size();
	unsigned int trgsize = target.size();

	for (unsigned int i = 0; i < trgsize; ++i)
	{
		if (target[i] == str[0])
		{
			if (strsize == 1) return TRUE;
			else
			{
				if (strsize > trgsize - i) return FALSE;
				for (unsigned int j = 1; j < strsize; ++j)
				{
					if (i + j >= trgsize) return FALSE;
					if (target[i + j] == str[j])
					{
						if (j + 1 == strsize) return TRUE;
					}
					else break;
				}
			}
		}
	}
	return FALSE;
}

/*
std::wstring MakeHexString(const std::wstring &str)
{
std::string tstr;
for (unsigned int i = 0; i < str.size(); ++i)
{
const char ch = static_cast<char>(str[i]);
tstr.append(&hex[(ch & 0xF0) >> 4], 1);
tstr.append(&hex[ch & 0xF], 1);
}
return StringToWString(tstr);
}
*/

/*
std::string FallbackGetValues(unsigned int &index, const char* buffer)
{
unsigned int type = static_cast<unsigned int>(buffer[index]);
std::string value;
switch (type)
{
case HX_FLOAT:
value = ExtractString(index + 9, index + 13, buffer);
break;
case HX_BOOL:
value = ExtractString(index + 9, index + 10, buffer);
break;
case HX_COLOR:
value = ExtractString(index + 9, index + 25, buffer);
break;
case HX_TRANSFORM4:
case HX_TRANSFORM6:
case HX_TRANSFORM8:
value = ExtractString(index + 10, index + 50, buffer);
break;
default:
value = "";
}
return value;
}
*/

bool ScoopSavegame()
{
	using namespace std;
	vector<Entry> indexed_entries;
	vector<Variable> temp_variables;

	ifstream iwc(filepath, ifstream::binary);

	if (!iwc.is_open())
	{
		return FALSE;
	}

	iwc.seekg(0, iwc.end);
	unsigned int length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	char *buffer = new char[length];
	iwc.read(buffer, length);

	wstring extract;
	unsigned int eindex = 0;
	unsigned int skip;

	for (unsigned int i = 0; i < length;)
	{
		skip = 1;
		int n = static_cast<int>(buffer[i]);
		if (n == HX_STARTENTRY)
		{
			n = static_cast<int>(buffer[i + 1]);
			for (int j = 2; j < (n + 2); j++)
			{
				extract += buffer[i + j];
			}

			// clean up identifier string
			transform(extract.begin(), extract.end(), extract.begin(), ::tolower);
			string::size_type pos1 = extract.find(_T("("));
			if (pos1 != string::npos)
			{
				wstring _str = extract.substr(pos1 + 1, 5);
				extract.erase(pos1, 7);
				string::size_type pos2 = _str.find_last_not_of(_T("x"));
				if (pos2 != string::npos && _str != _T("clone"))
				{
					extract.insert(pos1, _str.substr(0, pos2 + 1));
				}
			}
			// further clean up identifier string
			for (unsigned int k = 0; k < TextTable.size(); k++)
			{
				pos1 = extract.find(TextTable[k].badstring);
				while (pos1 != string::npos)
				{
					extract.replace(pos1, TextTable[k].badstring.length(), TextTable[k].newstring);
					pos1 = extract.find(TextTable[k].badstring);
				}
			}

			//fetch variable length (type + value)
			unsigned int type = -1;

			std::string value;
			unsigned int val_index = i + 2 + n + 4;
			int var_len = *((int*)&buffer[i + 2 + n]);

			if (var_len <= 0) return FALSE;
			skip = 5 + n + var_len;
			if ((i + skip) > length) return FALSE;
			if (buffer[i + skip] != HX_ENDENTRY) return FALSE;


			std::string btype = ExtractString(val_index, val_index + 5, buffer);

			for (unsigned int k = 0; k < 7; k++)
			{
				std::string dtype = WStringToString(DATATYPES[k].substr(0, 5));
				if (btype == dtype)
				{
					type = k;
					val_index += 5;
					var_len += -6;
					if (k < 2) { val_index++; var_len--; }
				}
			}

			//unsigned int index = i + 3 + n;
			//FallbackGetValues(index, buffer);

			value = ExtractString(val_index, val_index + var_len, buffer);

			switch (type)
			{
			case ID_STRINGL:
			{
				int str_len = *((int*)&buffer[val_index + 1]);

				str_len == 0 ? value = "" : value = value.substr(1);
				break;
			}
			case ID_TRANSFORM:
				value = value.substr(0, 40);
				break;
			case ID_STRING:
			case ID_BOOL:
			case ID_FLOAT:
			case ID_COLOR:
			case ID_INT:
				break;
			default:
				break;
			}

			indexed_entries.push_back(Entry(extract, eindex));
			eindex++;
			temp_variables.push_back(Variable(value, i + 2 + n, type, extract));

			extract.clear();
		}
		i += skip;
	}
	delete[] buffer;
	iwc.close();

	//sort identifier list
	std::sort(indexed_entries.begin(), indexed_entries.end());

	//sort variable list accordingly
	for (unsigned int i = 0; i < indexed_entries.size(); i++)
	{
		unsigned int index = indexed_entries[i].index;
		variables.push_back(temp_variables[index]);
	}

	wstring previous_extract;
	unsigned int group = 0;

	for (unsigned int i = 0; i < indexed_entries.size(); i++)
	{
		if (!previous_extract.empty())
		{
			if (indexed_entries[i].name.substr(0, previous_extract.length()) != previous_extract)
			{
				group++;
				previous_extract = indexed_entries[i].name;
				entries.push_back(indexed_entries[i].name);
			}
		}
		else
		{
			previous_extract = indexed_entries[i].name;
			entries.push_back(indexed_entries[i].name);
		}
		variables[i].group = group;
	}
	return entries.empty() ? FALSE : TRUE;
}