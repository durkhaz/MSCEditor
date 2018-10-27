#include "externs.h"
#ifdef _DEBUG
DebugOutput *dbglog;
#endif

WNDPROC DefaultListCtrlProc, DefaultListViewProc;
int iItem;

HWND hDialog, hEdit, hCEdit, hReport;
HINSTANCE hInst;
std::vector<std::wstring> entries;
std::vector<Variable> variables;
std::vector<std::pair<uint32_t, uint32_t>> indextable;
std::vector<std::pair<std::wstring, std::string>> locations;	// Holds all predefined locations, used by the teleport dialog
std::vector<CarPart> carparts;									// This vector is filled when bolt report is opened. Contains all the carparts found
std::vector<Item> itemTypes;									// Types of items in the game, e.g. sausage, beer etc
std::vector<ItemAttribute> itemAttributes;						// Attributes items can have, e.g. transform, consumed etc
std::vector<SpecialCase> partSCs;								// Bolt report special cases
std::vector<std::wstring> partIdentifiers;						// Properties of car parts. Used by the bolt report to detect parts properly
std::vector<CarProperty> carproperties;							// Car properties with min and max values such as wear, fuel and other liquids
std::vector<TimetableEntry> timetableEntries;					// Entry for the timetable inside the time and weather dialog
std::wstring filepath;											// Full file path of currently opened file
std::wstring filename;											// Filename of currently opened file
std::wstring tmpfilepath;										// Path to the temp file
HANDLE hTempFile = INVALID_HANDLE_VALUE;						// Handle of the tempfile. Invalidated upon exiting
SYSTEMTIME filedate;
HFONT hListFont;

bool bFiledateinit = FALSE, bMakeBackup = TRUE, bEulerAngles = FALSE, bCheckForUpdate = TRUE, bBackupChangeNotified = FALSE, bFirstStartup = TRUE, bAllowScale = FALSE, bDisplayRawNames = FALSE;
const std::wstring settings[] = { L"make_backup", L"backup_change_notified", L"check_updates", L"first_startup", L"allow_scale", L"use_euler", L"raw_names" };

PVOID pResizeState = NULL;

const float kindasmall = 1.0e-4f;
const float pi = std::atan(1.f) * 4.f;
const float rad2deg = 180.f / pi;
const float deg2Rad = pi / 180.f;

const std::wstring posInfinity(1, wchar_t(0x221E));
const std::wstring negInfinity = L"-" + posInfinity;
const std::wstring Version = L"1.09";
const std::wstring bools[2] = { L"false", L"true" };
const std::wstring Title = L"MSCEditor " + Version;
const std::wstring IniFile = L"msce.ini";
const std::wstring ErrorTitle = L"Perkele!";

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
	_T("\nCouldn't find start of entry. Expected symbol: ") + std::wstring(1, HX_STARTENTRY), //9   
	_T("Reduced %d entries to %d."), //10
	_T("%d unsaved change%s"), //11
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
	_T("Rotation Angles (Pitch, Yaw, Roll)"), //28
	_T("Input contains invalid angles"), //29
	_T("Something went wrong when trying to write dumpfile."), //30
	_T("Load failure at Offset: "), //31
	_T("\nExpected integer, but got nothing!"), //32
	_T("\nUnexpected end of entry. Expected symbol: ") + std::wstring(1, HX_ENDENTRY), //33    
	_T("\nNo entries!"), //34
	_T("Could not locate file \"") + IniFile + _T("\"!\nStarting program with reduced functionality.\n\n\nPossible solutions:\n\n - Make sure the file is in the same folder as this program.\n - Extract the compressed archive before starting.\n - If file is missing, redownload the program."), //35
	_T("Update available! Update now?\n(Will start download in browser)\n\nChangelog:\n"), //36
	_T("Could not write temporary backup at path:\n\"%s\"\nPlease report this issue."), //37
	_T("There are %d issues with your save that could lead to bugs in the game. Check them out now?\n (You can review the changes before saving)"), //38
	_T("File could not be reloaded because it was renamed or deleted!"), //39
	_T("Successfully wrote dumpfile!"), //40
	_T("Just a heads up, but as of version 1.04,\nsettings no longer reset on launch!\nKeep that in mind."), //41
	_T("Hey buddy! Looks like you're new."), //42
	_T("\nUnexpected EOF!"), //43
	_T("You are about to open a backup file. Is this intentional?"), //44
	_T("\nClick a value to modify, then press set to apply the change or press fix to set it to a recommended value.\n\n\nProgrammers Notes:\nThe values for tuning parts are just my suggestions.\nFor instance, the suggested air/fuel ratio of 14:7 is the stoichiometric mixture that provides the best balance between power and fuel economy. To further decrease fuel consumption, you could make the mixture even leaner. For maximum power, you could set it to something like 13.1. \n\nThe same applies to the spark timing on the distributor. There is no best value for this, as with a racing carburator with N2O, the timing needs to be much higher (~13) than with the twin or stock carburator (~14.8).\n\nThe final gear ratio should be between 3.7 - 4.625. Lower values provide higher top speed but less acceleration.\n\n\nI highly recommend tuning it in the game though, as this is what the game is about ;)\nHave fun!"), //45
	_T("Could not complete action.\n"), //46
	_T("Could not find item ID entry!\n"), //47
	_T("This will remove \"%s\" and can not be undone. Are you sure?\n"), //48
	_T("Container is corrupted and cannot be opened.\nCaught exception:\n"), //49
	_T("%d issue%s found!"), //50
	_T("s were"), //51
	_T(" was"), //52
};

const std::wstring BListSymbols[] =
{
	{ 0x002D },
	{ 0x2713 },
	{ 0x003F }
};

// Keep this alphabetical
const std::vector<std::pair<std::wstring, std::wstring>> NameTable =
{
	{ L"crankbearing", L"mainbearing" },
	{ L"crankwheel", L"crankshaftpulley" },
	{ L"cd1transform", L"cd1" },
	{ L"coffeecuppos", L"coffeecup" },
	{ L"coffeepanpos", L"coffeepan" },
	{ L"computerorderpos", L"computerorder" },
	{ L"fenderflarerf", L"fenderflarerl" },
	{ L"fireextinguisherholder", L"extinguisherholder" },
	{ L"fishtraptransform", L"fishtrap" },
	{ L"floppy1pos", L"floppy1" },
	{ L"floppy2pos", L"floppy2" },
	{ L"gaugeclock", L"clockgauge" },
	{ L"gaugerpm", L"rpmgauge" },
	{ L"ikwishbone", L"wishbone" },
	{ L"kiljubuckettransform", L"kiljubucket" },
	{ L"motorhoisttransform", L"motorhoist" },
	{ L"playertransform", L"player" },
	{ L"rallywheel", L"rallysteeringwheel" },
	{ L"repairshoporder", L"repairshop" },
	{ L"shockrallyfl", L"strutrallyfl" },
	{ L"shockrallyfr", L"strutrallyfr" },
	{ L"shockrallyrl", L"shockabsorberrallyrl" },
	{ L"shockrallyrr", L"shockabsorberrallyrr" },
	{ L"shockfl", L"strutfl" },
	{ L"shockfr", L"strutfr" },
	{ L"shockrl", L"shockabsorberrl" },
	{ L"shockrr", L"shockabsorberrr" },
	{ L"sportwheel", L"sportsteeringwheel" },
	{ L"stocksteeringwheel", L"steeringwheel" },
	{ L"strutflrally", L"strutrallyfl" },
	{ L"strutfrrally", L"strutrallyfr" },
	{ L"valvecover", L"rockercover" },

	{ L"wheelhayosiko1transform", L"wheelhayosiko1" },
	{ L"wheelhayosiko2transform", L"wheelhayosiko2" },
	{ L"wheelhayosiko3transform", L"wheelhayosiko3" },
	{ L"wheelhayosiko4transform", L"wheelhayosiko4" },

	{ L"wheelocto1transform", L"wheelocto1" },
	{ L"wheelocto2transform", L"wheelocto2" },
	{ L"wheelocto3transform", L"wheelocto3" },
	{ L"wheelocto4transform", L"wheelocto4" },

	{ L"wheelracing1transform", L"wheelracing1" },
	{ L"wheelracing2transform", L"wheelracing2" },
	{ L"wheelracing3transform", L"wheelracing3" },
	{ L"wheelracing4transform", L"wheelracing4" },

	{ L"wheelrally1transform", L"wheelrally1" },
	{ L"wheelrally2transform", L"wheelrally2" },
	{ L"wheelrally3transform", L"wheelrally3" },
	{ L"wheelrally4transform", L"wheelrally4" },

	{ L"wheelslot1transform", L"wheelslot1" },
	{ L"wheelslot2transform", L"wheelslot2" },
	{ L"wheelslot3transform", L"wheelslot3" },
	{ L"wheelslot4transform", L"wheelslot4" },

	{ L"wheelspoke1transform", L"wheelspoke1" },
	{ L"wheelspoke2transform", L"wheelspoke2" },
	{ L"wheelspoke3transform", L"wheelspoke3" },
	{ L"wheelspoke4transform", L"wheelspoke4" },

	{ L"wheelsteel1transform", L"wheelsteel1" },
	{ L"wheelsteel2transform", L"wheelsteel2" },
	{ L"wheelsteel3transform", L"wheelsteel3" },
	{ L"wheelsteel4transform", L"wheelsteel4" },
	{ L"wheelsteel5transform", L"wheelsteel5" },

	{ L"wheelsteelwide1transform", L"wheelsteelwide1" },
	{ L"wheelsteelwide2transform", L"wheelsteelwide2" },
	{ L"wheelsteelwide3transform", L"wheelsteelwide3" },
	{ L"wheelsteelwide4transform", L"wheelsteelwide4" },

	{ L"wheelturbine1transform", L"wheelturbine1" },
	{ L"wheelturbine2transform", L"wheelturbine2" },
	{ L"wheelturbine3transform", L"wheelturbine3" },
	{ L"wheelturbine4transform", L"wheelturbine4" },

	{ L"wiringmesstransform", L"wiring" },
};