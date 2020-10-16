#pragma once
#include "windows.h"
#include "resource.h"
#include "externs.h"
#include "utils.h"
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

INT_PTR CALLBACK TransformProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TimeWeatherProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ColorProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK KeyManagerProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CompareProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TeleportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SpawnItemProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK IssueProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListCtrlProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ReportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ReportChildrenProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);

typedef struct tag_plid {
	uint32_t nPID;			// Index of property
	COLORREF cColor;		// color ref
	WNDPROC hDefProc;		// def process
	uint32_t dat;			// some additional data
} PLID;
