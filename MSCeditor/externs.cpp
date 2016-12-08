#include "externs.h"


WNDPROC DefaultListCtrlProc, DefaultListViewProc, DefaultEditProc, DefaultComboProc;
int iItem, iSubItem;

HWND hDialog, hEdit, hCEdit, hTransform, hColor, hString, hTeleport, hReport;
HINSTANCE hInst;
std::vector<std::wstring> entries;
std::vector<Variable> variables;
std::vector<IndexLookup> indextable;
std::vector<TextLookup> locations;
std::vector<CarPart> carparts;
std::vector<SC> partSCs;
std::vector<std::wstring> partIdentifiers;
std::wstring filepath;
SYSTEMTIME filedate;


bool filedateinit = false, MakeBackup = true, EulerAngles = false;
PVOID pResizeState = NULL;

const double kindasmall = 1.0e-6;
const double pi = std::atan(1) * 4;
const TCHAR* bools[2] = { _T("false"), _T("true") };
const TCHAR Title[] = _T("MSCeditor 1.02");
//const KNOWNFOLDERID FOLDERID_LocalAppDataLow = { 0xA520A1A4, 0x1780, 0x4FF6,{ 0xBD, 0x18, 0x16, 0x73, 0x43, 0xC5, 0xAF, 0x16 } };

const std::wstring GLOB_STRS[] =
{
	_T("Input isn't a valid float!"), //0
	_T("Input isn't a valid array!"), //1
	_T("Array contains invalid floats!"), //2
	_T("Input can't be negative!"), //3
	_T("Array can only hold values between 0 and 1!"), //4
	_T("Array can only hold values between -1 and 1!"), //5
	_T("Could not retrieve filedate. Runtime polling disabled. Saving file might corrupt when modified by external source at runtime!"), //6
	_T("\"%s\" was modified.\n\nSave Changes?"), //7
	_T("Input has to be of type FLOAT, e.g. '1.25'"), //8
	_T("STRINGLIST[%d] - Click to edit."), //9
	_T("Reduced %d entries to %d."), //10
	_T("%d unsaved changes."), //11
	_T("Save failure: "), //12
	_T("ifstream fail!"), //13
	_T("Could not rename file!"), //14
	_T("Could not delete file!"), //15
	_T("ofstream fail!"), //16
	_T("The file was modified by another program (maybe the game?). Reload is necessary to avoid corruption!"), //17
	_T("Total Bolts: %d"), //18
	_T("Fastened Bolts: %d"), //19
	_T("Loose Bolts: %d"), //20
	_T("Installed Parts: %d / %d"), //21
	_T("Fixed Parts: %d"), //22
	_T("Loose Parts: %d"), //23
	_T("Damaged Parts: %d"), //24
	_T("Stuck Parts: %d"), //25
	_T("Could not load identifier."), //26
	_T("Rotation Quaternions (x , y , z , w)"), //27
	_T("Rotation Angles (Yaw , Pitch , Roll)"), //28
	_T("Input contains invalid angles") //29
};

const std::wstring TypeStrs[] =
{
	_T("STRINGLIST"),
	_T("TRANSFORM"),
	_T("FLOAT"),
	_T("STRING"),
	_T("BOOLEAN"),
	_T("COLOR"),
	_T("INTEGER")
};

const std::wstring DATATYPES[] =
{
	{ 0x53, 0xFF, 0xEE, 0xF1, 0xE9, 0xFD }, // STRINGLIST
	{ 0xFF, 0x76, 0xFA, 0x7A, 0x09, 0x04 }, // TRANSFORM
	{ 0xFF, 0x6B, 0xD7, 0x3E, 0x6E, 0x00 }, // FLOAT
	{ 0xFF, 0xEE, 0xF1, 0xE9, 0xFD, 0x00 }, // STRING
	{ 0xFF, 0x9C, 0x7C, 0x4D, 0xAD, 0x00 }, // BOOL
	{ 0xFF, 0x31, 0x4B, 0xCF, 0x32, 0x00 }, // COLOR
	{ 0xFF, 0x56, 0x08, 0xA8, 0xE2, 0x00 }  // INT
};

const int ValIndizes[7][2] =
{
	{ 7, -1 }, // STRINGLIST
	{ 6 , 40 }, // TRANSFORM
	{ 5 ,  4 }, // FLOAT
	{ 5 , -1 }, // STRING
	{ 5 ,  1 }, // BOOL
	{ 5 , 16 }, // COLOR
	{ 5 ,  4 }  // INT
};

const std::wstring BListSymbols[] =
{
	_T("-"),
	{ 0x2713 },
	_T("?"),
};

const TCHAR HelpStr[] = _T("Q: What is this?\n\
A: A savegame editor for the game \"My Summer Car\".\n\
Q: Where do I find my savegames?\n\
A: AppData\\LocalLow\\Amistech\\My Summer Car\\\n\
Q: There's 6 files, which one do I open?\n\
A: The entries of interest are inside \"defaultES2File\"\n\
Q: Why do entries show up red?\n\
A: red = unsaved modified entries. Right click to reset\n");

const std::vector<TextLookup> TextTable =
{
	TextLookup(_T("ik"),_T("")),
	TextLookup(_T("_"),_T("")),
	TextLookup(_T(" "),_T("")),
	//some special cases EWWW
	TextLookup(_T("playertransform"),_T("player")),
	TextLookup(_T("motorhoisttransform"),_T("motorhoist")),
	TextLookup(_T("repairshoporder"),_T("repairshop")) //todo: ugly pls fix
};