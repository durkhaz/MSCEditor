#pragma once
#include "windows.h"
#include "resource.h"
#include "externs.h"
#include "utils.h"
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

BOOL CALLBACK TransformProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TimeWeatherProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK HelpProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ColorProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK KeyManagerProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TeleportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SpawnItemProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK IssueProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListCtrlProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ReportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ReportChildrenProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);

typedef struct tag_plid {
	uint32_t nPID;			// Index of property
	COLORREF cColor;		// color ref
	WNDPROC hDefProc;		// def process
	uint32_t dat;			// some additional data
} PLID;
