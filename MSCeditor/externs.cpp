#include "externs.h"


WNDPROC DefaultListCtrlProc, DefaultListViewProc, DefaultEditProc, DefaultComboProc;
int iItem, iSubItem;

HWND hDialog, hEdit, hCEdit, hTransform, hColor, hString, hTeleport, hReport;
HINSTANCE hInst;
std::vector<std::wstring> entries;
std::vector<Variable> variables;
std::vector<IndexLookup> indextable;
std::vector<TextLookup> locations;			// Holds all predefined locations, used by the teleport dialog
std::vector<CarPart> carparts;				// This vector is filled when bolt report is opened. Contains all the carparts found
std::vector<Item> itemTypes;				// Types of items in the game, e.g. sausage, beer etc
std::vector<ItemAttribute> itemAttributes;	// Attributes items can have, e.g. transform, consumed etc
std::vector<SC> partSCs;					// Bolt report special cases
std::vector<std::wstring> partIdentifiers;	// Properties of car parts. Used by the bolt report to detect parts properly
std::vector<CarProperty> carproperties;		// Car properties with min and max values such as wear, fuel and other liquids
std::wstring filepath;						// Full file path of currently opened file
std::wstring filename;						// Filename of currently opened file
std::wstring tmpfilepath;					// Path to the temp file
HANDLE hTempFile = INVALID_HANDLE_VALUE;	// Handle of the tempfile. Invalidated upon exiting
SYSTEMTIME filedate;
HFONT hFont;

bool bSaveFromTemp = FALSE, bFiledateinit = FALSE, bMakeBackup = TRUE, bEulerAngles = FALSE, bCheckForUpdate = TRUE, bBackupChangeNotified = FALSE, bFirstStartup = TRUE, bAllowScale = FALSE;
const std::wstring settings[] = { L"make_backup", L"backup_change_notified", L"check_updates", L"first_startup", L"allow_scale" };

PVOID pResizeState = NULL;

const double kindasmall = 1.0e-4;
const double pi = std::atan(1) * 4;
const std::wstring Version = L"1.08";
const std::wstring bools[2] = { L"false", L"true" };
const std::wstring Title = L"MSCEditor " + Version;
const std::wstring IniFile = L"msce.ini";
const std::wstring ErrorTitle = L"Perkele!";
//const KNOWNFOLDERID FOLDERID_LocalAppDataLow = { 0xA520A1A4, 0x1780, 0x4FF6,{ 0xBD, 0x18, 0x16, 0x73, 0x43, 0xC5, 0xAF, 0x16 } };

const std::wstring GLOB_STRS[] =
{
	_T("Input isn't a valid float!"), //0
	_T("Input isn't a valid array!"), //1
	_T("Array contains invalid floats!"), //2
	_T("Input can't be negative!"), //3
	_T("Array can only hold values between 0 and 1!"), //4
	_T("Array can only hold values between -1 and 1!"), //5
	_T("Could not retrieve filedate. Runtime polling disabled. Savefile might corrupt when modified by external source at runtime!"), //6
	_T("\"%s\" was modified.\n\nSave Changes?"), //7
	_T("Input has to be of type FLOAT, e.g. '1.25'"), //8
	_T("ARRAY[%d] - Click to edit."), //9
	_T("Reduced %d entries to %d."), //10
	_T("%d unsaved changes."), //11
	_T("Save failure: "), //12
	_T("\nifstream fail!"), //13
	_T("Could not rename file!"), //14
	_T("Could not delete file!"), //15
	_T("ofstream fail!"), //16
	_T("\"%s\"\n\nThis file has been modified by another program - most likely by the game. Do you wish to load external changes?\n\nClicking yes will discard all unsaved changes made in MSCeditor and reload (recommended)."), //17
	_T("Total Bolts: %d"), //18
	_T("Fastened Bolts: %d"), //19
	_T("Loose Bolts: %d"), //20
	_T("Installed Parts: %d / %d"), //21
	_T("Fixed Parts: %d"), //22
	_T("Loose Parts: %d"), //23
	_T("Damaged Parts: %d"), //24
	_T("Stuck Parts: %d"), //25
	_T("Could not load identifiers. Make sure") + IniFile + _T("is in the same folder as executable."), //26
	_T("Rotation Quaternions (x , y , z , w)"), //27
	_T("Rotation Angles (Yaw , Pitch , Roll)"), //28
	_T("Input contains invalid angles"), //29
	_T("Something went wrong when trying to write dumpfile."), //30
	_T("Load failure at Offset: "), //31
	_T("\nExpected integer, but got nothing!"), //32
	_T("\nUnexpected end of entry. Expected symbol: ") + std::wstring(1, HX_ENDENTRY), //33    
	_T("\nNo entries!"), //34
	_T("Could not locate file \"") + IniFile + _T("\"!\nStarting program with reduced functionality.\n\n\nPossible solutions:\n\n - Make sure the file is in the same folder as this program.\n - Extract the compressed archive before starting.\n - If file is missing, redownload the program."), //35
	_T("Update available! Update now?\n(Will start download in browser)\n\nChangelog:\n"), //36
	_T("Could not write temporary backup at path:\n\"%s\"\nPlease report this issue."), //37
	_T("Could not locate temporary file to restore. Reloading instead!"), //38
	_T("File could not be reloaded because it was renamed or deleted!"), //39
	_T("Successfully wrote dumpfile!"), //40
	_T("Just a heads up, but as of version 1.04,\nsettings no longer reset on launch!\nKeep that in mind."), //41
	_T("Hey buddy! Looks like you're new."), //42
	_T("An instance of MSCeditor is already running."), //43
	_T("You are about to open a backup file. Is this intentional?"), //44
	_T("\nClick a value to modify, then press set to apply the change or press fix to set it to a recommended value.\n\n\nProgrammers Notes:\nThe values for tuning parts are just my suggestions.\nFor instance, the suggested air/fuel ratio of 14:7 is the stoichiometric mixture that provides the best balance between power and fuel economy. To further decrease fuel consumption, you could make the mixture even leaner. For maximum power, you could set it to something like 13.1. \n\nThe same applies to the spark timing on the distributor. There is no best value for this, as with a racing carburator with N2O, the timing needs to be much higher (~13) than with the twin or stock carburator (~14.8).\n\nThe final gear ratio should be between 3.7 - 4.625. Lower values provide higher top speed but less acceleration.\n\n\nI highly recommend tuning it in the game though, as this is what the game is about ;)\nHave fun!"), //45
	_T("Could not complete action.\n"), //46
};

const std::wstring TypeStrs[] =
{
	_T("UNKNOWN"),
	_T("ARRAY"),
	_T("TRANSFORM"),
	_T("FLOAT"),
	_T("STRING"),
	_T("BOOLEAN"),
	_T("COLOR"),
	_T("INTEGER"),
	_T("VECTOR")
};

const std::wstring DATATYPES[] =
{
	{ 0x53, 0xFF, 0xEE, 0xF1, 0xE9, 0xFD, 0x00 },	// ARRAY
	{ 0xFF, 0x76, 0xFA, 0x7A, 0x09, 0x04 },			// TRANSFORM
	{ 0xFF, 0x6B, 0xD7, 0x3E, 0x6E},				// FLOAT
	{ 0xFF, 0xEE, 0xF1, 0xE9, 0xFD},				// STRING
	{ 0xFF, 0x9C, 0x7C, 0x4D, 0xAD},				// BOOL
	{ 0xFF, 0x31, 0x4B, 0xCF, 0x32},				// COLOR
	{ 0xFF, 0x56, 0x08, 0xA8, 0xE2},				// INT
	{ 0xFF, 0x46, 0xDC, 0x66, 0xEC}					// VECTOR
};

const int ValIndizes[8][2] =
{
	{ 7,  -1 },	// ARRAY
	{ 6 , -1 }, // TRANSFORM
	{ 5 ,  4 }, // FLOAT
	{ 5 , -1 }, // STRING
	{ 5 ,  1 }, // BOOL
	{ 5 , 16 }, // COLOR
	{ 5 ,  4 }, // INT
	{ 5 , 12 }, // INT
};

const std::wstring BListSymbols[] =
{
	{ 0x002D },
	{ 0x2713 },
	{ 0x003F }
};

const std::vector<TextLookup> TextTable =
{
	TextLookup(_T("ik"),_T("")),
	TextLookup(_T("_"),_T("")),
	TextLookup(_T(" "),_T(""))
};

// Keep this alphabetical
const std::vector<TextLookup> NameTable =
{
	TextLookup(_T("crankbearing"),_T("mainbearing")),
	TextLookup(_T("crankwheel"),_T("crankshaftpulley")),
	TextLookup(_T("cd1transform"),_T("cd1")),
	TextLookup(_T("coffeecuppos"),_T("coffeecup")),
	TextLookup(_T("coffeepanpos"),_T("coffeepan")),
	TextLookup(_T("computerorderpos"),_T("computerorder")),
	TextLookup(_T("fenderflarerf"),_T("fenderflarerl")),
	TextLookup(_T("fireextinguisherholder"),_T("extinguisherholder")),
	TextLookup(_T("fishtraptransform"),_T("fishtrap")),
	TextLookup(_T("floppy1pos"),_T("floppy1")),
	TextLookup(_T("floppy2pos"),_T("floppy2")),
	TextLookup(_T("gaugeclock"),_T("clockgauge")),
	TextLookup(_T("gaugerpm"),_T("rpmgauge")),
	TextLookup(_T("kiljubuckettransform"),_T("kiljubucket")),
	TextLookup(_T("motorhoisttransform"),_T("motorhoist")),
	TextLookup(_T("playertransform"),_T("player")),
	TextLookup(_T("rallywheel"),_T("rallysteeringwheel")),
	TextLookup(_T("repairshoporder"),_T("repairshop")),
	TextLookup(_T("shockrallyfl"),_T("strutrallyfl")),
	TextLookup(_T("shockrallyfr"),_T("strutrallyfr")),
	TextLookup(_T("shockrallyrl"),_T("shockabsorberrallyrl")),
	TextLookup(_T("shockrallyrr"),_T("shockabsorberrallyrr")),
	TextLookup(_T("shockfl"),_T("strutfl")),
	TextLookup(_T("shockfr"),_T("strutfr")),
	TextLookup(_T("shockrl"),_T("shockabsorberrl")),
	TextLookup(_T("shockrr"),_T("shockabsorberrr")),
	TextLookup(_T("sportwheel"),_T("sportsteeringwheel")),
	TextLookup(_T("stocksteeringwheel"),_T("steeringwheel")),
	TextLookup(_T("strutflrally"),_T("strutrallyfl")),
	TextLookup(_T("strutfrrally"),_T("strutrallyfr")),
	TextLookup(_T("valvecover"),_T("rockercover")),
	
	TextLookup(_T("wheelhayoso1transform"),_T("wheelhayoso1")),
	TextLookup(_T("wheelhayoso2transform"),_T("wheelhayoso2")),
	TextLookup(_T("wheelhayoso3transform"),_T("wheelhayoso3")),
	TextLookup(_T("wheelhayoso4transform"),_T("wheelhayoso4")),

	TextLookup(_T("wheelocto1transform"),_T("wheelocto1")),
	TextLookup(_T("wheelocto2transform"),_T("wheelocto2")),
	TextLookup(_T("wheelocto3transform"),_T("wheelocto3")),
	TextLookup(_T("wheelocto4transform"),_T("wheelocto4")),

	TextLookup(_T("wheelracing1transform"),_T("wheelracing1")),
	TextLookup(_T("wheelracing2transform"),_T("wheelracing2")),
	TextLookup(_T("wheelracing3transform"),_T("wheelracing3")),
	TextLookup(_T("wheelracing4transform"),_T("wheelracing4")),

	TextLookup(_T("wheelrally1transform"),_T("wheelrally1")),
	TextLookup(_T("wheelrally2transform"),_T("wheelrally2")),
	TextLookup(_T("wheelrally3transform"),_T("wheelrally3")),
	TextLookup(_T("wheelrally4transform"),_T("wheelrally4")),

	TextLookup(_T("wheelslot1transform"),_T("wheelslot1")),
	TextLookup(_T("wheelslot2transform"),_T("wheelslot2")),
	TextLookup(_T("wheelslot3transform"),_T("wheelslot3")),
	TextLookup(_T("wheelslot4transform"),_T("wheelslot4")),

	TextLookup(_T("wheelspoke1transform"),_T("wheelspoke1")),
	TextLookup(_T("wheelspoke2transform"),_T("wheelspoke2")),
	TextLookup(_T("wheelspoke3transform"),_T("wheelspoke3")),
	TextLookup(_T("wheelspoke4transform"),_T("wheelspoke4")),

	TextLookup(_T("wheelsteel1transform"),_T("wheelsteel1")),
	TextLookup(_T("wheelsteel2transform"),_T("wheelsteel2")),
	TextLookup(_T("wheelsteel3transform"),_T("wheelsteel3")),
	TextLookup(_T("wheelsteel4transform"),_T("wheelsteel4")),
	TextLookup(_T("wheelsteel5transform"),_T("wheelsteel5")),

	TextLookup(_T("wheelsteelwide1transform"),_T("wheelsteelwide1")),
	TextLookup(_T("wheelsteelwide2transform"),_T("wheelsteelwide2")),
	TextLookup(_T("wheelsteelwide3transform"),_T("wheelsteelwide3")),
	TextLookup(_T("wheelsteelwide4transform"),_T("wheelsteelwide4")),

	TextLookup(_T("wheelturbine1transform"),_T("wheelturbine1")),
	TextLookup(_T("wheelturbine2transform"),_T("wheelturbine2")),
	TextLookup(_T("wheelturbine3transform"),_T("wheelturbine3")),
	TextLookup(_T("wheelturbine4transform"),_T("wheelturbine4")),

	TextLookup(_T("wiringmesstransform"),_T("wiring")),
};