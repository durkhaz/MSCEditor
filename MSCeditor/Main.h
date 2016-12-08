#pragma once
#include <windows.h> 
#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <CommCtrl.h>
#include <stdlib.h> 
#include <tchar.h> 
#include <fstream> //file stream input output
#include <string> 

#include "resource.h"
#include "externs.h"
#include "structs.h"
#include "dlgprocs.h"
#include "utils.h"
#include "resize.h"

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);