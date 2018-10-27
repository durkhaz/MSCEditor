#pragma once

#include <windows.h> 
#include <vector> 
#include <string> 
#include <tchar.h> 
#include "variable.h"
#include "structs.h"

#ifdef _DEBUG
#include <fstream>		// File stream input output
class DebugOutput
{
public:
	DebugOutput(const wchar_t* fileName)
	{
		m_fileStream.open(fileName, std::wofstream::app);
	}
	~DebugOutput()
	{
		m_fileStream.flush();
		m_fileStream.close();
	}
	std::wofstream m_fileStream;

	static std::wstring GetTime()
	{
		SYSTEMTIME time;
		GetLocalTime(&time);

		std::wstring buffer(128, '\0');
		buffer.resize(swprintf(&buffer[0], 128, L"[%02d:%02d:%02d] ", time.wHour, time.wMinute, time.wSecond));
		return buffer;
	}
	void LogNoConsole(const std::wstring &str)
	{
		m_fileStream << str;
	}
};



template <typename T> DebugOutput& operator<< (DebugOutput& stream, T val)
{
	stream.m_fileStream << val;
	std::wcout << val;
	return stream;
};

extern DebugOutput *dbglog;

#define LOG(val) *dbglog << DebugOutput::GetTime() << val; dbglog->m_fileStream.flush()
#else
#define LOG(val) while(false)
#endif


#define HX_STARTENTRY	0x7E
#define HX_ENDENTRY		0x7B
#define LPARAM_OFFSET	100000



extern WNDPROC DefaultListCtrlProc, DefaultListViewProc;
extern int iItem, iSubItem;
extern HWND hDialog, hEdit, hCEdit, hReport;
extern HINSTANCE hInst;
extern std::vector<std::wstring> entries;
extern std::vector<Variable> variables;
extern std::vector<std::pair<uint32_t, uint32_t>> indextable;
extern std::vector<std::pair<std::wstring, std::string>> locations;
extern std::vector<Item> itemTypes;
extern std::vector<ItemAttribute> itemAttributes;
extern std::vector<CarPart> carparts;
extern std::vector<SpecialCase> partSCs;
extern std::vector<std::wstring> partIdentifiers;
extern std::vector<CarProperty> carproperties;
extern std::vector<TimetableEntry> timetableEntries;
extern std::wstring filepath;
extern std::wstring filename;
extern std::wstring tmpfilepath;
extern HANDLE hTempFile;
extern SYSTEMTIME filedate;
extern HFONT hListFont;

extern bool bFiledateinit, bMakeBackup, bEulerAngles, bCheckForUpdate, bBackupChangeNotified, bFirstStartup, bAllowScale, bDisplayRawNames;

extern PVOID pResizeState;

extern const float kindasmall;

extern const float pi;

extern const float rad2deg;

extern const float deg2Rad;

extern const double pid;

extern const double deg2Radd;

extern const std::wstring bools[2];

extern const std::wstring posInfinity;

extern const std::wstring negInfinity;

extern const std::wstring Version;

extern const std::wstring Title;

extern const std::wstring IniFile;

extern const std::wstring ErrorTitle;

extern const std::wstring settings[];

extern const std::wstring GLOB_STRS[];

extern const std::wstring BListSymbols[];

extern const TCHAR HelpStr[];

extern const std::vector<std::pair<std::wstring, std::wstring>> NameTable;

typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;

typedef struct tag_dlghdr {
	HWND hwndTab;       // tab control 
	HWND hwndDisplay;   // current child dialog box 
	RECT rcDisplay;     // display rectangle for the tab control 
	DLGTEMPLATEEX *apRes[2];
} DLGHDR;