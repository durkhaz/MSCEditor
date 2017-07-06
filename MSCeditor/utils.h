#pragma once
#include "windows.h"
#include <CommCtrl.h>
#include <string>
#include <vector>
#include "structs.h"
#include "resource.h"
#include "externs.h"

//templates

template <typename TSTRING>
BOOL EntryExists(const TSTRING &str)
{
	int it;
	std::wstring key = StringToWString(str);
	transform(key.begin(), key.end(), key.begin(), ::tolower);
	UINT startindex = (UINT)((((float)key[0] - 97) / 25) * variables.size());
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

template <typename TSTRING>
BOOL FetchDataFileParameters(const TSTRING &str, std::string::size_type &startStr, std::string::size_type &endStr)
{
	startStr = std::string::npos;
	endStr = std::string::npos;

	for (UINT i = 0; i < str.size(); i++)
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

// forward declares

BOOL DownloadUpdatefile(const std::wstring url, std::wstring &path);
BOOL CheckUpdate(std::wstring &file, std::wstring &apppath, std::wstring &changelog);
std::wstring ReadRegistry(const HKEY root, const std::wstring key, const std::wstring name);
int CALLBACK AlphComp(LPARAM lp1, LPARAM lp2, LPARAM sortParam);
void OnSortHeader(LPNMLISTVIEW pLVInfo);
LPARAM MakeLPARAM(const UINT &i, const UINT &j, const bool &negative = false);
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
int CompareBolts(const std::wstring &str1, const std::wstring &str2);
void PopulateBList(HWND hwnd, const CarPart *part, UINT &item, Overview *ov);
void UpdateBOverview(HWND hwnd, Overview *ov);
void UpdateBListParams(HWND &hList);
void UpdateBDialog();
void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin = "");
void UpdateList(const std::wstring &str = _T(""));
template <typename TSTRING> void FormatString(std::wstring &str, const TSTRING &value, const UINT &type);
template <typename TSTRING> void FormatValue(std::string &str, const TSTRING &value, const UINT &type);

std::string BinToFloatStr(const std::string &str);
std::wstring BinToFloatStr(const std::wstring &str);
float BinToFloat(const std::string &str);
std::string FloatStrToBin(const std::string &str);
std::string IntStrToBin(const std::string &str);
std::string IntToBin(int x);
void BatchProcessStuck();
void BatchProcessUninstall();
void BatchProcessBolts(bool fix);
void BatchProcessDamage(bool all);
bool BinToBolts(const std::string &str, UINT &bolts, UINT &maxbolts, std::vector<UINT> &boltlist);
std::string BoltsToBin(std::vector<UINT> &bolts);
int Variables_add(Variable var);
bool Variables_remove(const UINT &index);
bool GroupRemoved(const UINT &group, const UINT &index = UINT_MAX, const bool &IsFirst = FALSE);
UINT ParseItemID(const std::wstring &str, const UINT sIndex);
bool StartsWithStr(const std::wstring &target, const std::wstring &str);
bool ContainsStr(const std::wstring &target, const std::wstring &str);
UINT VectorStrToBin(std::wstring &str, const UINT &size, std::string &bin, const bool allownegative = 1, const bool normalized = 0, const bool eulerconvert = 0, const QTRN *oldq = NULL, const std::string &oldbin = "");
bool QuatEqual(const QTRN *a, const QTRN *b);
QTRN EulerToQuat(const ANGLES *angles);
ANGLES QuatToEuler(const QTRN *q);
void TruncFloatStr(std::wstring &str);
void TruncFloatStr(std::string &str);
bool IsValidFloatStr(const std::wstring &str);
ErrorCode ScoopSavegame();
