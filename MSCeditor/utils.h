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

std::wstring ReadRegistry(HKEY root, std::wstring key, std::wstring name);
int CALLBACK AlphComp(LPARAM lp1, LPARAM lp2, LPARAM sortParam);
void OnSortHeader(LPNMLISTVIEW pLVInfo);
LPARAM MakeLPARAM(const unsigned int &i, const unsigned int &j, const bool &negative = false);
void BreakLPARAM(const LPARAM &lparam, int &i, int &j);

void OpenFileDialog();
void InitMainDialog(HWND hwnd);
int SaveFile();
void UnloadFile();
bool CanClose();
inline bool WasModified();
void ClearStatic(HWND hStatic, HWND hDlg);

int CompareStrs(const std::wstring &str1, const std::wstring &str2);
int CompareBolts(const std::wstring &str1, const std::wstring &str2);
void PopulateBList(HWND hwnd, const CarPart *part, unsigned int &item, Overview *ov);
void UpdateBOverview(HWND hwnd, Overview *ov);
void UpdateBListParams(HWND &hList);
void UpdateBDialog();
void UpdateValue(const std::wstring &viewstr, const int &vIndex, const std::string &bin = "");
void UpdateList(const std::wstring &str = _T(""));
template <typename TSTRING> void FormatString(std::wstring &str, const TSTRING &value, const unsigned int &type);
template <typename TSTRING> void FormatValue(std::string &str, const TSTRING &value, const unsigned int &type);

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
bool BinToBolts(const std::string &str, unsigned int &bolts, unsigned int &maxbolts, std::vector<unsigned int> &boltlist);
std::string BoltsToBin(std::vector<unsigned int> &bolts);
bool ContainsStr(const std::wstring &target, const std::wstring &str);
//void ReplaceString(const unsigned int start, std::string &buffer, const std::string &value);
unsigned int VectorStrToBin(std::wstring &str, const unsigned int &size, std::string &bin, const bool allownegative = 1, const bool normalized = 0, const bool eulerconvert = 0, const QTRN *oldq = NULL, const std::string &oldbin = "");
bool QuatEqual(const QTRN *a, const QTRN *b);
QTRN EulerToQuat(const ANGLES *angles);
ANGLES QuatToEuler(const QTRN *q);
//inline double rad2deg(const double &rad);
//inline double deg2rad(const double &degrees);
void TruncFloatStr(std::wstring &str);
void TruncFloatStr(std::string &str);
bool IsValidFloatStr(const std::wstring &str);
//void CreateMainMenu(HWND hWnd);
//std::string FallbackGetValues(unsigned int &index, const char* buffer);
bool ScoopSavegame();
