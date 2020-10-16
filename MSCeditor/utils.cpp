#include "utils.h"
#include <shlobj.h> //file dialog and getknownfolderpath
#include "aclapi.h" //ACLs SetEntriesInAcl, EXPLICIT_ACCESS
#include <algorithm> //sort
#include <fstream> //file stream input output
#include <iostream>
#include <cwctype>
#include <cctype>
#include <Urlmon.h>
#include <map>
#include <wrl\client.h> // COM Smart pointer
#include <io.h> // Console
#include <fcntl.h> // Console
#include <iostream> // Console

#ifdef _MAP
#include "map.h"
#endif /*_MAP*/
#undef max
#undef min

using Microsoft::WRL::ComPtr;

void ParseCommandLine(std::wstring& str, std::vector<std::pair<std::wstring, std::wstring>>& args)
{
	for (int32_t i = 0, j = 0;; i++)
	{
		bool bEnd = i == str.size();
		if (str[i] == '-' || bEnd)
		{
			if (i > 1)
			{
				std::wstring arg, id = str.substr(j + 1, i - j - (bEnd ? 1 : 2));
				std::size_t found = id.find(wchar_t(32));
				if (found != std::wstring::npos)
				{
					arg = id.substr(found + 1);
					id.resize(id.size() - arg.size() - 1);
				}
				args.push_back(std::pair<std::wstring, std::wstring>(id, arg));
			}
			j = i;
		}
		if (bEnd)
			return;
	}
}


void CommandLineConsole(std::wstring& arg)
{
	// Alloc debug console
	AllocConsole();
	std::wstring cTitle = Title + L" Debug Console";
	SetConsoleTitle(cTitle.c_str());

	// Get STDOUT handle
	HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
	FILE *COutputHandle = _fdopen(SystemOutput, "w");

	// Get STDERR handle
	HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
	int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
	FILE *CErrorHandle = _fdopen(SystemError, "w");

	// Get STDIN handle
	HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
	int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
	FILE *CInputHandle = _fdopen(SystemInput, "r");

	//make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
	std::ios::sync_with_stdio(true);

	// Redirect the CRT standard input, output, and error handles to the console
	freopen_s(&CInputHandle, "CONIN$", "r", stdin);
	freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
	freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

	// Move console over to other screen and maximize, if possible
	HWND hConsole = GetConsoleWindow();
	SetWindowPos(hConsole, NULL, 3000, 0, 200, 200, SWP_NOSIZE);
	ShowWindow(hConsole, SW_MAXIMIZE);

	std::wcout << cTitle << L" successfully allocated." << std::endl;

	// Log
	dbglog = new DebugOutput(L"MSCEditorLog.txt");
	dbglog->LogNoConsole(L"\n~~~\n\nNEW LOG INSTANCE\n\n~~~\n");
}

void CommandLineFile(std::wstring& arg)
{
	size_t found = arg.find_last_of('\\');
	if (found != std::string::npos)
	{
		// Check if we have an actual file at our hands
		HANDLE hTest = CreateFile(arg.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if (hTest == INVALID_HANDLE_VALUE)
			return;

		LARGE_INTEGER fSize;
		if (!GetFileSizeEx(hTest, &fSize))
			return;

		CloseHandle(hTest);
		filename = arg.substr(found + 1);
		filepath = arg;
		InitMainDialog(hDialog);
	}
}

void AppendPath(std::wstring& path, const wchar_t more[])
{
	std::wstring AppendStr = more;
	while (path.size() > 0)
		if (path.back() == '\\')
			path.pop_back();
		else
			break;

	UINT offset = 0;
	for (; offset < AppendStr.size(); offset++)
		if (AppendStr[offset] != '\\')
			break;
	path += L"\\" + AppendStr.substr(offset);
}

bool IsFolderWritable(const std::wstring& path)
{
	std::wstring filepath = path;
	filepath.resize(MAX_PATH);
	bool bSucc = GetTempFileName(filepath.c_str(), L"TMP", 0, &filepath[0]);
	TruncTailingNulls(&filepath);
	if (!bSucc)
		AppendPath(filepath, L"a");

	std::ofstream owc(filepath.c_str(), std::ofstream::binary);
	bool hr = owc.is_open();
	owc.close();
	DeleteFile(filepath.c_str());
	return hr;
}

HRESULT FindAndCreateAppFolder()
{
	std::wstring DocumentFolder;
	std::wstring AppFolder;

	// Get path to documents folder
	LPWSTR wszPath = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &wszPath);
	if (FAILED(hr))
		throw EditorException(L"SHGetKnownFolderPath() for \"FOLDERID_Documents\" failed.", hr);

	DocumentFolder = wszPath;
	AppendPath(DocumentFolder, L"MySummerCar");
	AppFolder = DocumentFolder;
	AppendPath(AppFolder, L"MSCEditor");
	CoTaskMemFree(wszPath);

	// Does the directory exist?
	DWORD dwAttr = GetFileAttributes(AppFolder.c_str());
	bool bFolderExists = dwAttr != 0xffffffff && (dwAttr & FILE_ATTRIBUTE_DIRECTORY);

	// If it exist, check if it's writable. Try to delete it if it's not
	if (bFolderExists)
	{
		if (IsFolderWritable(AppFolder))
		{
			appfolderpath = AppFolder;
			return S_OK;
		}
		else if (!RemoveDirectory(AppFolder.c_str()))
			throw EditorException(&(std::wstring(L"Directory \"") + AppFolder + std::wstring(L"\" exists but is not writable."))[0], GetLastError());
	}

	// If it doesn't exist or isn't writable we recreate folder
	// Create a well-known SID for the Everyone group
	PSID pEveryoneSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
		throw EditorException(L"SID allocation failed.", GetLastError());

	// Initialize an EXPLICIT_ACCESS structure for the access control entry
	// The ACE will allow all users access to files & folders
	EXPLICIT_ACCESS ea;
	SecureZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = 0xFFFFFFFF;
	ea.grfAccessMode = SET_ACCESS;

	// Both folders & files will inherit this ACE
	ea.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	// Create ACL that contains the new ACE
	PACL pACL = NULL;
	DWORD er = SetEntriesInAcl(1, &ea, NULL, &pACL);
	if (er != ERROR_SUCCESS)
		throw EditorException(L"ACL creation failed.", er);

	// Allocate and initialize security descriptor
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL)
		throw EditorException(L"Security descriptor allocation failed.", reinterpret_cast<uint64_t>(pSD));
	
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		throw EditorException(L"Security descriptor initialization failed.", GetLastError());

	// Add ACL to security descriptor
	if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
		throw EditorException(L"ACL placing failed.", GetLastError());

	// Initialize security attributes structure
	SECURITY_ATTRIBUTES sa;
	SecureZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	BOOL bResult = CreateDirectory(DocumentFolder.c_str(), &sa) || GetLastError() == ERROR_ALREADY_EXISTS;

	if (bResult)
		bResult = CreateDirectory(AppFolder.c_str(), &sa) || GetLastError() == ERROR_ALREADY_EXISTS;

	if (bResult)
		bResult = IsFolderWritable(AppFolder);
		
	if (!bResult)
		throw EditorException(&(std::wstring(L"Failed creating directory \"") + AppFolder + std::wstring(L"\". No write access."))[0], GetLastError());

	if (!HeapFree(GetProcessHeap(), 0, pSD))
		throw EditorException(L"Failed freeing memory", GetLastError());

	appfolderpath = AppFolder;
	return S_OK;
}

BOOL DownloadUpdatefile(const std::wstring url, std::wstring &path)
{
	path = appfolderpath;
	AppendPath(path, L"update.tmp");
	return URLDownloadToFile(NULL, (LPCWSTR)url.c_str(), (LPCWSTR)path.c_str(), 0, NULL);
}

void FlipString(std::string &str, std::wstring file = L"")
{
	for (uint32_t i = 0; i < str.size(); i++)
		str[i] = ~str[i];

	if (!file.empty())
	{
		std::ofstream outf(file, std::ofstream::binary);
		outf << str;
	}
}

BOOL ParseUpdateData(const std::string &str, std::vector<std::string> &strs)
{
	uint32_t i = 0;

	while (TRUE)
	{
		uint32_t size;
		if (str[i] < 0)
			size = 128 - (-128 - str[i]);
		else
			size = (uint32_t)str[i];
		strs.push_back(str.substr(i + 1, size));
		i += 1 + size;
		if ((i + 1) >= str.size())
			break;
	}
	return i == str.size();
}

// true = newer than local version, false = identical or older
BOOL IsRemoteVersionNewer(const std::string &localV, const std::string &remoteV)
{
	uint32_t iMax = static_cast<uint32_t>(std::max(localV.size(), remoteV.size()));
	std::string sl (iMax, '0');
	sl.replace(0, localV.size(), localV);
	std::string sr (iMax, '0');
	sr.replace(0, remoteV.size(), remoteV);

	for (uint32_t i = 0; i < iMax; i++)
	{
		if (isdigit(sl[i]) && isdigit(sr[i]))
		{
			int ln = atoi(&sl[i]);
			int rn = atoi(&sr[i]);
			if (ln != rn)
				return (rn > ln);
		}
	}
	return FALSE;
}

BOOL CheckUpdate(std::wstring &file, std::wstring &apppath, std::wstring &changelog)
{
	using namespace std;

	ifstream iwc(file, ifstream::in, ifstream::binary);
	if (!iwc.is_open())
		return 1;

	iwc.seekg(0, iwc.end);
	uint32_t length = static_cast<int>(iwc.tellg());
	iwc.seekg(0, iwc.beg);
	char *buffer = new char[length + 1];
	memset(buffer, '\0', length + 1);
	iwc.read(buffer, length);
	iwc.close();
	std::string str = buffer;
	delete[] buffer;
	FlipString(str);
	std::vector<std::string> strs;
	bool up2date = TRUE;
	if (ParseUpdateData(str, strs))
	{
		for (uint32_t i = 0; (i + 1) < strs.size(); i++)
		{
			if ((strs[i]) == "changelog")
				changelog = WidenStr(strs[i + 1]);
			if ((strs[i]) == "path1")
				apppath = WidenStr(strs[i + 1]);
			if ((strs[i]) == "version")
				up2date = !IsRemoteVersionNewer(NarrowStr(Version), strs[i + 1]);
		}
	}
	SetFileAttributes(file.c_str(), FILE_ATTRIBUTE_NORMAL);
	DeleteFile(file.c_str());

	return (!up2date && !changelog.empty() && !apppath.empty());
}

// We might have to read the registry to find out where steam installed My Summer Car
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

	HWND hwnd;
	DLGHDR *pHdr = (DLGHDR *)GetWindowLongPtr(hReport, GWLP_USERDATA);
	if (pHdr != nullptr)
		hwnd = pHdr->hwndDisplay;
	else
		return 0;

	ListView_GetItemText(GetDlgItem(hwnd, IDC_BLIST), lp1, Column, (LPWSTR)s1.c_str(), 64);
	ListView_GetItemText(GetDlgItem(hwnd, IDC_BLIST), lp2, Column, (LPWSTR)s2.c_str(), 64);
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

INT_PTR GetLastWriteTime(LPTSTR pszFilePath, SYSTEMTIME &stUTC)
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

LPARAM MakeLPARAM(const uint32_t &i, const uint32_t &j, const bool &negative)
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
		i = static_cast<int>(lparam / LPARAM_OFFSET);
		j = static_cast<int>(lparam - (i * LPARAM_OFFSET));
	}
	else
	{
		i = 0;
		j = static_cast<int>(lparam);
	}
}

// 0 if not equal, 1 if equal without wildcard, 2 if equal with wildcard
BOOL CompareStrsWithWildcard(const std::wstring &StrWithNumber, const std::wstring &StrWithWildcard)
{
	uint32_t offset = 0;
	for (uint32_t i = 0; i < (StrWithNumber.size() - offset); i++)
	{
		if (StrWithWildcard[i + (offset > 0 ? 1 : 0)] == '*')
		{
			for (offset = i; offset < StrWithNumber.size(); offset++)
				if (!std::iswdigit(StrWithNumber[offset]))
					break;
			offset += -static_cast<int>(i);
		}
		wchar_t bla2 = StrWithWildcard[i + (offset > 0 ? 1 : 0)];
		wchar_t bla1 = StrWithNumber[i + offset];
		if (StrWithNumber[i + offset] != StrWithWildcard[i + (offset > 0 ? 1 : 0)])
			return false;
	}
	return StrWithNumber.size() != StrWithWildcard.size() ? FALSE : 1 + offset;
}

int CompareStrs(const std::wstring &str1, const std::wstring &str2)
{
	auto max = str1.size();
	if (str1.size() > str2.size())
		max = str2.size();

	for (uint32_t i = 0; i < max; i++)
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

		bolts1 = static_cast<int>(::strtol(NarrowStr(str1.substr(0, pos - 1)).c_str(), NULL, 10));
		maxbolts1 = static_cast<int>(::strtol(NarrowStr(str1.substr(pos + 1)).c_str(), NULL, 10));
		diff1 = maxbolts1 - bolts1;
	}
	else if (str2.size() > 1) return 1;

	pos = str2.find('/');
	if (pos != std::string::npos)
	{
		bolts2 = static_cast<int>(::strtol(NarrowStr(str2.substr(0, pos - 1)).c_str(), NULL, 10));
		maxbolts2 = static_cast<int>(::strtol(NarrowStr(str2.substr(pos + 1)).c_str(), NULL, 10));
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

	auto max = static_cast<uint32_t>(SendMessage(hList, LVM_GETITEMCOUNT, 0, 0));

	for (uint32_t i = 0; i < max; i++)
	{
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.lParam = i;
		ListView_SetItem(hList, (LPARAM)&lvi);
	}
}

uint32_t GetGroupStartIndex(const uint32_t &group, const uint32_t &index = UINT_MAX)
{
	if (group == 0) return 0;
	if (index == UINT_MAX)
	{
		for (uint32_t i = 0; i < variables.size(); i++)
		{
			if (variables[i].group == group)
				return i;
		}
	}
	else
	{
		if (variables[index].group != group)
			return GetGroupStartIndex(group, UINT_MAX);
		for (uint32_t i = index; i >= 0; i--)
		{
			if (variables[i].group == (group - 1))
				return (i + 1);
		}
	}
	return UINT_MAX;
}

LVITEM GetGroupEntry(const uint32_t &group)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	auto max = static_cast<uint32_t>(SendMessage(hList, LVM_GETITEMCOUNT, 0, 0));
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iItem = -1;
	for (uint32_t i = 0; i < max; i++)
	{
		lvi.iItem = i;
		ListView_GetItem(hList, (LPARAM)&lvi);
		ListParam* listparam = (ListParam*)lvi.lParam;

		if (listparam->GetIndex() == group)
			break;
	}
	return lvi;
}

void UpdateChangeCounter()
{
	//count number of changes made to file

	uint32_t num = 0;
	for (uint32_t i = 0; i < variables.size(); i++)
		if (variables[i].IsModified() || variables[i].IsAdded() || variables[i].IsRemoved() || variables[i].IsRenamed())
			num++;

	// Enable menues when changes were made or if file got deleted
	HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
	num > 0 || filedate.wYear == 0xcccc ? EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_ENABLED) : EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);

	//Update change counter

	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT3), hDialog);
	std::wstring buffer(128, '\0');
	swprintf(&buffer[0], 128, GLOB_STRS[11].c_str(), num, num == 1 ? L"" : L"s");
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)&buffer[0]);
}

bool DatesMatch(SYSTEMTIME stUTC)
{
	if (bFiledateinit)
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
	return TRUE;
}

bool FileChanged()
{
	SYSTEMTIME stUTC;
	bool bFileExists = GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC);

	if (!DatesMatch(stUTC))
	{
		std::wstring buffer(512, '\0');
		swprintf(&buffer[0], 512, GLOB_STRS[17].c_str(), filepath.c_str());
		switch (MessageBox(NULL, buffer.c_str(), ErrorTitle.c_str(), MB_YESNO | MB_ICONWARNING))
		{
			case IDNO:
			{
				filedate = stUTC;
				if (!bFileExists)
					UpdateChangeCounter();
				break;
			}
			case IDYES:
			{
				// Make sure file still exists
				HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hFile);
					InitMainDialog(hDialog);
					break;
				}
				else
				{
					MessageBox(NULL, GLOB_STRS[39].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
					UnloadFile();
				}
				break;
			}

		}
	}
	return FALSE;
}

void OpenFileDialog(std::wstring &fpath, std::wstring &fname)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		ComPtr<IFileOpenDialog> pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(pFileOpen.ReleaseAndGetAddressOf()));

		// Try to set default folder to default savefile path
		if (SUCCEEDED(hr))
		{
			PWSTR pszFilePath;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, NULL, NULL, &pszFilePath)))
			{
				ComPtr<IShellItem> pDefaultFolder;
				std::wstring defaultpath = pszFilePath;
				defaultpath += L"\\Amistech\\My Summer Car\\";
				if (SUCCEEDED(SHCreateItemFromParsingName((PCWSTR)defaultpath.c_str(), NULL, IID_PPV_ARGS(pDefaultFolder.ReleaseAndGetAddressOf()))))
					pFileOpen->SetDefaultFolder(pDefaultFolder.Get());
				CoTaskMemFree(pszFilePath);
			}
		}

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				ComPtr<IShellItem> pItem;
				hr = pFileOpen->GetResult(pItem.ReleaseAndGetAddressOf());
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
				}
			}
		}
	}
	CoUninitialize();
}

void ReloadLists()
{
	HWND hList1 = GetDlgItem(hDialog, IDC_List);
	HWND hList2 = GetDlgItem(hDialog, IDC_List2);
	FreeLPARAMS(hList1);
	SendMessage(hList1, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(hList2, LVM_DELETEALLITEMS, 0, 0);
	indextable.clear();
}

void UnloadFile()
{
	// Will delete tmpfile once handle is closed
	if (hTempFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hTempFile);
		hTempFile = INVALID_HANDLE_VALUE;
	}
		
	bFiledateinit = FALSE;

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
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT1), WM_SETTEXT, 0, (LPARAM)L"");
	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT2), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT2), WM_SETTEXT, 0, (LPARAM)L"");
	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT3), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)L"");
	entries.clear();
	variables.clear();
	indextable.clear();
	carparts.clear();

	HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
	EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 4), MF_GRAYED);

	menu = GetSubMenu(GetMenu(hDialog), 1);
	EnableMenuItem(menu, GetMenuItemID(menu, 0), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 3), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 6), MF_GRAYED);
	EnableMenuItem(menu, GetMenuItemID(menu, 7), MF_GRAYED);

	MENUITEMINFO info = { sizeof(MENUITEMINFO) };
	info.fMask = MIIM_STATE;
	info.fState = MFS_GRAYED;
	SetMenuItemInfo(menu, 4, TRUE, &info);

	for (uint32_t i = 0; i < carproperties.size(); i++)
		carproperties[i].index = UINT_MAX;

	ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT4), hDialog);
	SendMessage(GetDlgItem(hDialog, IDC_OUTPUT4), WM_SETTEXT, 0, (LPARAM)0);

#ifdef _MAP
	if (EditorMap)
		EditorMap->ClearAllMapObjects();
#endif
}

bool CanClose()
{
	if (WasModified())
	{
		std::wstring buffer(128, '\0');
		swprintf(&buffer[0], 128, GLOB_STRS[7].c_str(), filepath.c_str());

		switch (MessageBox(NULL, buffer.c_str(), ErrorTitle.c_str(), MB_YESNOCANCEL | MB_ICONWARNING))
		{
		case IDNO:
			return TRUE;
		case IDYES:
		{
			int result = SaveFile();
			if (result > 0)
			{
				MessageBox(hDialog, (GLOB_STRS[12] + GLOB_STRS[12 + result]).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
				return FALSE;
			}
			else
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
	for (uint32_t i = 0; i < str.size(); i++)
	{
		if (!iswspace(str[i]))
			tmp += str[i];
	}
	str = tmp;
}

std::vector<char> MakeCharArray(std::wstring &str)
{
	std::vector<char> cArray;
	uint32_t iC = 0, iP = 0;
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
		if (params.size() >= 2)
		{
			std::string bin(1, char(4));
			std::wstring coords = params[1];

			VectorStrToBin(coords, 3, bin);
			VectorStrToBin(std::wstring(L"0,0,0,1"), 4, bin);
			VectorStrToBin(std::wstring(L"1,1,1"), 3, bin);
			locations.push_back( 
				std::pair<std::pair<std::wstring, bool>, std::string> 
					(std::pair<std::wstring, bool>(params[0], params.size() == 3 ? static_cast<bool>(params[2][0]) : FALSE), bin));
		}
	}
	else if (identifier == L"Items")
	{
		switch (params.size())
		{
			case 5:
				itemTypes.push_back(Item(params[0], params[1], MakeCharArray(std::wstring(params[2])), NarrowStr(params[3]), params[4]));
				break;
			case 4:
				itemTypes.push_back(Item(params[0], params[1], MakeCharArray(std::wstring(params[2])), NarrowStr(params[3])));
				break;
		}
	}
	else if (identifier == L"Item_Attributes")
	{
		if (params.size() >= 3)
		{
			char c = static_cast<char>(static_cast<int>(::wcstol(params[2].c_str(), NULL, 10)));
			if (params.size() == 3)
				itemAttributes.push_back(ItemAttribute(params[1], c));
			else if (params.size() == 5)
				itemAttributes.push_back(ItemAttribute(params[1], c, static_cast<double>(::wcstod(params[3].c_str(), NULL)), static_cast<double>(::wcstod(params[4].c_str(), NULL))));
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
			std::string param = "";
			if (params.size() == 3)
				param = NarrowStr(params[2]);
			partSCs.push_back(SpecialCase(params[0], static_cast<int>(::strtol(NarrowStr(params[1]).c_str(), NULL, 10)), param));
		}
	}
	else if (identifier == L"Report_Maintenance")
	{
		if (params.size() < 4)
			return;

		auto datatype = static_cast<uint32_t>(::strtol(NarrowStr(params[2]).c_str(), NULL, 10));
		std::string worst = !params[3].empty() ? Variable::ValueStrToBin(params[3], datatype) : "";
		std::string optimum = !params[4].empty() ? Variable::ValueStrToBin(params[4], datatype) : "";

		if (params.size() == 5)
			carproperties.push_back(CarProperty(params[0], params[1], datatype, worst, optimum));
		else if (params.size() == 6)
			carproperties.push_back(CarProperty(params[0], params[1], datatype, worst, optimum, Variable::ValueStrToBin(params[5], datatype)));
	}
	else if (identifier == L"Event_Timetable")
	{
		if (params.size() == 3)
			timetableEntries.push_back(TimetableEntry(params[1], params[2], params[0]));
	}
	else if (identifier == L"Settings")
	{
		if (params.size() >= 2)
		{
			bool bSetting = ::strtol(NarrowStr(params[1]).c_str(), NULL, 10) == 1;
			if (params[0] == settings[0])
				bMakeBackup = bSetting;
			else if (params[0] == settings[1])
				bBackupChangeNotified = bSetting;
			else if (params[0] == settings[2])
				bCheckForUpdate = bSetting;
			else if (params[0] == settings[3])
				bFirstStartup = bSetting;
			else if (params[0] == settings[4])
				bAllowScale = bSetting;
			else if (params[0] == settings[5])
				bEulerAngles = bSetting;
			else if (params[0] == settings[6])
				bDisplayRawNames = bSetting;
			else if (params[0] == settings[7])
				bCheckIssues = bSetting;
			else if (params[0] == settings[8])
				bStartWithMap = bSetting;
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
		for (uint32_t i = 0; i < strInput.size(); i++)
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
	// Sort lists
	std::sort(locations.begin(), locations.end(), [](const std::pair<std::pair<std::wstring, bool>, std::string> &a, const std::pair<std::pair<std::wstring, bool>, std::string> &b) -> bool { return a.first.first < b.first.first; });
	return 0;
}

// Sets numeric item suffix (id). Assumes the item is valid and has an id. Won't do anything if no id was found
void SetItemId(Variable* var, const int id, const Item* ItemType)
{
	for (uint32_t i = static_cast<uint32_t>(ItemType->GetName().size()); i < var->raw_key.size(); i++)
		if (isdigit(var->raw_key.at(i)))
		{
			std::wstring s = var->raw_key;
			var->SetRawKey(s.replace(i, std::to_wstring(::strtol((NarrowStr(s.substr(i))).c_str(), NULL, 10)).size(), std::to_wstring(id)));
			return;
		}
}

// Gets numeric item suffix (id). Returns -1 when no id was found
int GetItemId(const std::wstring& VariableKey, const std::vector<Item>& ItemTypes, const Item* ItemType)
{
	uint32_t i = 0;
	if (ItemType)
		i = isdigit(ItemType->GetName().back()) ? static_cast<uint32_t>(ItemType->GetName().size()) : 0;
	else
		for (auto& cItem : ItemTypes)
		{
			if (VariableKey.substr(0, cItem.GetName().size()) == cItem.GetName())
			{
				i = static_cast<uint32_t>(cItem.GetName().size());
				break;
			}
		}
	for (; i < VariableKey.size(); i++)
		if (isdigit(VariableKey[i]))
			return static_cast<int>(::strtol((NarrowStr(VariableKey.substr(i))).c_str(), NULL, 10));
	return -1;
}

void CleanEmptyItems()
{
	uint32_t NumCleanedItems = 0, NumCleanedVariables = 0, NumFixedKeys = 0;
	std::vector<Item> sortedItemTypes = itemTypes;
	std::sort(sortedItemTypes.begin(), sortedItemTypes.end(), [](const Item &a, const Item &b) -> bool { return a.GetName().size() > b.GetName().size(); });

	// Find items that were consumed, then find the corresponding group for the item, flag it for deletion and append _cleaned suffix
	{
		static std::wstring ConsumedStr = L"Consumed";
		uint32_t CurrentGroup = 0;
		std::vector<uint32_t> GroupVariables;
		bool bIsConsumed = FALSE;
		for (uint32_t i = 0; i < variables.size(); i++)
		{
			Variable& var = variables[i];
			if (var.group != CurrentGroup)
			{
				if (bIsConsumed)
				{
					for (auto& index : GroupVariables)
					{
						if (variables[index].IsRemoved())
							continue;
						variables[index].SetRemoved(TRUE);
						variables[index].SetRawKey(variables[index].raw_key + L"_Cleaned");
						UpdateValue(L"", index, variables[index].value);
						NumCleanedVariables++;
					}
					NumCleanedItems++;
				}

				GroupVariables.clear();
				bIsConsumed = FALSE;
				CurrentGroup = var.group;
			}
			if (var.raw_key.size() > ConsumedStr.size() && var.raw_key.substr(var.raw_key.size() - ConsumedStr.size()) == ConsumedStr && !var.IsRemoved() && GetItemId(var.raw_key, sortedItemTypes, NULL) > 0)
				bIsConsumed = *reinterpret_cast<bool*>(&var.value[0]);
			GroupVariables.push_back(i);
		}
	}
	// Since some entries got renamed, we need to fix up the numeric item suffixes
	if (NumCleanedItems > 0)
	{
		std::vector<std::pair<Item*, uint32_t>> RenamedItemTypes;
		uint32_t CurrentGroup = -1, id = 1;
		Item* PreviousItem = NULL;
		for (uint32_t i = 0; i < variables.size(); i++)
		{
			Item* CurrentItem = NULL;
			std::wstring DEBUG_varname = variables[i].raw_key;
			for (auto& cItem : sortedItemTypes)
			{
				if (variables[i].raw_key.substr(0, cItem.GetName().size()) == cItem.GetName())
				{
					CurrentItem = &cItem;;
					break;
				}
			}
			if (CurrentItem)
			{
				if (RenamedItemTypes.empty() || RenamedItemTypes.back().first->GetName() != CurrentItem->GetName())
					RenamedItemTypes.push_back(std::pair<Item*, uint32_t>(CurrentItem, 0));
				int ParsedId = GetItemId(variables[i].raw_key, sortedItemTypes, CurrentItem);
				if (!variables[i].IsRemoved())
				{
					if (ParsedId > 0 && PreviousItem && PreviousItem == CurrentItem)
					{
						if (CurrentGroup != variables[i].group)
							id++;
					}
					else
						id = ParsedId < 1 ? 0 : 1;
					if (ParsedId > 0 && ParsedId != id)
					{
						NumCleanedVariables++;
						NumFixedKeys++;
						SetItemId(&variables[i], id, CurrentItem);
					}
						
				}
				else if (PreviousItem != CurrentItem)
					id = 0;
				CurrentGroup = variables[i].group;

				if (id > RenamedItemTypes.back().second)
					RenamedItemTypes.back().second = id;
			}
			PreviousItem = CurrentItem;
		}
		// adjust itemIDs
		for (auto& cItemType : RenamedItemTypes)
		{
			uint32_t index = FindVariable(cItemType.first->GetNameID());
			if (index != UINT_MAX)
			{
				if (*reinterpret_cast<const int*>(variables[index].value.data()) != cItemType.second)
				{
					NumCleanedVariables++;
					UpdateValue(std::to_wstring(cItemType.second), index);
				}
			}
			else
			{
				std::wstring buffer(128, '\0');
				swprintf(&buffer[0], 128, GLOB_STRS[65].c_str(), cItemType.first->GetName().c_str());
				MessageBox(hDialog, buffer.c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
			}
		}
		SCROLLINFO si = { sizeof(SCROLLINFO) , SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS , 0 , 0 , 0 , 0 , 0 };
		GetScrollInfo(GetDlgItem(hDialog, IDC_List), SB_VERT, &si);

		PopulateGroups(TRUE, &variables);
		uint32_t size = GetWindowTextLength(GetDlgItem(hDialog, IDC_FILTER)) + 1;
		std::wstring str(size, '\0');
		GetWindowText(GetDlgItem(hDialog, IDC_FILTER), (LPWSTR)str.data(), size);
		str.resize(size - 1);
		UpdateList(str);
		UpdateChangeCounter();
		ListView_Scroll(GetDlgItem(hDialog, IDC_List), 0, static_cast<int>((static_cast<float>(si.nPos) / static_cast<float>(si.nMax)) * static_cast<float>(HIWORD(static_cast<DWORD>(SendMessage(GetDlgItem(hDialog, IDC_List), LVM_APPROXIMATEVIEWRECT, -1, MAKELPARAM(-1, -1)))))));
	}
	std::wstring buffer(256, '\0');
	swprintf(&buffer[0], 256, GLOB_STRS[64].c_str(), NumCleanedItems, NumFixedKeys, NumCleanedVariables);
	MessageBox(NULL, buffer.c_str(), Title.c_str(), MB_OK | MB_ICONINFORMATION);
}

typedef std::pair<uint32_t, uint32_t> vListEntry;
void CompareVariables(const std::vector <Variable> *v1, const std::vector <Variable> *v2, const COMPDLGRET *options)
{
	std::vector<vListEntry> vMissing;		// Missing means the variable is present in v2, but not in v1
	std::vector<vListEntry> vAdditional;	// Additional means the variable is present in v1, but not in v2
	std::vector<vListEntry> vIdentical;		// Identical entries exist in both v1 and v1, and are identical
	std::vector<vListEntry> vDifferent;		// Different entries exist in both v1 and v1, but are different

	std::map <uint32_t, bool> EnabledDatatypes 
	{ 
		{ EntryValue::Transform, options->bTransform }, 
		{ EntryValue::Float, options->bFloat },
		{ EntryValue::String, options->bString },
		{ EntryValue::Bool, options->bBoolean },
		{ EntryValue::Color, options->bColor },
		{ EntryValue::Integer, options->bInteger },
		{ EntryValue::Vector3, options->bVector },
		{ EntryValue::Unknown, options->bUnknown }
	};

	uint32_t i1 = 0, i2 = 0;
	while (true)
	{
		bool bIsAllowedDatatype1 = EnabledDatatypes.at(v1->at(i1).header.GetValueType());
		bool bIsAllowedDatatype2 = EnabledDatatypes.at(v2->at(i2).header.GetValueType());
		bool bNameBlacklisted1 = (!(options->KeyFilter.empty()) && (v1->at(i1).key.find(options->KeyFilter) == std::wstring::npos));
		bool bNameBlacklisted2 = (!(options->KeyFilter.empty()) && (v2->at(i2).key.find(options->KeyFilter) == std::wstring::npos));

		if (!bIsAllowedDatatype1 || bNameBlacklisted1)
			i1++;
		else if (!bIsAllowedDatatype2 || bNameBlacklisted2)
			i2++;
		else if (v1->at(i1).key < v2->at(i2).key)
		{
			vMissing.push_back(vListEntry(i1, UINT_MAX));
			i1++;
		}
		else if (v1->at(i1).key > v2->at(i2).key)
		{
			vAdditional.push_back(vListEntry(UINT_MAX, i2));
			i2++;
		}
		else if (v1->at(i1).key == v2->at(i2).key)
		{
			bool bDifferent = TRUE;
			switch (v1->at(i1).header.GetValueType())
			{
			case EntryValue::Float:
			{
				bDifferent = std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.data()) - *reinterpret_cast<const float*>(v2->at(i2).value.data())) >= options->FloatDelta;
				break;
			}
			case EntryValue::Transform:
			{
				bDifferent =
				(
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(1, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(1, 4).c_str())) >= options->FloatDelta) ||
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(5, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(5, 4).c_str())) >= options->FloatDelta) ||
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(9, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(9, 4).c_str())) >= options->FloatDelta) ||
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(13, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(13, 4).c_str())) >= options->FloatDelta) ||
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(17, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(17, 4).c_str())) >= options->FloatDelta) ||
					(std::abs(*reinterpret_cast<const float*>(v1->at(i1).value.substr(21, 4).c_str()) - *reinterpret_cast<const float*>(v2->at(i2).value.substr(21, 4).c_str())) >= options->FloatDelta)
				);
				break;
			}
			default:
				bDifferent = (v1->at(i1).value != v2->at(i2).value);
			}

			if (bDifferent)
				vDifferent.push_back(vListEntry(i1, i2));
			else
				vIdentical.push_back(vListEntry(i1, i2));
			i1++; i2++;
		}
		if (i1 == v1->size())
		{
			for (i2; i2 < v2->size(); i2++)
				if (bIsAllowedDatatype2 && !bNameBlacklisted2)
					vAdditional.push_back(vListEntry(UINT_MAX, i2));
			break;
		}
		else if (i2 == v2->size())
		{
			for (i1; i1 < v1->size(); i1++)
				if (bIsAllowedDatatype1 && !bNameBlacklisted1)
					vMissing.push_back(vListEntry(i1, INT_MAX));
			break;
		}
	}

	// Here we assemble the html file. We store it in the app folder path for now. 

	std::wstring htm = appfolderpath + L"\\Result.htm";
	std::wofstream file(htm, std::wofstream::out, std::wofstream::trunc);
	if (!file)
		return;

	file << HtmlHeader << std::endl;

	for (auto& CurrentList : { std::make_pair(&vMissing, L"Missing Entries"), std::make_pair(&vAdditional, L"Additional Entries"), std::make_pair(&vDifferent, L"Different Entries"), std::make_pair(&vIdentical, L"Identical Entries") })
	{
		std::wstring buffer(HtmlTableHeader.size() + 64, '\0');
		int bSz = swprintf(&buffer[0], HtmlTableHeader.size() + 64, HtmlTableHeader.c_str(), CurrentList.second);
		file << buffer.substr(0, bSz) << std::endl;

		for (vListEntry& ListEntry : *CurrentList.first)
		{
			buffer.clear();
			buffer = std::wstring(256, '\0');
			bSz = swprintf(&buffer[0], 256, HtmlTableEntry.c_str(), ListEntry.first == UINT_MAX ? v2->at(ListEntry.second).key.c_str() : v1->at(ListEntry.first).key.c_str(), ListEntry.first != UINT_MAX ? v1->at(ListEntry.first).GetDisplayString().c_str() : L"", ListEntry.second != UINT_MAX ? v2->at(ListEntry.second).GetDisplayString().c_str() : L"");
			file << buffer.substr(0, bSz);
		}
		file << L"</table>";
	}
		
	file << HtmlEnd;
	file.close();

	// We open the html in the default app using shell
	std::wstring buffer(128, '\0');
	swprintf(&buffer[0], 128, GLOB_STRS[63].c_str(), htm.c_str());

	if (MessageBox(NULL, buffer.c_str(), ErrorTitle.c_str(), MB_YESNO | MB_ICONINFORMATION) == IDYES)
		ShellExecute(NULL, L"open", &htm[0], NULL, NULL, SW_SHOWDEFAULT);
}

void StrGetLine(const std::wstring &target, std::wstring &str, uint32_t &offset, bool truncEOL = FALSE)
{
	uint32_t i = offset, j = 0;
	for (i; i < target.size(); i++)
	{
		if (target[i] == '\n' || target[i] == '\r')
		{
			if (i + 1 < target.size())
				if (target[i + 1] == '\n' || target[i + 1] == '\r')
					truncEOL ? j = 2 : i += 2;
				else
					truncEOL ? j = 1 : i += 1;
			break;
		}
	}
	i = i - offset;
	str = target.substr(offset, i);
	offset += (i + j);
}

//very ugly but idc

bool SaveSettings(const std::wstring &savefilename)
{
	using namespace std;

	wifstream inf(savefilename);
	if (!inf.is_open()) return FALSE;

	inf.seekg(0, inf.end);
	uint32_t length = static_cast<uint32_t>(inf.tellg());
	inf.seekg(0, inf.beg);
	wstring buffer(length + 1, '\0');
	inf.read((wchar_t*)buffer.data(), length);

	if (buffer.empty())
		return FALSE;

	inf.close();

	std::wstring strInput;
	uint32_t offset = 0, start = UINT_MAX;

	while (offset < buffer.size())
	{
		StrGetLine(buffer, strInput, offset);
		if (ContainsStr(strInput, L"#\"Settings\""))
		{
			start = offset;
			break;
		}

	}

	//first we clear the settings block

	while (offset < buffer.size())
	{
		StrGetLine(buffer, strInput, offset);

		for (uint32_t i = 0; i < strInput.size(); i++)
		{
			if (strInput[i] == '/')
				if (i + 1 < strInput.size())
					if (strInput[i + 1] == '/')
						break;

			if (strInput[i] == '#')
			{
				offset = UINT_MAX;
				break;
			}

			if (strInput[i] == '"')
			{
				buffer.replace(offset - strInput.size(), strInput.size(), L"");
				offset = offset - static_cast<uint32_t>(strInput.size());
				break;
			}
		}
	}
	
	//then we insert the settings

	std::wstring setting;

	setting += L'\"' + settings[0] + L"\" \"" + std::to_wstring(bMakeBackup == 1) + L"\"\n";
	setting += L'\"' + settings[1] + L"\" \"" + std::to_wstring(bBackupChangeNotified == 1) + L"\"\n";
	setting += L'\"' + settings[2] + L"\" \"" + std::to_wstring(bCheckForUpdate == 1) + L"\"\n";
	setting += L'\"' + settings[3] + L"\" \"" + std::to_wstring(bFirstStartup == 1) + L"\"\n";
	setting += L'\"' + settings[4] + L"\" \"" + std::to_wstring(bAllowScale == 1) + L"\"\n";
	setting += L'\"' + settings[5] + L"\" \"" + std::to_wstring(bEulerAngles == 1) + L"\"\n";
	setting += L'\"' + settings[6] + L"\" \"" + std::to_wstring(bDisplayRawNames == 1) + L"\"\n";
	setting += L'\"' + settings[7] + L"\" \"" + std::to_wstring(bCheckIssues == 1) + L"\"\n";
#ifdef _MAP
	setting += L'\"' + settings[8] + L"\" \"" + std::to_wstring(EditorMap != nullptr) + L"\"\n";
#endif
	buffer.insert(start, setting);

	//write to disk

	TruncTailingNulls(&buffer);

	wofstream owc(savefilename, wofstream::trunc);
	if (!owc.is_open()) return FALSE;

	owc.write(buffer.c_str(), buffer.size());
	if (owc.bad() || owc.fail())
	{
		owc.close();
		return FALSE;
	}
	owc.close();

	return TRUE;
}

inline bool WasModified()
{
	for (uint32_t i = 0; i < variables.size(); i++)
	{
		if (variables[i].IsModified() || variables[i].IsAdded() || variables[i].IsRemoved())
		{
			return TRUE;
		}
	}
	return FALSE;
}

int SaveFile()
{
	using namespace std;

	FileChanged();

	if (bMakeBackup)
	{
		// We clean up ALL old txt backup files in directory and rename to bak file extension so it looks nicer and avoid double backups
		wstring directory;
		size_t found = filepath.find_last_of(L"\\");
		if (found != string::npos && ContainsStr(filepath, L"Amistech"))
		{
			directory = filepath.substr(0, found + 1);
			wstring str = directory + L"*_backup*.txt";
			WIN32_FIND_DATA FindFileData;

			HANDLE hFind = FindFirstFile(str.c_str(), &FindFileData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				str = FindFileData.cFileName;
				str.replace(str.size() - 3, 3, L"bak");
				std::wstring oldpath = directory + FindFileData.cFileName;
				std::wstring newpath = directory + str.c_str();
				if (MoveFile(oldpath.c_str(), newpath.c_str()) != 0)
				{
					while (FindNextFile(hFind, &FindFileData))
					{
						str = FindFileData.cFileName;
						str.replace(str.size() - 3, 3, L"bak");
						oldpath = directory + FindFileData.cFileName;
						newpath = directory + str.c_str();
						MoveFile(oldpath.c_str(), newpath.c_str());
					}
				}
				FindClose(hFind);
			}
		}

		// First we remove the file extension

		std::wstring nfilepath = filepath;
		found = nfilepath.find_last_of(L".");
		if (found == string::npos)
			found = nfilepath.size() - 1;
		nfilepath.resize(found);

		nfilepath += L"_backup01.bak";

		if (MoveFileEx(filepath.c_str(), nfilepath.c_str(), MOVEFILE_WRITE_THROUGH) == 0)
		{
			DWORD eword = GetLastError();
			if (eword != ERROR_ALREADY_EXISTS)
			{
				HANDLE hTest = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
				if (hTest == INVALID_HANDLE_VALUE)
					eword = ERROR_SUCCESS; // File doesn't exist, so we just proceed without backup
				else
				{
					CloseHandle(hTest);
					return 2;
				}
			}
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

	typedef std::pair<uint32_t, int64_t> Position;
	vector<Position> offsets;
	for (uint32_t i = 0; i < variables.size(); i++)
		if (!variables[i].IsRemoved())
			offsets.push_back(Position(i, variables[i].pos));

	std::sort(offsets.begin(), offsets.end(), [](const Position &a, const Position &b) -> bool { return a.second < b.second; });

	for (uint32_t i = 0; i < offsets.size(); i++)
		owc << variables[offsets[i].first].MakeEntry();

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
		bFiledateinit = TRUE;
	}

	return 0;
}

bool IsItemFile()
{
	bool bIsItemFile = FALSE;
	if (!variables.empty())
	{
		for (uint32_t i = 0; i < itemTypes.size(); i++)
			if (FindVariable(itemTypes[i].GetNameID()) >= 0)
			{
				bIsItemFile = TRUE;
				break;
			}
	}
	else
		bIsItemFile = (filename.find(L"items") != std::string::npos);

	return bIsItemFile;
}

void LoadLists(HWND hwnd)
{
	// Prepare left List
	{
		LVCOLUMN lvc;
		HWND hList = GetDlgItem(hwnd, IDC_List);
		RECT rekt;
		GetWindowRect(hList, &rekt);
		const int width = rekt.right - rekt.left - 4 - GetSystemMetrics(SM_CXVSCROLL);

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 0; lvc.pszText = L""; lvc.cx = width; lvc.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		ListView_SetBkColor(hList, (COLORREF)GetSysColor(COLOR_WINDOW));
	}
	// Prepare right list
	{
		LVCOLUMN lvc;
		HWND hList = GetDlgItem(hwnd, IDC_List2);
		RECT rekt;
		GetWindowRect(hList, &rekt);
		const int width = rekt.right - rekt.left - 4 - GetSystemMetrics(SM_CXVSCROLL);

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 1; lvc.pszText = L"Key"; lvc.cx = 150; lvc.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 0; lvc.pszText = L"Value"; lvc.cx = (width - 150); lvc.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		ListView_SetBkColor(hList, (COLORREF)GetSysColor(COLOR_WINDOW));
	}
	// If a filter is set, we update list
	{
		uint32_t size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
		std::wstring str(size, '\0');
		GetWindowText(GetDlgItem(hwnd, IDC_FILTER), (LPWSTR)str.data(), size);
		str.resize(size - 1);
		UpdateList(str);
	}
}

void InitMainDialog(HWND hwnd)
{
	UnloadFile();
	try
	{
		auto err = ParseSavegame();
		if (std::get<0>(err) != -1)
		{
			MessageBox(hDialog, (GLOB_STRS[31] + std::to_wstring(std::get<1>(err)) + GLOB_STRS[std::get<0>(err)]).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
			return;
		}
	}
	catch (const std::exception& e) { MessageBox(hDialog, (GLOB_STRS[46] + WidenStr(std::string(e.what()))).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR); return; }

	SYSTEMTIME stUTC;
	if (GetLastWriteTime((LPTSTR)filepath.c_str(), stUTC))
	{
		filedate = stUTC;
		bFiledateinit = TRUE;
	}
	else
	{
		MessageBox(hDialog, GLOB_STRS[6].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
		bFiledateinit = FALSE;
	}

	if (filename.substr(filename.size() - 4) == L".bak")
		MessageBox(hDialog, GLOB_STRS[44].c_str(), L"Info", MB_OK | MB_ICONQUESTION);

	// Temp file, which is just a copy of the currently opened file. Not even used anymore but we'll keep it I guess
	{
		std::wstring tmppath = appfolderpath;
		tmppath.resize(MAX_PATH);
		DWORD dwUID = GetTempFileName(tmppath.c_str(), L"TMP", 0, &tmppath[0]);
		TruncTailingNulls(&tmppath);
		if (!dwUID)
			AppendPath(tmppath, L"\\file.tmp");

		// If GetTempFileName failed, we might have to remove "file.tmp"
		if (!DeleteFile(tmppath.c_str()))
		{
			if (GetLastError() == ERROR_ACCESS_DENIED)
			{
				// File might be read only
				SetFileAttributes(tmppath.c_str(), FILE_ATTRIBUTE_NORMAL);
				DeleteFile(tmppath.c_str());
			}
		}

		// Copy file, open it and store handle
		if (CopyFileEx(filepath.c_str(), tmppath.c_str(), NULL, NULL, FALSE, 0))
			hTempFile = CreateFile(tmppath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	}

	// Hide username from title
	{
		static const std::wstring UsersStr = L"\\Users\\";
		std::wstring TitleStr(128, '\0');
		swprintf(&TitleStr[0], 128,L"%s - [%s]", Title.c_str(), filepath.c_str());
		std::string::size_type Found = TitleStr.find(UsersStr);
		if (Found != std::string::npos)
		{
			std::string::size_type Found2 = TitleStr.find(L"\\", Found + UsersStr.length());
			if (Found2 != std::string::npos)
				TitleStr.replace(Found + UsersStr.length(), Found2 - Found - UsersStr.length(), L"...");
		}
		SetWindowText(hDialog, (LPCWSTR)TitleStr.c_str());
	}

	LoadLists(hwnd);

	// Enable menus
	{
		HMENU menu = GetSubMenu(GetMenu(hDialog), 0);
		EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_ENABLED);
		EnableMenuItem(menu, GetMenuItemID(menu, 4), MF_ENABLED);
		menu = GetSubMenu(GetMenu(hDialog), 1);
		if (!locations.empty())
			EnableMenuItem(menu, GetMenuItemID(menu, 0), MF_ENABLED);
		if (!partIdentifiers.empty())
			EnableMenuItem(menu, GetMenuItemID(menu, 1), MF_ENABLED);
		if (FindVariable(L"keysatsuma") >= 0)
			EnableMenuItem(menu, GetMenuItemID(menu, 2), MF_ENABLED);
		if (FindVariable(L"worldtime") >= 0)
			EnableMenuItem(menu, GetMenuItemID(menu, 3), MF_ENABLED);
		EnableMenuItem(menu, GetMenuItemID(menu, 6), MF_ENABLED);
		EnableMenuItem(menu, GetMenuItemID(menu, 7), MF_ENABLED);

		if (IsItemFile())
		{
			MENUITEMINFO info = { sizeof(MENUITEMINFO) };
			info.fMask = MIIM_STATE;
			info.fState = MFS_ENABLED;
			SetMenuItemInfo(menu, 4, TRUE, &info);
		}
	}
	// Set current changes to 0
	{
		std::wstring buffer(128, '\0');
		swprintf(&buffer[0], 128, GLOB_STRS[11].c_str(), 0, L"s");
		SendMessage(GetDlgItem(hDialog, IDC_OUTPUT3), WM_SETTEXT, 0, (LPARAM)&buffer[0]);
	}
	// Update issue counter
	{
		HWND hIssues = GetDlgItem(hDialog, IDC_OUTPUT4);
		std::vector<Issue> issues;
		PopulateCarparts();
		if (SaveHasIssues(issues))
		{
			ClearStatic(hIssues, hDialog);
			std::wstring buffer(128, '\0');
			swprintf(&buffer[0], 128, GLOB_STRS[50].c_str(), issues.size(), (issues.size() == 1 ? GLOB_STRS[52] : GLOB_STRS[51]).c_str());
			SendMessage(hIssues, WM_SETTEXT, 0, (LPARAM)&buffer[0]);
		}

	}
#ifdef _MAP
	if (!EditorMap)
	{
		if (bStartWithMap)
			OpenMap();
	}
	else
		EditorMap->UpdateAllMapObjects();
#endif /*_MAP*/
}

#ifdef _MAP
void HRToStr(int nErrorCode, std::wstring& hrstr)
{
	DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, nErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &hrstr[0], static_cast<DWORD>(hrstr.size()), NULL);
	if (len == 0)
		len = swprintf(&hrstr[0], hrstr.size(), L"Error code %u", nErrorCode);
	hrstr.resize(len);
}

void ShowObjectOnMap(class Variable* var)
{
	OpenMap();
	EditorMap->ShowObjectOnMap(var);
}

void OpenMap()
{
	try
	{
		if (!EditorMap)
			EditorMap = new MapDialog();
	}
	catch (const std::wstring &str)
	{ 
		size_t sz = str.size() + 64;
		std::wstring buffer(sz, '\0');
		buffer.resize(swprintf(&buffer[0], sz, GLOB_STRS[62].c_str(), str.c_str()));
		MessageBox(hDialog, buffer.c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
	}
}
#endif /*_MAP*/

int GetScrollbarPos(HWND hwnd, int bar, uint32_t code)
{
	SCROLLINFO si = { sizeof(SCROLLINFO) , SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS , 0 , 0 , 0 , 0 , 0 };
	GetScrollInfo(hwnd, bar, &si);

	const int minPos = si.nMin;
	const int maxPos = si.nMax - (si.nPage - 1);

	int result = -1;

	switch (code)
	{
	case SB_LINEUP /*SB_LINELEFT*/:
		result = std::max(si.nPos - 1, minPos);
		break;

	case SB_LINEDOWN /*SB_LINERIGHT*/:
		result = std::min(si.nPos + 1, maxPos);
		break;

	case SB_PAGEUP /*SB_PAGELEFT*/:
		result = std::max(si.nPos - static_cast<int>(si.nPage), minPos);
		break;

	case SB_PAGEDOWN /*SB_PAGERIGHT*/:
		result = std::min(si.nPos + static_cast<int>(si.nPage), maxPos);
		break;

	case SB_THUMBPOSITION:
		// do nothing
		break;

	case SB_THUMBTRACK:
		result = si.nTrackPos;
		break;

	case SB_TOP /*SB_LEFT*/:
		result = minPos;
		break;

	case SB_BOTTOM /*SB_RIGHT*/:
		result = maxPos;
		break;

	case SB_ENDSCROLL:
		// do nothing
		break;
	}
	return result;
}

bool SaveHasIssues(std::vector<Issue> &issues)
{
	for (auto &carpart : carparts)
	{
		if ((carpart.iInstalled != UINT_MAX && !variables[carpart.iInstalled].value[0]) || 
			(carpart.iCorner != UINT_MAX && variables[carpart.iCorner].value.size() == 1))
		{
			if (carpart.iBolted != UINT_MAX && variables[carpart.iBolted].value[0])
				issues.push_back(Issue(carpart.iBolted, std::string(1, '\0')));

			if (carpart.iTightness != UINT_MAX && *reinterpret_cast<const int*>(variables[carpart.iTightness].value.data()) != 0)
				issues.push_back(Issue(carpart.iTightness, IntToBin(0)));

			if (carpart.iBolts != UINT_MAX)
			{
				uint32_t bolts = 0, maxbolts = 0;
				std::vector<uint32_t> boltlist;
				if (BinToBolts(variables[carpart.iBolts].value, bolts, maxbolts, boltlist) && bolts != 0)
					issues.push_back(Issue(carpart.iBolts, BoltsToBin(std::vector<uint32_t>(maxbolts, 0))));
			}
			static const std::wstring PartStr = L"PART";
			const int iTransform = FindVariable(carpart.name);
			if (iTransform >= 0)
			{
				std::string value = variables[iTransform].value;
				if (BinStrToWStr(value.substr(41)) != PartStr)
					issues.push_back(Issue(iTransform, value.replace(41, value.size() - 41, WStrToBinStr(PartStr))));
			}
		}
	}
	const int iTime = FindVariable(L"worldtime");
	if (iTime >= 0)
	{
		int time = *reinterpret_cast<const int*>(variables[iTime].value.data());
		if (time < 0 || time > 22)
			issues.push_back(Issue(iTime, IntToBin(time <= 0 ? 0 : time >= 22 ? 22 : time)));
		else if (time % 2 != 0)
			issues.push_back(Issue(iTime, IntToBin(time + 1)));
	}
	const int iDay = FindVariable(L"worldday");
	if (iDay >= 0)
	{
		int day = *reinterpret_cast<const int*>(variables[iDay].value.data());
		if (day < 1 || day > 7)
			issues.push_back(Issue(iDay, IntToBin(day <= 1 ? 1 : day >= 7 ? 7 : day)));
	}
	return !issues.empty();
}

inline bool PartIsStuck(std::wstring &stuckStr, const std::vector<uint32_t> &boltlist, const uint32_t &tightness, const CarPart *part)
{
	uint32_t boltstate = 0;
	for (uint32_t k = 0; k < boltlist.size(); k++)
		boltstate += boltlist[k];

	if (tightness > boltstate)
	{
		std::wstring buffer(32, '\0');
		swprintf(&buffer[0], 32, L" (%d != %d)", boltstate, tightness);
		stuckStr = BListSymbols[1];
		stuckStr += buffer;
		return TRUE;
	}
	return FALSE;
}

void PopulateCarparts()
{
	if (variables.empty() || partIdentifiers.empty())
		return;

	carparts.clear();

	uint32_t group = 0;
	uint32_t numgroups = variables[variables.size() - 1].group;
	uint32_t varindex = 0;

	while (group < numgroups)
	{
		std::wstring prefix;
		uint32_t i;
		for (i = varindex; i < variables.size() && variables[i].group == group; i++)
		{
			std::size_t index_found = variables[i].key.find(partIdentifiers[3]); // Contains installed?
			if (index_found != std::wstring::npos)
				prefix = variables[i].key.substr(0, index_found);
			else
			{
				index_found = variables[i].key.find(partIdentifiers[1]); // Contains bolts?
				if (index_found != std::wstring::npos)
				{
					bool bValid = TRUE;
					std::wstring bprefix = variables[i].key.substr(0, index_found);
					std::wstring strInstalled = bprefix + partIdentifiers[3];
					for (uint32_t j = varindex; j < variables.size() && variables[j].group == group; j++)
					{
						if (variables[j].key == strInstalled)
						{
							bValid = FALSE;
							break;
						}
					}
					if (bValid)
						prefix = bprefix;
				}
			}
			if (!prefix.empty())
			{
				//filter out special cases
				bool valid = TRUE;
				if (!partSCs.empty())
				{
					for (uint32_t j = 0; j < partSCs.size(); j++)
					{
						if (partSCs[j].id == 2)
						{
							if (partSCs[j].str == prefix)
							{
								valid = FALSE;
								break;
							}
						}
					}
				}
				if (valid)
				{
					CarPart part;

					std::wstring _str1 = prefix + partIdentifiers[3];
					std::wstring _str2 = prefix + partIdentifiers[1];
					std::wstring _str3 = prefix + partIdentifiers[4];
					std::wstring _str4 = prefix + partIdentifiers[2];
					std::wstring _str5 = prefix + partIdentifiers[0];
					std::wstring _str6 = prefix + partIdentifiers[5];

					for (uint32_t j = varindex; j < variables.size() && variables[j].group == group; j++)
					{
						if (variables[j].key == _str1)
							part.iInstalled = j;

						else if (variables[j].key == _str2)
							part.iBolts = j;

						else if (variables[j].key == _str3)
							part.iTightness = j;

						else if (variables[j].key == _str4)
							part.iDamaged = j;

						else if (variables[j].key == _str5)
							part.iBolted = j;

						else if (variables[j].key == _str6)
							part.iCorner = j;
					}
					part.name = prefix;
					carparts.push_back(part);
					prefix.clear();
				}
			}
		}
		varindex = i;
		group++;
	}
}

void PopulateBList(HWND hwnd, const CarPart *part, uint32_t &item, Overview *ov)
{
	HWND hList3 = GetDlgItem(hwnd, IDC_BLIST);

	std::wstring stuckStr = BListSymbols[0];
	std::wstring boltStr = BListSymbols[0];
	LVITEM lvi;

	if (part->iBolts != UINT_MAX && part->iTightness != UINT_MAX)
	{
		uint32_t bolts = 0, maxbolts = 0;
		std::vector<uint32_t> boltlist;

		if (BinToBolts(variables[part->iBolts].value, bolts, maxbolts, boltlist))
		{
			uint32_t tightness = static_cast<uint32_t>(BinToFloat(variables[part->iTightness].value));
			TCHAR buffer[32];
			memset(buffer, 0, 32);
			swprintf(buffer, 32, L"%d / %d", bolts, maxbolts);
			boltStr = buffer;

			ov->numMaxBolts += maxbolts;
			ov->numBolts += bolts;

			bool invalid = (part->iInstalled == UINT_MAX);
			if (!invalid)
				invalid = (variables[part->iInstalled].value[0] == 0x01);
			if (invalid && !(part->iCorner != UINT_MAX && variables[part->iCorner].value.size() == 1))
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
				for (uint32_t j = 0; j < partSCs.size(); j++)
				{
					if (partSCs[j].id == 0)
					{
						if (partSCs[j].str == part->name)
						{
							int offset = static_cast<int>(::strtol((partSCs[j].param).c_str(), NULL, 10));
							tightness >= 8 ? tightness += -offset : tightness = 0;
							break;
						}
					}
				}
			}

			if (PartIsStuck(stuckStr, boltlist, tightness, part)) ov->numStuck++;
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
	if (invalid && !(part->iCorner != UINT_MAX && variables[part->iCorner].value.size() == 1))
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

	std::wstring installedStr;
	if (part->iCorner != UINT_MAX)
		installedStr = variables[part->iCorner].value.size() > 1 ? BListSymbols[1] : BListSymbols[0];
	else
		installedStr = part->iInstalled != UINT_MAX ? ((variables[part->iInstalled].value[0] == 0x01) ? BListSymbols[1] : BListSymbols[0]) : BListSymbols[2];

	lvi.iItem = item; lvi.iSubItem = 3; lvi.pszText = (LPWSTR)installedStr.c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);

	lvi.iItem = item; lvi.iSubItem = 4; lvi.pszText = (LPWSTR)stuckStr.c_str();
	SendMessage(hList3, LVM_SETITEM, 0, (LPARAM)&lvi);
	item++;
}

void UpdateBDialog(HWND &hwnd)
{
	uint32_t item = 0;
	Overview ov;
	HWND hList3 = GetDlgItem(hwnd, IDC_BLIST);
	SendMessage(hList3, WM_SETREDRAW, 0, 0);
	SendMessage(hList3, LVM_DELETEALLITEMS, 0, 0);

	for (uint32_t i = 0; i < carparts.size(); i++)
	{
		PopulateBList(hwnd, &carparts[i], item, &ov);
	}

	SendMessage(hList3, WM_SETREDRAW, 1, 0);

	NMHDR nmhdr = { hList3 , 0 , 0 };
	NMLISTVIEW nmlv = { nmhdr , 0 , INT_MAX , 0 , 0 , 0 , 0 , 0 };
	LPNMLISTVIEW pLVInfo = &nmlv;
	OnSortHeader(pLVInfo);

	//overview
	UpdateBOverview(hwnd, &ov);
}

void UpdateBOverview(HWND hwnd, Overview *ov)
{
	int statics[] = { IDC_BT1 , IDC_BT2 , IDC_BT3 , IDC_BT4, IDC_BT5, IDC_BT6, IDC_BT7, IDC_BT8 };

	for (uint32_t i = 0; i < 8; i++)
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

void UpdateParent(const uint32_t &group)
{
	HWND hList = GetDlgItem(hDialog, IDC_List);
	auto max = static_cast<uint32_t>(SendMessage(hList, LVM_GETITEMCOUNT, 0, 0));

	LVITEM lvi = GetGroupEntry(group);
	if (lvi.iItem != -1)
	{
		bool modified = FALSE;
		uint32_t index = UINT_MAX;

		for (uint32_t j = 0; j < variables.size(); j++)
		{
			if (variables[j].group == group)
			{
				if (index == UINT_MAX)
					index = j;
				if (variables[j].IsModified() || variables[j].IsAdded() || variables[j].IsRemoved() || variables[j].IsRenamed())
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
	for (uint32_t i = 0; i < indextable.size(); i++)
	{
		if (indextable[i].second == vIndex)
		{
			std::wstring out = variables[vIndex].GetDisplayString();
			ListView_SetItemText(GetDlgItem(hDialog, IDC_List2), indextable[i].first, 0, (LPWSTR)out.c_str());
			break;
		}
	}
}

void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin)
{
	std::string str = Variable::ValueStrToBin(viewstr, variables[vIndex].header.GetNonContainerValueType());
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
#ifdef _MAP
	if (EditorMap && variables[vIndex].header.GetNonContainerValueType() == EntryValue::Transform)
		EditorMap->UpdateMapObject(&variables[vIndex]);
#endif
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
		for (uint32_t i = 0; i < entries.size(); i++)
		{
			ListParam *param = new ListParam(0, i);
			uint32_t j;
			for (j = 0; j < variables.size(); j++)
			{
				if (variables[j].group == i) break;
			}
			param->SetFlag(VAR_REMOVED, GroupRemoved(i, j, true));
			for (j; j < variables.size(); j++)
			{
				if (variables[j].group != i) break;
				if (variables[j].IsModified() || variables[j].IsAdded() || variables[j].IsRemoved() || variables[j].IsRenamed())
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
		uint32_t* indizes = new uint32_t[variables.size()];
		uint32_t index = 0;

		for (uint32_t i = 0; i < variables.size(); i++)
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

		for (uint32_t i = 0; i < index; i++)
		{
			ListParam *param = new ListParam(0, indizes[i]);
			uint32_t j;
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
	size_t size = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
	if (size > 1)
	{
		for (uint32_t i = 0; i < size; i++)
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
uint32_t VectorStrToBin(const std::wstring &str, const uint32_t &size, std::string &bin, const bool allownegative, const bool normalized, const bool eulerconvert, const QTRN *oldq, const std::string &oldbin)
{
	std::string::size_type found = 0;
	int *indizes = new int[size + 1]{ -1 };
	indizes[size] = static_cast<int32_t>(str.size());
	uint32_t seperators = 0;

	//kill whitespaces
	std::wstring wstr = str;
	KillWhitespaces(wstr);

	//wrong amount of elements?
	while (TRUE)
	{
		std::string::size_type _i = wstr.substr(found).find(L",");
		if (_i == std::string::npos)
		{
			break;
		}
		else
		{
			found += _i;
			seperators++;
			indizes[seperators] = static_cast<int32_t>(found);
			found++;
		}
	}
	if (seperators != size - 1) return 1;

	//any illegal characters?
	for (uint32_t i = 0; i != size; i++)
	{
		if (!IsValidFloatStr(wstr.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1))) return 2;
		if (allownegative == 0)
		{
			if (wstr.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1)[0] == 45) return 3;
		}
		if (normalized)
		{
			float x = static_cast<float>(::strtod(NarrowStr(wstr.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1)).c_str(), NULL));
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
		a.x = ::strtof(NarrowStr(wstr.substr(indizes[0] + 1, indizes[1] - indizes[0] - 1)).c_str(), NULL);
		a.y = ::strtof(NarrowStr(wstr.substr(indizes[1] + 1, indizes[2] - indizes[1] - 1)).c_str(), NULL);
		a.z = ::strtof(NarrowStr(wstr.substr(indizes[2] + 1, indizes[3] - indizes[2] - 1)).c_str(), NULL);
		QTRN q = EulerToQuat(&a);

		if (!QuatEqual(&q, oldq))
		{
			for (auto& elem : { q.x, q.y, q.z, q.w })
			{
				bin += FloatToBin(elem);
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
		std::wstring element;
		for (uint32_t i = 0; i != size; i++)
		{
			element = wstr.substr(indizes[i] + 1, indizes[i + 1] - indizes[i] - 1);
			bin += FloatStrToBin(element);
		}
	}
	delete[] indizes;
	return 0;
}

void BatchProcessUninstall()
{
	for (uint32_t i = 0; i < carparts.size(); i++)
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
			UpdateValue(L"0", carparts[i].iTightness);
		}

		if (carparts[i].iBolts != UINT_MAX)
		{
			uint32_t bolts = 0, maxbolts = 0;
			std::vector<uint32_t> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				for (uint32_t j = 0; j < boltlist.size(); j++)
				{
					boltlist[j] = 0;
				}
				UpdateValue(L"", carparts[i].iBolts, BoltsToBin(boltlist));
			}
		}

	}
}

void BatchProcessStuck()
{
	for (uint32_t i = 0; i < carparts.size(); i++)
	{
		if ((carparts[i].iBolts != UINT_MAX) && (carparts[i].iTightness != UINT_MAX))
		{
			uint32_t bolts = 0, maxbolts = 0;
			std::vector<uint32_t> boltlist;

			if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist))
			{
				uint32_t boltstate = 0;
				uint32_t tightness = static_cast<uint32_t>(BinToFloat(variables[carparts[i].iTightness].value));

				for (uint32_t j = 0; j < boltlist.size(); j++)
				{
					boltstate += boltlist[j];
				}

				// adjust boltstate for special cases, if there are any
				if (!partSCs.empty())
				{
					for (uint32_t j = 0; j < partSCs.size(); j++)
					{
						if (partSCs[j].id == 0)
						{
							if (partSCs[j].str == carparts[i].name)
							{
								int offset = static_cast<int>(::strtol((partSCs[j].param).c_str(), NULL, 10));
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
	for (uint32_t i = 0; i < carparts.size(); i++)
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
	for (uint32_t i = 0; i < carparts.size(); i++)
	{
		if (carparts[i].iTightness != UINT_MAX && carparts[i].iBolts != UINT_MAX)
		{
			bool invalid = carparts[i].iInstalled == UINT_MAX;
			if (!invalid)
				invalid = (variables[carparts[i].iInstalled].value[0] == 0x01);

			if ((invalid && !(carparts[i].iCorner != UINT_MAX && variables[carparts[i].iCorner].value.size() == 1)) || !fix)
			{
				uint32_t bolts = 0, maxbolts = 0;
				std::vector<uint32_t> boltlist;

				if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist) && (maxbolts != bolts || !fix))
				{
					int boltstate = fix ? 8 : 0;
					std::vector<uint32_t> boltlist;
					for (uint32_t j = 0; j < maxbolts; j++)
					{
						boltlist.push_back(boltstate);
					}
					int tightness = static_cast<uint32_t>(boltlist.size()) * boltstate;

					// adjust tightness for special cases, if there are any
					if (!partSCs.empty() && fix)
					{
						for (uint32_t j = 0; j < partSCs.size(); j++)
						{
							if (partSCs[j].id == 0 && partSCs[j].str == carparts[i].name)
							{
								int offset = static_cast<int>(::strtol((partSCs[j].param).c_str(), NULL, 10));
								tightness >= 8 ? tightness += offset : tightness = 0;
								break;
							}
						}
					}

					if (carparts[i].iBolted != UINT_MAX) UpdateValue(bools[fix], carparts[i].iBolted);
					UpdateValue(L"", carparts[i].iBolts, BoltsToBin(boltlist));
					UpdateValue(std::to_wstring(tightness), carparts[i].iTightness);
				}
			}
		}
	}
}

void BatchProcessWiring()
{
	static const std::wstring WiringIdentifier = L"wiring";
	static const std::vector<std::wstring> WiringRequirements = { L"wiringbatteryminus", L"wiringbatteryplus", L"wiringstarter"};
	std::vector<uint32_t> InstalledParts;

	// We don't wire the car when there's no battery or starter installed
	for (auto& part : carparts)
		for (auto& str : WiringRequirements)
			if ((part.name == str) && (part.iBolts == UINT_MAX || part.iTightness == UINT_MAX))
			{
				std::wstring buffer(128, '\0');
				swprintf(&buffer[0], 128, GLOB_STRS[67].c_str(), part.name.substr(WiringIdentifier.size(), std::wstring::npos).c_str());
				MessageBox(NULL, buffer.c_str(), ErrorTitle.c_str(), MB_ICONERROR | MB_OK);
				return;
			}

	for (uint32_t i = 0; i < carparts.size(); i++)
	{
		if (StartsWithStr(carparts[i].name, WiringIdentifier))
		{
			if (carparts[i].iInstalled != UINT_MAX && !static_cast<bool>(variables[carparts[i].iInstalled].value[0]))
			{
				UpdateValue(bools[TRUE], carparts[i].iInstalled);
				InstalledParts.push_back(i);
			}
				
			// We only set bolted to true when there's also bolts present
			if (carparts[i].iTightness != UINT_MAX && carparts[i].iBolts != UINT_MAX)
			{
				uint32_t bolts = 0, maxbolts = 0;
				std::vector<uint32_t> boltlist;

				if (BinToBolts(variables[carparts[i].iBolts].value, bolts, maxbolts, boltlist) && (maxbolts != bolts))
				{
					std::vector<uint32_t> boltlist;
					for (uint32_t j = 0; j < maxbolts; j++)
						boltlist.push_back(8);
					int tightness = static_cast<int32_t>(boltlist.size()) * 8;

					if (carparts[i].iBolted != UINT_MAX) UpdateValue(bools[TRUE], carparts[i].iBolted);
					UpdateValue(L"", carparts[i].iBolts, BoltsToBin(boltlist));
					UpdateValue(std::to_wstring(tightness), carparts[i].iTightness);
				}
			}
		}
	}
	std::wstring str;
	for (auto& i : InstalledParts)
		str.append(carparts[i].name + L"\n");

	size_t sz = str.size() + GLOB_STRS[66].size() + 64;
	std::wstring str1(sz, '\0');
	swprintf(&str1[0], sz, GLOB_STRS[66].c_str(), InstalledParts.size(), str.c_str());
	MessageBox(NULL, str1.c_str(), Title.c_str(), MB_ICONINFORMATION);
}

bool BinToBolts(const std::string &str, uint32_t &bolts, uint32_t &maxbolts, std::vector<uint32_t> &boltlist)
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
			uint32_t boltstate = static_cast<uint32_t>(::strtol(str.substr(offset + start, end - start).c_str(), NULL, 10));
			boltlist.push_back(boltstate);
			bolts += (boltstate == 8 ? 1 : 0);
			maxbolts++;
		}
		offset += end;
	}
	return (maxbolts != 0) ? TRUE : FALSE;
}

std::string BoltsToBin(std::vector<uint32_t> &bolts)
{
	if (bolts.size() == 0) return "";
	std::string bin;
	bin += IntToBin(static_cast<int32_t>(bolts.size()));

	for (uint32_t i = 0; i < bolts.size(); i++)
	{
		std::string s = "int(" + std::to_string(bolts[i]) + ")";
		char c = char(s.length());
		bin += c + s;
	}

	return bin;
}

// Returns -1 when failure, otherwise index of newly added var
// Really slow because of vector? Should maybe refactor?
int Variables_add(Variable var)
{
	uint32_t index = 0;
	uint32_t group = UINT_MAX;
	for (index; index < variables.size(); index++)
	{
		if (variables[index].key > var.key) break;
		if (variables[index].key == var.key) return -1;
	}

	for (uint32_t i = 0; i < 2; i++)
	{
		int n = (index - i);
		if (n < 0) n = 0;
		if (static_cast<uint32_t>(n) >= static_cast<uint32_t>(variables.size())) 
			n = static_cast<uint32_t>(variables.size()) - 1;
		if (ContainsStr(var.key, entries[variables[n].group]))
			group = variables[n].group;
	}
	if (group == UINT_MAX)
	{
		if (index == 0)
			group = 0;
		else 
			group = variables[index - 1].group + 1;

		for (uint32_t i = index; i < variables.size(); i++)
		{
			variables[i].group += 1;
		}
		entries.insert((entries.begin() + group), var.key); // slow af lol
	}
		
	var.group = group;
	var.SetAdded(TRUE);
	variables.insert((variables.begin() + index), var);
	UpdateChangeCounter();
	return index;
}

bool Variables_remove(const uint32_t &index)
{
	if (index < 0 && index >= variables.size())
		return FALSE;

	uint32_t group = variables[index].group;

	//remove added entries, but highlight standard ones
	if (variables[index].IsAdded())
	{
		variables.erase(variables.begin() + index);

		//when group is empty
		bool GroupIsEmpty = true;
		for (uint32_t i = 0; i < variables.size(); i++)
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
			for (uint32_t i = index; i < variables.size(); i++)
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

bool GroupRemoved(const uint32_t &group, const uint32_t &index, const bool &IsFirst)
{
	uint32_t startindex = index;
	if (!IsFirst) 
		startindex = GetGroupStartIndex(group, index);
	bool removed = TRUE;
	for (uint32_t i = startindex; variables[i].group <= group; i++)
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
	float f = ::wcstof(str.c_str(), NULL);
	return (isnormal(f) || f == 0.f);
}

bool QuatEqual(const QTRN *a, const QTRN *b)
{
	if (abs(a->x) - abs(b->x) > kindasmall) return FALSE;
	if (abs(a->y) - abs(b->y) > kindasmall) return FALSE;
	if (abs(a->z) - abs(b->z) > kindasmall) return FALSE;
	if (abs(a->w) - abs(b->w) > kindasmall) return FALSE;
	return TRUE;
}

float NormalizeAngle(float angle)
{
	while (angle > 360)
		angle -= 360;
	while (angle < 0)
		angle += 360;
	return angle;
}

ANGLES NormalizeAngles(ANGLES *angles)
{
	angles->x = NormalizeAngle(angles->x);
	angles->y = NormalizeAngle(angles->y);
	angles->z =	NormalizeAngle(angles->z);
	return *angles;
}

ANGLES QuatToEuler(const QTRN *q)
{
	// unit correction factor
	float unit = pow(q->x, 2) + pow(q->y, 2) + pow(q->z, 2) + pow(q->w, 2);
	float test = q->x * q->w - q->y * q->z;
	float sign = test > 0.4995f * unit ? 1.f : (test < -0.4995f * unit ? -1.f : 0.f);

	ANGLES v = sign != 0.f ?
		ANGLES((sign * pi / 2.f) * rad2deg, sign * 2.f * std::atan2(q->y, q->x) * rad2deg, 0.f) :
		ANGLES(std::asin(2.f * (q->w * q->x - q->y * q->z)) * rad2deg,
			std::atan2(2.f * q->w * q->y + 2.f * q->z * q->x, 1.f - 2.f * (q->x * q->x + q->y * q->y)) * rad2deg,
			std::atan2(2.f * q->w * q->z + 2.f * q->x * q->y, 1.f - 2.f * (q->z * q->z + q->x * q->x)) * rad2deg);

	return NormalizeAngles(&v);
}

QTRN EulerToQuat(const ANGLES *angles)
{
	float roll = angles->z * deg2Rad * 0.5f;
	float pitch = angles->y * deg2Rad * 0.5f;
	float yaw = angles->x * deg2Rad * 0.5f;

	float t0 = std::sin(roll);
	float t1 = std::cos(roll);
	float t2 = std::sin(pitch);
	float t3 = std::cos(pitch);
	float t4 = std::sin(yaw);
	float t5 = std::cos(yaw);

	return QTRN(
		t4 * t3 * t1 + t5 * t2 * t0,
		t5 * t2 * t1 - t4 * t3 * t0,
		t5 * t3 * t0 - t4 * t2 * t1,
		t5 * t3 * t1 + t4 * t2 * t0);
}

float BinToFloat(const std::string &str)
{
	return str.size() != 4 ? NAN : *reinterpret_cast<const float*>(str.data());
}

std::wstring BinToFloatStr(const std::string &str)
{
	std::wstring s(64, '\0');
	float f = BinToFloat(str);
	int length = std::swprintf(&s[0], s.size(), L"%.10f", f);
	if (length < 0)
		return L"NaN";

	if (isinf(f) && f > 0.f)
		return posInfinity;
	else if (isnan(f) && signbit(f))
		return negInfinity;
		
	s.resize(length);
	return s;
}

std::wstring BinToFloatVector(const std::string &value, int max, int start)
{
	std::wstring VectorStr;
	max += start;

	if ((max * 4) > static_cast<int>(value.size()))
		return L"";

	for (int i = start; i < max; i++)
	{
		std::wstring astr = BinToFloatStr(value.substr((4 * i), 4));
		VectorStr.append(*TruncFloatStr(astr) + L", ");
	}
	VectorStr.resize(VectorStr.size() - 2);
	return VectorStr;
}

std::wstring* TruncFloatStr(std::wstring& str)
{
	std::string out = NarrowStr(str);
	std::string::size_type found = out.find(".");
	if (found != std::string::npos)
	{
		found = out.find_last_not_of("0\t\f\v\n\r");
		if (found != std::string::npos)
		{
			if (out[found] == 46) found--;
			out.resize(found + 1);
		}
		else
			out.clear();
		str = WidenStr(out);
	}
	return &str;
}

std::string NarrowStr(const std::wstring s)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	//std::string wsTmp(s.begin(), s.end());
	std::string wsTmp = converter.to_bytes(s);
	return wsTmp;
}

std::wstring WidenStr(const std::string s)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring wsTmp = converter.from_bytes(s);
	//std::wstring wsTmp(s.begin(), s.end());
	return wsTmp;
}

std::wstring BinStrToWStr(const std::string &str, BOOL bContainsSize)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	try { return converter.from_bytes(str.substr(bContainsSize)); }
	catch (const std::exception& e) { LOG(L"BinStrToWStr: " + WidenStr(std::string(e.what())) + L"\n"); return L""; }
}

std::string WStrToBinStr(const std::wstring &str)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string out;
	try { out = converter.to_bytes(str).substr(0, 255); }
	catch (const std::exception& e) { LOG(L"WStrToBinStr: " + WidenStr(std::string(e.what())) + L"\n"); return "\0"; }
	return out.insert(0, 1, char(out.size()));
}

std::string FloatStrToBin(const std::wstring &str)
{
	std::string out;
	float f = ::wcstof(str.c_str(), NULL);
	if (f == 0.f)
	{
		std::wstring::size_type pos = str.find(posInfinity[0]);
		if (pos != std::wstring::npos)
			f = pos == 0 ? std::numeric_limits<float>::infinity() : -std::numeric_limits<float>::quiet_NaN();
	}
	out.assign(reinterpret_cast<char*>(&f), 4);
	return out;
}

std::string FloatToBin(const float f)
{
	std::string out;
	out.assign(reinterpret_cast<const char*>(&f), 4);
	return out;
}

std::string IntStrToBin(const std::wstring &str)
{
	std::string out;
	int x = static_cast<int>(::wcstol(str.c_str(), NULL, 10));
	out.assign(reinterpret_cast<char*>(&x), 4);
	return out;
}

std::string IntToBin(int x)
{
	std::string out;
	out.assign(reinterpret_cast<char*>(&x), 4);
	return out;
}

uint32_t ParseItemID(const std::wstring &str, const uint32_t sIndex)
{
	uint32_t ItemID = static_cast<uint32_t>(::wcstol(str.substr(sIndex, str.size() - sIndex).c_str(), NULL, 10));
	return ItemID == 0 ? UINT_MAX : ItemID;
}

BOOL FindVariable(const std::wstring &str, const bool bConvert2Lower)
{
	if (variables.empty())
		return -1;
	int it;
	std::wstring key = str;
	if (bConvert2Lower)
		transform(key.begin(), key.end(), key.begin(), ::tolower);
	uint32_t startindex = static_cast<uint32_t>(((static_cast<float>(key[0]) - 97) / 25) * variables.size());
	variables[startindex].key != key ? variables[startindex].key > key ? it = -1 : it = 1 : it = 0;
	if (it != 0)
	{
		for (startindex; startindex < variables.size() && startindex >= 0; startindex += it)
		{
			if (variables[startindex].key == key)
				return startindex;
			if (it == -1 && variables[startindex].key < key)
				return -1;
			if (it == 1 && variables[startindex].key > key)
				return -1;
		}
	}
	else
		return startindex;

	return -1;
}

std::wstring GetItemPrefix(const std::wstring& VariableKey, const std::vector<Item>* SortedItemTypes)
{
	for (auto& item : *SortedItemTypes)
	{
		std::wstring name = *SanitizeTagStr(item.GetName());
		if (VariableKey.substr(0, name.size()) == name)
			for (auto i = static_cast<uint32_t>(name.size()); i < VariableKey.size(); i++)
				if (!isdigit(VariableKey[i]))
					return VariableKey.substr(0, i);
	}
	return std::wstring();
}

// This function is responsible of pooling variables into groups. Variables of the same group will be shown in the list on the right when selected
// Called after parsing the file, and also when we need to resort the list after variabels have been renamed (e.g. CLeanItems)
uint32_t PopulateGroups(bool bRequiresSort,  std::vector<Variable> *pvariables)
{
	if(bRequiresSort)
		std::sort(pvariables->begin(), pvariables->end(), [](const Variable &a, const Variable &b) -> bool { return a.key < b.key; });

	uint32_t group = UINT_MAX;
	bool bIsItemFile = IsItemFile();
	std::wstring previous_extract(1, '\0');
	std::vector<Item> sortedItemTypes;
	entries.clear();

	if (bIsItemFile)
	{
		// we sort by longest itemnames here to avoid substring issues when grouping
		sortedItemTypes = itemTypes;
		std::sort(sortedItemTypes.begin(), sortedItemTypes.end(), [](const Item &a, const Item &b) -> bool { return a.GetName().size() > b.GetName().size(); });
	}

	for (uint32_t i = 0; i < pvariables->size(); i++)
	{
		if (pvariables->at(i).key.substr(0, previous_extract.length()) != previous_extract)
		{
			group++;
			previous_extract = bIsItemFile ? GetItemPrefix(pvariables->at(i).key, &sortedItemTypes) : pvariables->at(i).key;
			if (bIsItemFile && previous_extract.empty())
				previous_extract = pvariables->at(i).key;
			entries.push_back(previous_extract);
		}
		pvariables->at(i).group = group;
	}
	return group;
}

bool StartsWithStrWildcard(const std::wstring &target, const std::wstring &str)
{
	if (str.size() > target.size())
		return FALSE;

	std::wstring tTar = target, tStr = str;
	auto max = static_cast<uint32_t>(str.size());
	for (uint32_t i = 0; i < max; i++)
	{
		tTar[i] = static_cast<wchar_t>(tolower(tTar[i]));
		tStr[i] = static_cast<wchar_t>(tolower(tStr[i]));
		if (tTar[i] == '*')
		{
			tTar.replace(i, 1, L"");
			uint32_t j = i;
			for (; j < max && isdigit(tStr[j]); j++);
			tStr.replace(i, j - i, L"");
			max += -static_cast<int>(j - i);
		}
		if (tTar[i] != tStr[i] && i < max)
			return FALSE;
	}
	return TRUE;
}

bool StartsWithStr(const std::wstring &target, const std::wstring &str)
{
	std::wstring tTar = target, tStr = str;
	transform(tTar.begin(), tTar.end(), tTar.begin(), ::tolower);
	transform(tStr.begin(), tStr.end(), tStr.begin(), ::tolower);

	for (uint32_t i = 0; tTar[i] == tStr[i]; ++i)
	{
		if (i + 1 >= tStr.size())
			return TRUE;
	}
	return FALSE;
}

inline std::wstring ParseBracketStr(std::wstring &str, uint32_t i)
{
	std::wstring out = L"";
	for (i; i < str.size(); i++)
	{
		char c = ::tolower(str[i]);
		if (c == 'x' || c == ')')
			break;
		else
			out += c;
	}
	return out != L"clone" ? out : L"";
}

std::wstring* SanitizeTagStr(std::wstring &str)
{
	std::wstring out = L"";
	wchar_t c;
	for (uint32_t i = 0; i < str.size(); i++)
	{
		c = str[i];
		if (c == 32 || c == 95)
			continue;
		if (c == 40)
		{
			out += ParseBracketStr(str, i + 1);
			i += 6;
			continue;
		}
		out += ::tolower(c);
	}
	for (uint32_t i = 0; i < NameTable.size(); i++)
	{
		auto pos1 = out.find(NameTable[i].first);
		if (pos1 != std::wstring::npos)
		{
			out.replace(pos1, NameTable[i].first.length(), NameTable[i].second);
			break;
		}
	}
	return &str.assign(out);
}

typedef std::pair<int, int64_t> ErrorCode;
ErrorCode ParseSavegame(std::wstring *differentfilepath, std::vector<Variable> *varlist)
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	if (dbglog) 
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start);
		LOG(L"\n");
		LOG(L"Opening File \"" + (differentfilepath == NULL ? filepath : *differentfilepath) + L"\"\n");
	}

	using namespace std;
	int64_t position;
	uint32_t EmptyTagNum = 0;
	ifstream iwc(differentfilepath == NULL ? filepath : *differentfilepath, ios::in | ios::binary);
	std::vector<Variable> *pvariables = varlist ? varlist : &variables;

	if (!iwc.is_open())
		return ErrorCode(13, -1);

	if (iwc.peek() != HX_STARTENTRY)
		return ErrorCode(9, 0);

	while (iwc.good())
	{
		position = iwc.tellg();
		char c;
		if (!iwc.get(c))
			break;

		if (c != HX_STARTENTRY)
			return ErrorCode(9, position + iwc.gcount());

		position = iwc.tellg();
		uint8_t TagSize = iwc.get();
		if (!iwc)
			return ErrorCode(43, position + iwc.gcount());

		position = iwc.tellg();
		string TagStr = string(TagSize, '\0');
		if (!iwc.read(&TagStr[0], TagSize))
			return ErrorCode(43, position + iwc.gcount());

		wstring TagStrRaw = BinStrToWStr(TagStr, FALSE);

		position = iwc.tellg();
		string ValueSizeStr = string(4, '\0');
		iwc.read(&ValueSizeStr[0], 4);
		uint32_t ValueSize = *reinterpret_cast<uint32_t*>(&ValueSizeStr[0]);
		if (!iwc || ValueSize < 6)
			return ErrorCode(32, position + iwc.gcount());

		position = iwc.tellg();
		string ValueStr = string(ValueSize, '\0');
		iwc.read(&ValueStr[0], ValueSize);
		if (!iwc || ValueStr[ValueSize - 1] != HX_ENDENTRY)
			return ErrorCode(33, position + iwc.gcount());

		Header ValueHeader = Header(ValueStr, ValueSize);

		if (ValueSize == UINT_MAX || ValueStr.size() - ValueSize - 1 == 0)
			return ErrorCode(49, position + iwc.gcount());
			
		ValueStr = ValueStr.substr(ValueSize, ValueStr.size() - ValueSize - 1);

		// Entry read in successfully. Now we process
		wstring TagStrFormatted = TagStrRaw;
		if (TagStrFormatted.empty())
			TagStrFormatted = L"untagged" + std::to_wstring(EmptyTagNum++);
		else
			SanitizeTagStr(TagStrFormatted);

		pvariables->push_back(Variable(ValueHeader, ValueStr, static_cast<uint32_t>(pvariables->size()), TagStrRaw, TagStrFormatted));
	}
	iwc.close();
	std::sort(pvariables->begin(), pvariables->end(), [](const Variable &a, const Variable &b) -> bool { return a.key < b.key; } );

	uint32_t NumGroups = UINT_MAX;
	if (!varlist)
		NumGroups = PopulateGroups(FALSE, pvariables);
	
	if (dbglog)
	{
		QueryPerformanceCounter(&end);
		LOG(L"Parsing save-file took " + std::to_wstring((end.QuadPart - start.QuadPart) / static_cast<double>(frequency.QuadPart)) + L" seconds.\n");
		LOG(L"Entries: " + std::to_wstring(pvariables->size()) + L", Groups: " + std::to_wstring(NumGroups + 1) + L"\n");
		std::vector<int> numdts(EntryValue::Num, 0);
		std::vector<std::pair<std::wstring, int>> numcts;
		for (std::vector<Variable>::iterator it = pvariables->begin(); it != pvariables->end(); ++it)
		{
			if (!it->header.IsContainer())
				numdts[it->header.GetValueType()]++;
			else
			{
				bool bExists = FALSE;
				for (auto &ct : numcts)
				{
					if (it->header.GetContainerDisplayString() == ct.first)
					{
						bExists = TRUE;
						ct.second++;
						break;
					}
				}
				if (!bExists)
					numcts.push_back(std::pair<std::wstring, int>(it->header.GetContainerDisplayString(), 1));
			}
		}
		LOG(L"Data Types:\n");
		for (uint32_t i = 0; i < numdts.size(); i++)
		{
			if (numdts[i] > 0)
				LOG(L" - " + EntryValue::Ids[i].second + L": " + std::to_wstring(numdts[i]) + L"\n");
		}
		LOG(L"Container Types:\n");
		for (uint32_t i = 0; i < numcts.size(); i++)
		{
			LOG(L" - " + numcts[i].first.substr(0, numcts[i].first.size() - 3) + L" : " + std::to_wstring(numcts[i].second) + L"\n");
		}
	}
	return pvariables->empty() ? ErrorCode(34, -1) : ErrorCode(-1, -1);
}