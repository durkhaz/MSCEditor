#pragma once

#include "structs.h"
#include <windows.h> 
#include <vector> 
#include <string> 
#include <tchar.h> 
#include "variable.h"

#define HX_STARTENTRY	0x7E
#define HX_ENDENTRY		0x7B

#define ID_STRINGL		0
#define ID_TRANSFORM	1
#define ID_FLOAT		2
#define ID_STRING		3
#define ID_BOOL			4
#define ID_COLOR		5
#define ID_INT			6
#define ID_VECTOR		7

#define LPARAM_OFFSET	100000

extern WNDPROC DefaultListCtrlProc, DefaultListViewProc, DefaultEditProc, DefaultComboProc;
extern int iItem, iSubItem;
extern HWND hDialog, hEdit, hCEdit, hTransform, hColor, hString, hTeleport, hReport;
extern HINSTANCE hInst;
extern std::vector<std::wstring> entries;
extern std::vector<Variable> variables;
extern std::vector<IndexLookup> indextable;
extern std::vector<TextLookup> locations;
extern std::vector<Item> itemTypes;
extern std::vector<ItemAttribute> itemAttributes;
extern std::vector<CarPart> carparts;
extern std::vector<SC> partSCs;
extern std::vector<std::wstring> partIdentifiers;
extern std::vector<CarProperty> carproperties;
extern std::wstring filepath;
extern std::wstring filename;
extern std::wstring tmpfilepath;
extern HANDLE hTempFile;
extern SYSTEMTIME filedate;
extern HFONT hFont;

extern bool bSaveFromTemp, bFiledateinit, bMakeBackup, bEulerAngles, bCheckForUpdate, bBackupChangeNotified, bFirstStartup, bAllowScale, bListProcessed;

extern PVOID pResizeState;

extern const double kindasmall;

extern const double pi;

extern const std::wstring bools[2];

extern const std::wstring Version;

extern const std::wstring Title;

extern const std::wstring IniFile;

extern const std::wstring settings[];

extern const std::wstring GLOB_STRS[];

extern const std::wstring TypeStrs[];

extern const std::wstring DATATYPES[];

extern const int ValIndizes[8][2];

extern const std::wstring BListSymbols[];

extern const TCHAR HelpStr[];

extern const std::vector<TextLookup> TextTable;

extern const std::vector<TextLookup> NameTable;

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
	// 	sz_Or_Ord menu;
	// 	sz_Or_Ord windowClass;
	// 	WCHAR title[titleLen];
	// 	WORD pointsize;
	// 	WORD weight;
	// 	BYTE italic;
	// 	BYTE charset;
	// 	WCHAR typeface[stringLen];
} DLGTEMPLATEEX;

typedef struct tag_dlghdr {
	HWND hwndTab;       // tab control 
	HWND hwndDisplay;   // current child dialog box 
	RECT rcDisplay;     // display rectangle for the tab control 
	DLGTEMPLATEEX *apRes[2];
} DLGHDR;