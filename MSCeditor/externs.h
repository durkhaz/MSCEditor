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
extern std::wstring filepath;
extern std::wstring filename;
extern SYSTEMTIME filedate;
extern HFONT hFont;

extern bool filedateinit, MakeBackup, EulerAngles, CheckForUpdate, backup_change_notified, first_startup;

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

extern const int ValIndizes[7][2];

extern const std::wstring BListSymbols[];

extern const TCHAR HelpStr[];

extern const std::vector<TextLookup> TextTable;

//const KNOWNFOLDERID FOLDERID_LocalAppDataLow = { 0xA520A1A4, 0x1780, 0x4FF6,{ 0xBD, 0x18, 0x16, 0x73, 0x43, 0xC5, 0xAF, 0x16 } };
//char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B','C','D','E','F' };