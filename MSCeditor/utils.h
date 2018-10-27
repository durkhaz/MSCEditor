#pragma once
#include "windows.h"
#include <CommCtrl.h>
#include <string>
#include <vector>
#include "structs.h"
#include "resource.h"
#include "externs.h"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

//templates

template <typename TSTRING>
BOOL FetchDataFileParameters(const TSTRING &str, std::string::size_type &startStr, std::string::size_type &endStr)
{
	startStr = std::string::npos;
	endStr = std::string::npos;

	for (uint32_t i = 0; i < str.size(); i++)
	{
		if (str[i] == '"')
		{
			if (startStr == std::string::npos)
			{
				startStr = i + 1;
				continue;
			}
			if (endStr == std::string::npos)
			{
				endStr = i;
				break;
			}
		}
		if (str[i] == '#')
		{
			if (startStr == std::string::npos || endStr != std::string::npos)
				return 2;
		}
		if (str[i] == '/' && i < str.size() - 1)
			if (str[i + 1] == '/')
				break;
	}
	return (endStr == std::string::npos);
}

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

template <typename TSTRING>
TSTRING* TruncFloatStr(TSTRING &str)
{
	std::string out = WStringToString(str);
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
		str = TSTRING(out.begin(), out.end());
	}
	return &str;
}

template <typename TSTRING1, typename TSTRING2>
inline bool ContainsStr(const TSTRING1 &target, const TSTRING2 &str)
{
	return target.find(str) != TSTRING1::npos;
}


// Forward declarations

BOOL DownloadUpdatefile(const std::wstring url, std::wstring &path);
BOOL CheckUpdate(std::wstring &file, std::wstring &apppath, std::wstring &changelog);
std::wstring ReadRegistry(const HKEY root, const std::wstring key, const std::wstring name);
int CALLBACK AlphComp(LPARAM lp1, LPARAM lp2, LPARAM sortParam);
void OnSortHeader(LPNMLISTVIEW pLVInfo);
LPARAM MakeLPARAM(const uint32_t &i, const uint32_t &j, const bool &negative = false);
void BreakLPARAM(const LPARAM &lparam, int &i, int &j);

void OpenFileDialog();
void InitMainDialog(HWND hwnd);
bool FileChanged();
int SaveFile();
void UnloadFile();
bool CanClose();
inline bool WasModified();
bool SaveSettings(const std::wstring &savefilename);
void ClearStatic(HWND hStatic, HWND hDlg);
void FreeLPARAMS(HWND hwnd);
BOOL LoadDataFile(const std::wstring &datafilename);

int CompareStrs(const std::wstring &str1, const std::wstring &str2);
BOOL CompareStrsWithWildcard(const std::wstring &StrWithNumber, const std::wstring &StrWithWildcard);
int CompareBolts(const std::wstring &str1, const std::wstring &str2);
void PopulateCarparts();
void PopulateBList(HWND hwnd, const CarPart *part, uint32_t &item, Overview *ov);
void UpdateBOverview(HWND hwnd, Overview *ov);
void UpdateBListParams(HWND &hList);
void UpdateBDialog(HWND &hwnd);
void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin = "");
void UpdateList(const std::wstring &str = _T(""));

int GetScrollbarPos(HWND hwnd, int bar, uint32_t code);
std::wstring BinToFloatStr(const std::string &str);
std::string FloatStrToBin(const std::wstring &str);
std::string IntStrToBin(const std::wstring &str);
std::string IntToBin(int x);
typedef std::pair<uint32_t , std::string> Issue;
bool SaveHasIssues(std::vector<Issue> &issues);
void BatchProcessStuck();
void BatchProcessUninstall();
void BatchProcessBolts(bool fix);
void BatchProcessDamage(bool all);
void BatchProcessWiring();
bool BinToBolts(const std::string &str, uint32_t &bolts, uint32_t &maxbolts, std::vector<uint32_t> &boltlist);
std::string BoltsToBin(std::vector<uint32_t> &bolts);
int Variables_add(Variable var);
bool Variables_remove(const uint32_t &index);
bool GroupRemoved(const uint32_t &group, const uint32_t &index = UINT_MAX, const bool &IsFirst = FALSE);
uint32_t ParseItemID(const std::wstring &str, const uint32_t sIndex);
bool StartsWithStrWildcard(const std::wstring &target, const std::wstring &str);
bool StartsWithStr(const std::wstring &target, const std::wstring &str);
BOOL EntryExists(const std::wstring &str, const bool bConvert2Lower = TRUE);
uint32_t VectorStrToBin(const std::wstring &str, const uint32_t &size, std::string &bin, const bool allownegative = 1, const bool normalized = 0, const bool eulerconvert = 0, const QTRN *oldq = NULL, const std::string &oldbin = "");
bool QuatEqual(const QTRN *a, const QTRN *b);
QTRN EulerToQuat(const ANGLES *angles);
ANGLES QuatToEuler(const QTRN *q);
float BinToFloat(const std::string &str);
std::wstring BinStrToWStr(const std::string &str, BOOL bContainsSize = TRUE);
std::string WStrToBinStr(const std::wstring &str);
std::string FloatToBin(const float f);
std::wstring BinToFloatVector(const std::string &value, int max, int start = 0);
bool IsValidFloatStr(const std::wstring &str);
std::wstring* SanitizeTagStr(std::wstring &str);
std::pair<int, int64_t> ParseSavegame();
