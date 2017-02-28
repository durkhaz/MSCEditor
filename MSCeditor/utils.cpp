#include "utils.h"
#include <shobjidl.h> //COM
#include <fstream> //file stream input output
#include <algorithm> //sort
#include <Urlmon.h>

void GetPathToTemp(std::wstring &path)
{
	TCHAR buffer[MAX_PATH];
	
	if (GetTempPath(MAX_PATH, buffer))
	{
		path = buffer;
		path += L"\\MSCeditor";
		CreateDirectory(path.c_str(), NULL);
	}
}

BOOL DownloadUpdatefile(const std::wstring url, std::wstring &path)
{
	GetPathToTemp(path);
	path += L"\\update.tmp";
	return URLDownloadToFile(NULL, (LPCWSTR)url.c_str(), (LPCWSTR)path.c_str(), 0, NULL);
}

void FlipString(std::string &str, std::wstring file = L"")
{
	for (UINT i = 0; i < str.size(); i++)
	{
		str[i] = ~str[i];
	}
	if (!file.empty())
	{
		std::ofstream outf(file, std::ofstream::binary);
		outf << str;
	}
}

BOOL ParseUpdateData(const std::string &str, std::vector<std::string> &strs)
{
	UINT i = 0;

	while (TRUE)
	{
		UINT size = (UINT)str[i];
		strs.push_back(str.substr(i + 1, size));
		i += 1 + size;
		if ((i + 1) >= str.size())
			break;
	}
	return i == str.size();
}

BOOL CheckUpdate(std::wstring &file, std::wstring &apppath, std::wstring &changelog)
{
	using namespace std;

	ifstream iwc(file, ifstream::in, ifstream::binary);
	if (!iwc.is_open())
		return 1;

	iwc.seekg(0, iwc.end);
	UINT length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	char *buffer = new char[length + 1];
	memset(buffer, '\0', length + 1);
	iwc.read(buffer, length);
	iwc.close();
	std::string str = buffer;
	//FlipString(str, L"invert");
	delete[] buffer;
	FlipString(str);
	std::vector<std::string> strs;
	bool up2date = TRUE;
	if (ParseUpdateData(str, strs))
	{
		for (UINT i = 0; (i + 1) < strs.size(); i++)
		{
			if ((strs[i]) == "changelog")
			{
				changelog = StringToWString(strs[i + 1]);
			}
			if ((strs[i]) == "path1")
			{
				apppath = StringToWString(strs[i + 1]);
			}
			if ((strs[i]) == "version")
			{
				double uVer = static_cast<float>(::strtod(strs[i + 1].c_str(), NULL));
				double fVer = static_cast<float>(::wcstod(Version.c_str(), NULL));
				if (uVer > fVer)
					up2date = FALSE;

			}
		}
	}
	return (!up2date && !changelog.empty() && !apppath.empty());
}

std::wstring ReadRegistry(const HKEY root, const std::wstring key, const std::wstring name)
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
		{ 
			CloseHandle(hFile);
			return FALSE;
		}
		FileTimeToSystemTime(&ftWrite, &stUTC);
		CloseHandle(hFile);
	}
	else return FALSE;
	return TRUE;
}

LPARAM MakeLPARAM(const UINT &i, const UINT &j, const bool &negative)
{
	if (i > static_cast<int>(INT_MAX/LPARAM_OFFSET) || j > (LPARAM_OFFSET - 1))
		return (LPARAM)0;
	int k;
	negative ? k = -1 : k = 1;
	return (LPARAM)((i * LPARAM_OFFSET + j) * k);
}

void BreakLPARAM(const LPARAM &lparam, int &i, int &j)
{
	if (lparam >= (LPARAM_OFFSET))
	{
		i = lparam / LPARAM_OFFSET;
		j = lparam - (i * LPARAM_OFFSET);
	}
	else
	{
		i = 0;
		j = lparam;
	}
}

int CompareStrs(const std::wstring &str1, const std::wstring &str2)
{
	UINT max = str1.size();
	if (str1.size() > str2.size())
		max = str2.size();

	for (UINT i = 0; i < max; i++)
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

	UINT max = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);

	for (UINT i = 0; i < max; i++)
	{
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.lParam = i;
		ListView_SetItem(hList, (LPARAM)&lvi);
	}
}

UINT GetGroupStartIndex(const UINT &group, const UINT &index = UINT_MAX)
{
	if (group == 0) return 0;
	if (index == UINT_MAX)
	{
		for (UINT i = 0; i < variables.size(); i++)
		{
			if (variables[i].group == group)
				return i;
		}
	}
	else
	{
		if (variables[index].group != group)
			return GetGroupStartIndex(group, UINT_MAX);
		for (UINT i = index; i >= 0; i--)
		{
			if (variables[i].group == (group - 1))
				return (i + 1);
		}
	}
	return UINT_MAX;
}

LVITEM GetGroupEntry(const UINT &group)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	UINT max = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iItem = -1;
	for (UINT i = 0; i < max; i++)
	{
		lvi.iItem = i;
		ListView_GetItem(hList, (LPARAM)&lvi);
		ListParam* listparam = (ListParam*)lvi.lParam;

		if (listparam->GetIndex() == group)
			break;
	}
	return lvi;
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

bool FileChanged()
{
	SYSTEMTIME stUTC;
	GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC);

	if (!DatesMatch(stUTC))
	{
		TCHAR buffer[256];
		memset(buffer, 0, 256);
		swprintf(buffer, 256, GLOB_STRS[17].c_str(), filepath.c_str());
		switch (MessageBox(NULL, buffer, Title.c_str(), MB_YESNO | MB_ICONWARNING))
		{
			case IDNO:
			{
				std::wstring tmpPath;
				GetPathToTemp(tmpPath);
				tmpPath += L"\\file.tmp";

				HANDLE hFile = CreateFileW(tmpPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hFile);
					DeleteFile(filepath.c_str());
					CopyFileEx(tmpPath.c_str(), filepath.c_str(), NULL, NULL, FALSE, 0);
					break;
				}
				else
				{
					MessageBox(NULL, GLOB_STRS[38].c_str(), Title.c_str(), MB_OK | MB_ICONERROR);
					//leak into IDYES to reload
				}
				
			}
			case IDYES:
				InitMainDialog(hDialog);
				break;
		}
	}
	return FALSE;
}

void OpenFileDialog()
{
	std::wstring fpath;
	std::wstring fname;

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
					PWSTR pszFileName;
					pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszFileName);
					fpath = pszFilePath;
					fname = pszFileName;
					CoTaskMemFree(pszFilePath);
					CoTaskMemFree(pszFileName);
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();

		if (!fpath.empty() && !fname.empty())
		{
			filepath = fpath;
			filename = fname;
			InitMainDialog(hDialog);
		}
	}
}

void UnloadFile()
{
	std::wstring tmppath;
	GetPathToTemp(tmppath);
	tmppath += L"\\file.tmp";
	DeleteFile(tmppath.c_str());

	SetWindowText(hDialog, (LPCWSTR)Title.c_str());
	HWND hList1 = GetDlgItem(hDialog, IDC_List);
	HWND hList2 = GetDlgItem(hDialog, IDC_List2);
	FreeLPARAMS(hList1);
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
	EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 4), MF_GRAYED);

	menu = GetSubMenu(GetMenu(hDialog), 1);
	EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_GRAYED);

	MENUITEMINFO info = { sizeof(MENUITEMINFO) };
	info.fMask = MIIM_STATE;
	info.fState = MFS_GRAYED;
	SetMenuItemInfo(menu, 3, TRUE, &info);
}

bool CanClose()
{
	if (WasModified())
	{
		TCHAR buffer[128];
		memset(buffer, 0, 128);
		swprintf(buffer, 128, GLOB_STRS[7].c_str(), filepath.c_str());

		switch (MessageBox(NULL, buffer, Title.c_str(), MB_YESNOCANCEL | MB_ICONWARNING))
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

template <typename TSTRING>
void KillWhitespaces(TSTRING &str)
{
	TSTRING tmp;
	tmp.reserve(str.size());
	for (UINT i = 0; i < str.size(); i++)
	{
		if (!isspace(str[i]))
			tmp += str[i];
	}
	str = tmp;
}

std::vector<char> MakeCharArray(std::wstring &str)
{
	std::vector<char> cArray;
	UINT iC = 0, iP = 0;
	KillWhitespaces(str);

	while (iC < str.size())
	{
		if (str[iC] == ',')
		{
			cArray.push_back(static_cast<char>(static_cast<int>(::wcstol(str.substr(iP, iC - iP).c_str(), NULL, 10))));
			iP = iC + 1;
		}
		iC++;
	}
	cArray.push_back(static_cast<char>(static_cast<int>(::wcstol(str.substr(iP, iC - iP).c_str(), NULL, 10))));
	return cArray;
}

// This is a disgusting mess. I should probably do this function properly, but I'm not getting paid for this shit so deal with it, nerd
void FillVector(const std::vector<std::wstring> &params, const std::wstring &identifier)
{
	if (identifier == L"Locations")
	{
		if (params.size() == 2)
		{
			std::string bin;
			std::wstring coords = params[1];
			VectorStrToBin(coords, 3, bin);
			VectorStrToBin(std::wstring(_T("0,0,0,1")), 4, bin);
			VectorStrToBin(std::wstring(_T("1,1,1")), 3, bin);
			locations.push_back(TextLookup(params[0], StringToWString(bin)));
		}
	}
	else if (identifier == L"Items")
	{
		switch (params.size())
		{
			case 3:
				itemTypes.push_back(Item(WStringToString(params[0]), MakeCharArray(std::wstring(params[1])), WStringToString(params[2])));
				break;
			case 2:
				itemTypes.push_back(Item(WStringToString(params[0]), MakeCharArray(std::wstring(params[1]))));
				break;
		}
	}
	else if (identifier == L"Item_Attributes")
	{
		if (params.size() >= 3)
		{
			std::string s = WStringToString(params[1]);
			char c = static_cast<char>(static_cast<int>(::wcstol(params[2].c_str(), NULL, 10)));
			if (params.size() == 3)
				itemAttributes.push_back(ItemAttribute(s, c));
			else if (params.size() == 5)
				itemAttributes.push_back(ItemAttribute(s, c, static_cast<int>(::wcstol(params[3].c_str(), NULL, 10)), static_cast<int>(::wcstol(params[4].c_str(), NULL, 10))));
		}
	}
	else if (identifier == L"Report_Identifiers")
	{
		if (params.size() == 1)
			partIdentifiers.push_back(params[0]);
	}
	else if (identifier == L"Report_Special")
	{
		if (params.size() >= 2)
		{
			std::wstring param = _T("");
			if (params.size() == 3)
				param = params[2];
			partSCs.push_back(SC(params[0], static_cast<int>(::strtol(WStringToString(params[1]).c_str(), NULL, 10)), param));
		}
	}

}

BOOL LoadDataFile(const std::wstring &datafilename)
{
	using namespace std;

	wstring strInput, identifier;
	vector<wstring> params;
	wifstream inf(datafilename, wifstream::in);
	if (!inf.is_open())
		return 1;

	getline(inf, strInput);
	while (inf)
	{
		for (UINT i = 0; i < strInput.size(); i++)
		{
			if (strInput[i] == '/')
				if (i + 1 < strInput.size())
					if (strInput[i + 1] == '/')
						break;
			if (strInput[i] == '#')
			{
				string::size_type startpos = 0, endpos = 0;
				if (FetchDataFileParameters(strInput.substr(i + 1), startpos, endpos) == 0)
					identifier = strInput.substr(startpos + 1, endpos - startpos);
				break;
			}
		}
		if (!identifier.empty())
		{
			bool LinesLeft = true;
			while (LinesLeft && !inf.eof())
			{
				getline(inf, strInput);
				string::size_type startpos = 0, endpos = 0, offset = 0;
				bool LineNotDone = true;
				while (LineNotDone && LinesLeft)
				{
					switch (FetchDataFileParameters(strInput.substr(offset), startpos, endpos))
					{
						case 0:
							params.push_back(strInput.substr(startpos + offset, endpos - startpos));
							offset += endpos + 1;
							LineNotDone = !(offset >= strInput.size());
							break;
						case 1:
							LineNotDone = false;
							break;
						case 2:
							LinesLeft = false;
							break;
					}
				}
				if (!params.empty())
				{
					FillVector(params, identifier);
					params.clear();
				}
			}
			identifier.clear();
			continue;
		}
		getline(inf, strInput);
	}
	return 0;
}

inline bool WasModified()
{
	for (UINT i = 0; i < variables.size(); i++)
	{
		if (variables[i].IsModified() || variables[i].IsAdded() || variables[i].IsRemoved())
		{
			return TRUE;
		}
	}
	return FALSE;
}

void TruncTailingNulls(std::string *str)
{
	UINT i = str->size() - 1;
	for (i; str->at(i) == '\0'; i--);
	str->resize(i + 1);
}

int SaveFile()
{
	using namespace std;

	FileChanged();

	ifstream iwc(filepath, ifstream::binary);
	if (!iwc.is_open()) return 1;

	iwc.seekg(0, iwc.end);
	UINT length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	string buffer(length + 1, '\0');
	iwc.read((char*)buffer.data(), length);

	if (iwc.bad() || iwc.fail())
	{
		iwc.close();
		return 1;
	}
	iwc.close();

	TruncTailingNulls(&buffer);

	vector<Position> offsets;
	for (UINT i = 0; i < variables.size(); i++)
	{
		if (variables[i].IsModified() ||  variables[i].IsRemoved() || variables[i].IsAdded())
		{
			offsets.push_back(Position(i, variables[i].pos));
		}
	}
	std::sort(offsets.begin(), offsets.end());

	int offset_sum = 0;

	for (UINT i = 0; i < offsets.size(); i++)
	{
		UINT index = offsets[i].index;
		string str;
		UINT kLength = static_cast<int>(buffer[variables[index].pos + offset_sum + 1]) + 2;

		char intstr[4];
		for (UINT j = 0; j < 4; ++j)
		{
			intstr[j] = (char)buffer[variables[index].pos + kLength + offset_sum + j];
		}
		int vLength = *((int*)&intstr);

		if (variables[index].IsRemoved())
		{
			int eLength = kLength + 4 + vLength;
			buffer.erase(variables[index].pos + offset_sum, eLength);
			offset_sum += -eLength;
		}
		else if (variables[index].IsAdded())
		{
			buffer += variables[index].MakeEntry();
		}
		else
		{
			switch (variables[index].type)
			{
				case ID_STRING:
				case ID_STRINGL:
				case ID_TRANSFORM:
					FormatValue(str, to_string(variables[index].value.size() + ValIndizes[variables[index].type][0] + 1), ID_INT);

					buffer.replace(variables[index].pos + kLength + offset_sum, str.size(), str);
					buffer.replace(variables[index].pos + kLength + offset_sum + 4 + ValIndizes[variables[index].type][0], vLength - ValIndizes[variables[index].type][0] - 1, variables[index].value);

					offset_sum += variables[index].value.size() - (vLength - ValIndizes[variables[index].type][0] - 1);
					break;
				default:
					buffer.replace(variables[index].pos + kLength + offset_sum + 4 + ValIndizes[variables[index].type][0], ValIndizes[variables[index].type][1], variables[index].value);
			}
		}
	}

	if (MakeBackup)
	{
		wstring nfilepath = filepath;
		size_t found = nfilepath.find_last_of(_T("."));
		if (found == string::npos)
			found = nfilepath.size() - 1;
		nfilepath = nfilepath.insert(found, _T("_backup01"));

		if (MoveFileEx(filepath.c_str(), nfilepath.c_str(), MOVEFILE_WRITE_THROUGH) == 0)
		{
			DWORD eword = GetLastError();
			if (eword != ERROR_ALREADY_EXISTS) return 2;
			while (eword == ERROR_ALREADY_EXISTS)
			{
				wstring _tmp = to_wstring(stoi(nfilepath.substr(found + 7, 2), nullptr, 10) + 1);
				if (_tmp.size() == 1) _tmp.insert(0, L"0");
				nfilepath.replace(found + 7, 2, _tmp);
				MoveFileEx(filepath.c_str(), nfilepath.c_str(), MOVEFILE_WRITE_THROUGH);
				eword = GetLastError();
			}
		}
	}
	else
	{
		if (DeleteFile(filepath.c_str()) == 0) return 3;
		if (GetLastError() != ERROR_SUCCESS) return 3;
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

	SYSTEMTIME stUTC;
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
	ErrorCode err = ScoopSavegame();
	if (err.id != -1)
	{
		MessageBox(hDialog, (GLOB_STRS[31] + std::to_wstring(err.info) + GLOB_STRS[err.id]).c_str(), _T("Error"), MB_OK | MB_ICONERROR);
	}
	else
	{
		SYSTEMTIME stUTC;
		if (GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC))
		{
			filedate = stUTC;
			filedateinit = TRUE;
		}
		else
		{
			MessageBox(hDialog, GLOB_STRS[6].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
			filedateinit = FALSE;
		}

		std::wstring newpath;
		GetPathToTemp(newpath);
		newpath += L"\\file.tmp";
		if (!CopyFileEx(filepath.c_str(), newpath.c_str(), NULL, NULL, FALSE, 0))
		{
			TCHAR buffer[128];
			memset(buffer, 0, 128);
			swprintf(buffer, 128, GLOB_STRS[37].c_str(), newpath.c_str());
			MessageBox(hDialog, buffer, Title.c_str(), MB_OK | MB_ICONERROR);
		}

		TCHAR buffer[128];
		memset(buffer, 0, 128);
		swprintf(buffer, 128, _T("%s - [%s]"), Title.c_str(), filepath.c_str());
		SetWindowText(hDialog, (LPCWSTR)buffer);

		LVCOLUMN lvc;

		ListView_SetBkColor(GetDlgItem(hwnd, IDC_List), (COLORREF)GetSysColor(COLOR_WINDOW));
		ListView_SetBkColor(GetDlgItem(hwnd, IDC_List2), (COLORREF)GetSysColor(COLOR_WINDOW));

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 0; lvc.pszText = _T(""); lvc.cx = 230; lvc.fmt = LVCFMT_LEFT;
		SendMessage(GetDlgItem(hwnd, IDC_List), LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		UINT size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
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
		menu = GetSubMenu(GetMenu(hDialog), 1);
		EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_ENABLED);

		for (UINT i = 0; i < itemTypes.size(); i++)
		{
			if (EntryExists(itemTypes[i].GetID()) >= 0)
			{
				MENUITEMINFO info = { sizeof(MENUITEMINFO) };
				info.fMask = MIIM_STATE;
				info.fState = MFS_ENABLED;
				SetMenuItemInfo(menu, 3, TRUE, &info);
				break;
			}
		}
		memset(buffer, 0, 128);
		swprintf(buffer, 128, GLOB_STRS[11].c_str(), 0);
		SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)buffer);
	}
}

inline bool PartIsStuck(std::wstring &stuckStr, const std::vector<UINT> &boltlist, const UINT &tightness)
{
	UINT boltstate = 0;
	for (UINT k = 0; k < boltlist.size(); k++)
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

void PopulateBList(HWND hwnd, const CarPart *part, UINT &item, Overview *ov)
{
	HWND hList3 = GetDlgItem(hwnd, IDC_BLIST);

	std::wstring stuckStr = BListSymbols[0];
	std::wstring boltStr = BListSymbols[0];
	LVITEM lvi;

	if (part->iBolts != UINT_MAX && part->iTightness != UINT_MAX)
	{
		UINT bolts = 0, maxbolts = 0;
		std::vector<UINT> boltlist;

		if (BinToBolts(variables[part->iBolts].value, bolts, maxbolts, boltlist))
		{
			UINT tightness = static_cast<UINT>(BinToFloat(variables[part->iTightness].value));
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
				for (UINT j = 0; j < partSCs.size(); j++)
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
	UINT item = 0;
	Overview ov;
	HWND hList3 = GetDlgItem(hReport, IDC_BLIST);
	SendMessage(hList3, WM_SETREDRAW, 0, 0);
	SendMessage(hList3, LVM_DELETEALLITEMS, 0, 0);

	for (UINT i = 0; i < carparts.size(); i++)
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

	for (UINT i = 0; i < 8; i++)
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
	UINT max = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);

	LVITEM lvi = GetGroupEntry(group);
	if (lvi.iItem != -1)
	{
		bool modified = FALSE;
		UINT index = UINT_MAX;

		for (UINT j = 0; variables[j].group <= (UINT)group; j++)
		{
			if (variables[j].group == group)
			{
				if (index == UINT_MAX)
					index = j;
				if (variables[j].IsModified() || variables[j].IsAdded() || variables[j].IsRemoved())
				{
					modified = TRUE;
					break;
				}
			}
		}
		
		ListParam* listparam = (ListParam*)lvi.lParam;

		listparam->SetFlag(VAR_REMOVED, GroupRemoved(group, index, TRUE));
		listparam->SetFlag(VAR_MODIFIED, modified);
		ListView_SetItem(hList, (LPARAM)&lvi);
		ListView_RedrawItems(hList, lvi.iItem, lvi.iItem);
		UpdateWindow(hList);
	}
}

void UpdateChild(const int &vIndex, std::string &str)
{
	variables[vIndex].value = str;
	for (UINT i = 0; i < indextable.size(); i++)
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

void UpdateChangeCounter()
{
	//count number of changes made to file

	UINT num = 0;
	for (UINT i = 0; i < variables.size(); i++)
	{
		if (variables[i].IsModified() || variables[i].IsAdded() || variables[i].IsRemoved())
		{
			num++;
		}
	}

	//Enable menues when changes were made

	HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
	num > 0 ? EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_ENABLED) : EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);

	//Update change counter

	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT3), hDialog);
	TCHAR buffer[128];
	memset(buffer, 0, 128);
	swprintf(buffer, 128, GLOB_STRS[11].c_str(), num);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)buffer);
}

void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin)
{
	std::string str;
	FormatValue(str, viewstr, variables[vIndex].type);
	if (bin.size() != 0) str = bin;

	if (variables[vIndex].static_value == str)
	{
		variables[vIndex].SetModified(FALSE);
		if (variables[vIndex].value != str)
		{
			UpdateChild(vIndex, str);
			UpdateParent(variables[vIndex].group);
		}
		else
		{
			UpdateChild(vIndex, variables[vIndex].value);
			UpdateParent(variables[vIndex].group);
		}
	}
	else
	{
		variables[vIndex].SetModified(TRUE);

		UpdateChild(vIndex, str);
		UpdateParent(variables[vIndex].group);
	}

	UpdateChangeCounter();
}

void UpdateList(const std::wstring &str)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	FreeLPARAMS(hList);
	SendMessage(hList, WM_SETREDRAW, 0, 0);
	SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);

	LVITEM lvi;

	if (str.size() == 0)
	{
		SendMessage(hList, LVM_SETITEMCOUNT, entries.size(), 0);
		for (UINT i = 0; i < entries.size(); i++)
		{
			ListParam *param = new ListParam(0, i);
			UINT j;
			for (j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == i) break;
			}
			param->SetFlag(VAR_REMOVED, GroupRemoved(i, j, true));
			for (j; j < variables.size(); j++)
			{
				if (variables[j].group != i) break;
				if (variables[j].IsModified() || variables[j].IsAdded() || variables[j].IsRemoved())
				{
					param->SetFlag(VAR_MODIFIED, true);
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
		UINT* indizes = new UINT[variables.size()];
		UINT index = 0;

		for (UINT i = 0; i < variables.size(); i++)
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

		for (UINT i = 0; i < index; i++)
		{
			ListParam *param = new ListParam(0, indizes[i]);
			UINT j;
			for (j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == indizes[i]) break;
			}
			param->SetFlag(VAR_REMOVED, GroupRemoved(indizes[i], j, true));
			for (j; j < variables.size(); j++)
			{
				if (variables[j].group != indizes[i]) break;
				if (variables[j].IsModified() || variables[j].IsAdded() || variables[j].IsRemoved())
				{
					param->SetFlag(VAR_MODIFIED, true);
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

void FreeLPARAMS(HWND hwnd)
{
	UINT size = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
	if (size > 1)
	{
		for (UINT i = 0; i < size; i++)
		{
			LVITEM lvi;
			lvi.mask = LVIF_PARAM;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			ListView_GetItem(hwnd, (LPARAM)&lvi);
			delete (ListParam*)lvi.lParam;
		}
	}
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
UINT VectorStrToBin(std::wstring &str, const UINT &size, std::string &bin, const bool allownegative, const bool normalized, const bool eulerconvert, const QTRN *oldq, const std::string &oldbin)
{
	std::string::size_type found = 0;
	int *indizes = new int[size + 1]{ -1 };
	indizes[size] = str.size();
	UINT seperators = 0;

	//kill shitespaces
	KillWhitespaces(str);

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
	for (UINT i = 0; i != size; i++)
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
			for (UINT i = 0; i < 4; i++)
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
		for (UINT i = 0; i != size; i++)
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

void BatchProcessUninstall()
{
	for (UINT i = 0; i < carparts.size(); i++)
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
			UINT bolts = 0, maxbolts = 0;
			std::vector<UINT> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				for (UINT j = 0; j < boltlist.size(); j++)
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
	for (UINT i = 0; i < carparts.size(); i++)
	{
		if ((carparts[i].iBolts != UINT_MAX) && (carparts[i].iTightness != UINT_MAX))
		{
			UINT bolts = 0, maxbolts = 0;
			std::vector<UINT> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				UINT boltstate = 0;
				UINT tightness = static_cast<UINT>(BinToFloat(variables[carparts[i].iTightness].value));

				for (UINT j = 0; j < boltlist.size(); j++)
				{
					boltstate += boltlist[j];
				}

				// adjust boltstate for special cases, if there are any
				if (!partSCs.empty())
				{
					for (UINT j = 0; j < partSCs.size(); j++)
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
	for (UINT i = 0; i < carparts.size(); i++)
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
	for (UINT i = 0; i < carparts.size(); i++)
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
				UINT bolts = 0, maxbolts = 0;
				std::vector<UINT> boltlist;

				if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
				{
					if (maxbolts != bolts || !fix)
					{
						int boltstate = fix ? 8 : 0;
						std::vector<UINT> boltlist;
						for (UINT j = 0; j < maxbolts; j++)
						{
							boltlist.push_back(boltstate);
						}
						int tightness = boltlist.size() * boltstate;

						// adjust tightness for special cases, if there are any
						if (!partSCs.empty() && fix)
						{
							for (UINT j = 0; j < partSCs.size(); j++)
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

bool BinToBolts(const std::string &str, UINT &bolts, UINT &maxbolts, std::vector<UINT> &boltlist)
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
			UINT boltstate = static_cast<UINT>(::strtol(str.substr(offset + start, end - start).c_str(), NULL, 10));
			boltlist.push_back(boltstate);
			bolts += (boltstate == 8 ? 1 : 0);
			maxbolts++;
		}
		offset += end;
	}
	return (maxbolts != 0) ? TRUE : FALSE;
}

std::string BoltsToBin(std::vector<UINT> &bolts)
{
	if (bolts.size() == 0) return "";
	std::string bin;
	bin += IntToBin(bolts.size());

	for (UINT i = 0; i < bolts.size(); i++)
	{
		std::string s = "int(" + std::to_string(bolts[i]) + ")";
		char c = char(s.length());
		bin += c + s;
	}

	return bin;
}

//returns -1 when failure, otherwise index of newly added var
int Variables_add(Variable var)
{
	UINT index = 0;
	UINT group = UINT_MAX;
	for (index; index < variables.size(); index++)
	{
		std::wstring xname = variables[index].key;
		std::wstring yname = var.key;
		transform(xname.begin(), xname.end(), xname.begin(), ::tolower);
		transform(yname.begin(), yname.end(), yname.begin(), ::tolower);
		if (xname == yname) return -1;
		if (xname > yname) break;
	}

	for (UINT i = 0; i < 2; i++)
	{
		int n = (index - i);
		if (n < 0) n = 0;
		if ((UINT)n >= variables.size()) n = variables.size() - 1;
		if (ContainsStr(var.key, entries[variables[n].group]))
			group = variables[n].group;
	}
	if (group == UINT_MAX)
	{
		if (index == 0)
			group = 0;
		else 
			group = variables[index - 1].group + 1;

		for (UINT i = index; i < variables.size(); i++)
		{
			variables[i].group += 1;
		}
		entries.insert((entries.begin() + group), var.key);
	}
		
	var.group = group;
	var.SetAdded(TRUE);
	variables.insert((variables.begin() + index), var);
	UpdateChangeCounter();
	return index;
}

bool Variables_remove(const UINT &index)
{
	if (index < 0 && index >= variables.size())
		return FALSE;

	UINT group = variables[index].group;

	//remove added entries, but highlight standard ones
	if (variables[index].IsAdded())
	{
		variables.erase(variables.begin() + index);

		//when group is empty
		bool GroupIsEmpty = true;
		for (UINT i = 0; i < variables.size(); i++)
		{
			if (variables[i].group == group)
			{
				GroupIsEmpty = false;
				break;
			}
		}
		//clean up
		if (GroupIsEmpty)
		{
			for (UINT i = index; i < variables.size(); i++)
			{
				variables[i].group += -1;
			}
			entries.erase(entries.begin() + group);
		}
		UpdateChangeCounter();
	}
	else
	{
		variables[index].SetRemoved(TRUE);
		UpdateValue(L"", index, variables[index].value);
	}
	return TRUE;
}

bool GroupRemoved(const UINT &group, const UINT &index, const bool &IsFirst)
{
	UINT startindex = index;
	if (!IsFirst) 
		startindex = GetGroupStartIndex(group, index);
	bool removed = TRUE;
	for (UINT i = startindex; variables[i].group <= group; i++)
	{
		if (!variables[i].IsRemoved())
		{
			removed = FALSE;
			break;
		}
	}
	return removed;
}

bool IsValidFloatStr(const std::wstring &str)
{
	for (UINT i = 0; i < str.size(); i++)
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
void FormatString(std::wstring &str, const TSTRING &value, const UINT &type)
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
		str = (str.c_str())[0] == 0x00 ? bools[0] : bools[1];
		break;
	}
	case ID_COLOR:
	{
		std::wstring tstr;
		for (UINT i = 0; i < 4; i++)
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
		for (UINT i = 0; i < 3; i++)
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
			for (UINT i = 0; i < 4; ++i)
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
		for (UINT i = 0; i < 4; ++i)
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
void FormatValue(std::string &str, const TSTRING &value, const UINT &type)
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
		if (str.size() > 1)
		{
			std::string test = WStringToString(bools[0]);
			str = (str == WStringToString(bools[0]) ? char(0) : char(1));
		}
		else if (str.size() == 1)
			str = (str == "1" ? char(1) : char(0));
		
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

std::string ExtractString(const UINT start, const UINT end, const char* buffer)
{
	std::string str;
	for (UINT i = start; i < end; ++i)
	{
		str += buffer[i];
	}
	return str;
}

/*
void ReplaceString(const UINT start, std::string &buffer, const std::string &value)
{
std::string str;
for (UINT i = 0; i < value.size(); ++i)
{
buffer[start + i] = value[i];
}
}
*/

/*
//toggles hexadecimal float representation between little endian and big endian

void StrFlipEndian(std::string &str)
{
for (UINT j = 0; j < (str.size() / 4); j++)
{
std::string holdmybeer = str.substr(j * 4, 4);
UINT size = holdmybeer.size();
for (UINT i = 0; i < size; i++)
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
	for (UINT i = 0; i < 4; ++i)
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
	for (UINT i = 0; i < 4; ++i)
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
	for (UINT i = 0; i < 4; ++i)
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

UINT ParseItemID(const std::wstring &str, const UINT sIndex)
{
	if (sIndex < str.size())
	{
		for (UINT i = sIndex; isdigit(str[i]); ++i)
		{
			if (str[i + 1] >= str.size())
				return static_cast<UINT>(::wcstol(str.substr(sIndex, i - sIndex).c_str(), NULL, 10));
		}
	}
	return UINT_MAX;
}

bool StartsWithStr(const std::wstring &target, const std::wstring &str)
{
	std::wstring tTar = target, tStr = str;
	transform(tTar.begin(), tTar.end(), tTar.begin(), ::tolower);
	transform(tStr.begin(), tStr.end(), tStr.begin(), ::tolower);

	for (UINT i = 0; tTar[i] == tStr[i]; ++i)
	{
		if (i + 1 >= tStr.size())
			return TRUE;
	}
	return FALSE;
}

bool ContainsStr(const std::wstring &target, const std::wstring &str)
{
	UINT strsize = str.size();
	UINT trgsize = target.size();

	for (UINT i = 0; i < trgsize; ++i)
	{
		if (target[i] == str[0])
		{
			if (strsize == 1) return TRUE;
			else
			{
				if (strsize > trgsize - i) return FALSE;
				for (UINT j = 1; j < strsize; ++j)
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
for (UINT i = 0; i < str.size(); ++i)
{
const char ch = static_cast<char>(str[i]);
tstr.append(&hex[(ch & 0xF0) >> 4], 1);
tstr.append(&hex[ch & 0xF], 1);
}
return StringToWString(tstr);
}
*/

ErrorCode ScoopSavegame()
{
	using namespace std;
	vector<Entry> indexed_entries;
	vector<Variable> temp_variables;

	ifstream iwc(filepath, ifstream::binary);

	if (!iwc.is_open())
	{
		return ErrorCode(13);
	}

	iwc.seekg(0, iwc.end);
	UINT length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	char *buffer = new char[length];
	iwc.read(buffer, length);

	wstring extract;
	UINT eindex = 0;
	UINT skip;

	for (UINT i = 0; i < length;)
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
			for (UINT k = 0; k < TextTable.size(); k++)
			{
				pos1 = extract.find(TextTable[k].badstring);
				while (pos1 != string::npos)
				{
					extract.replace(pos1, TextTable[k].badstring.length(), TextTable[k].newstring);
					pos1 = extract.find(TextTable[k].badstring);
				}
			}

			//fetch variable length (type + value)
			UINT type = -1;

			std::string value;
			UINT val_index = i + 2 + n + 4;
			int var_len = *((int*)&buffer[i + 2 + n]);

			if (var_len <= 0)
			{
				return ErrorCode(32, i);
			}
			skip = 5 + n + var_len;
			if ((i + skip) > length)
			{
				return ErrorCode(33, i);
			}
			if (buffer[i + skip] != HX_ENDENTRY)
			{
				return ErrorCode(33, i);
			}

			std::string btype = ExtractString(val_index, val_index + 5, buffer);

			for (UINT k = 0; k < 7; k++)
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

			value = ExtractString(val_index, val_index + var_len, buffer);
			std::wstring tag = L"";

			switch (type)
			{
				case ID_STRINGL:
				{
					int str_len = *((int*)&buffer[val_index + 1]);

					str_len == 0 ? value = "" : value = value.substr(1);
					break;
				}
				case ID_TRANSFORM:
					//value = value.substr(0, ValIndizes[0][type]);
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
			temp_variables.push_back(Variable(value, i, type, extract));

			extract.clear();
		}
		i += skip;
	}
	delete[] buffer;
	iwc.close();

	//sort identifier list
	std::sort(indexed_entries.begin(), indexed_entries.end());

	//sort variable list accordingly
	for (UINT i = 0; i < indexed_entries.size(); i++)
	{
		UINT index = indexed_entries[i].index;
		variables.push_back(temp_variables[index]);
	}

	wstring previous_extract;
	UINT group = 0;

	for (UINT i = 0; i < indexed_entries.size(); i++)
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
	return entries.empty() ? ErrorCode(34) : ErrorCode(-1);
}