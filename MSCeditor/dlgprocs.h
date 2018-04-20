#pragma once
#include "windows.h"
#include "resource.h"
#include "externs.h"
#include "utils.h"
#ifdef _DEBUG
	#include <iostream>
#endif // _DEBUG


BOOL CALLBACK TransformProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK HelpProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ColorProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TeleportProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SpawnItemProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ReportProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ReportChildrenProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

typedef struct tag_plid {
	UINT nPID;			// Index of property
	COLORREF cColor;	
	WNDPROC hDefProc;	// def process
} PLID;