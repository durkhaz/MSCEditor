#include "dlgprocs.h"
#include <CommCtrl.h>
#define		WM_L2CONTEXTMENU	(WM_USER + 10)
#define		WM_DRAWINFOICON		(WM_USER + 11)
#define		WM_UPDATESIZE		(WM_USER + 12)

#define		STR_WEAR			_T("wear")
#define		STR_STATE			_T("condition")
#define		STR_GOOD			_T("Good")
#define		STR_BAD				_T("Bad")
#define		CLR_GOOD			RGB(153, 185, 152)
#define		CLR_OK				RGB(253, 206, 170)
#define		CLR_BAD				RGB(244, 131, 125)
// Kinda messy to do this with CID ranges but it seemed easier at the time. I don't really feel like refactoring this now.
#define		ID_PL_BASE_OFFSET	10000
#define		ID_PL_DTXT			50000
#define		ID_PL_STATE			60000
#define		ID_PL_EDIT			70000
#define		ID_PL_BUTTON		80000

#undef max
#undef min

// ======================
// dlg utils
// ======================

// Loads and locks a dialog box template resource. 
// Returns the address of the locked dialog box template resource. 
// lpszResName - name of the resource. 

DLGTEMPLATEEX* DoLockDlgRes(LPCTSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);
	return (DLGTEMPLATEEX *)LockResource(hglb);
}

inline void ShowBalloontip(HWND hwnd, UINT msgIndex)
{
	EDITBALLOONTIP ebt;
	ebt.cbStruct = sizeof(EDITBALLOONTIP);
	ebt.pszTitle = _T("Oops!");
	ebt.ttiIcon = TTI_ERROR_LARGE;
	ebt.pszText = GLOB_STRS[msgIndex].c_str();
	SendMessage(hwnd, EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
}

bool CloseEditbox(HWND hwnd, HWND hOutput)
{
	int size = GetWindowTextLength(hwnd) + 1;
	bool valid = TRUE;
	std::wstring value(size - 1, '\0');
	GetWindowText(hwnd, (LPWSTR)value.data(), size);
	UINT type = variables[indextable[iItem].index2].type;
	if (type == ID_FLOAT)
	{
		valid = IsValidFloatStr(value);
		if (valid)
			TruncFloatStr(value);
	}
	
	valid ? UpdateValue(value, indextable[iItem].index2) : ShowBalloontip(hOutput, 8);
	DestroyWindow(hwnd);
	hEdit = NULL;
	return TRUE;
}

inline std::wstring GetEditboxString(HWND hEdit)
{
	UINT size = GetWindowTextLength(hEdit);
	std::wstring str(size, '\0');
	GetWindowText(hEdit, (LPWSTR)str.data(), size + 1);
	return str;
}

// Returns whether actions are required (for the fix button)
bool GetStateLabelSpecs(const CarProperty *cProp, std::wstring &sLabel, COLORREF &tColor)
{
	if (cProp == nullptr)
		return FALSE;

	sLabel = _T(" ");
	tColor = (COLORREF)GetSysColor(COLOR_WINDOW);
	bool bActionAvailable = FALSE;

	if (cProp->worst != -1.f)
	{
		float fMin = std::min(cProp->worst, cProp->optimum);
		float fMax = std::max(cProp->worst, cProp->optimum);
		float fValue = BinToFloat(variables[cProp->index].value);

		// If recommended is defined (not NAN), then we check if it's in range of min max
		if (cProp->recommended == cProp->recommended)
		{
			if (fValue > fMax || fValue < fMin)
			{
				sLabel += STR_BAD;
				tColor = CLR_BAD;
				bActionAvailable = TRUE;
			}
			else
			{
				sLabel += STR_GOOD;
				tColor = CLR_GOOD;
			}
		}
		// Otherwise we calculate percentage
		else
		{
			fMax -= fMin;
			fValue = std::max(fMin, std::min((fValue - fMin), fMax));
			if (cProp->worst > cProp->optimum)
				fValue = cProp->worst - fValue;
			int percentage = static_cast<int>((fValue / fMax) * 100);
			sLabel += std::to_wstring(percentage);
			sLabel += _T("%");
			if (percentage < 100)
				bActionAvailable = TRUE;
			tColor = (percentage >= 75) ? CLR_GOOD : ((percentage < 15) ? CLR_BAD : CLR_OK);
		}
	}
	else
	{
		float fDelta = std::abs(cProp->optimum - BinToFloat(variables[cProp->index].value));
		if (fDelta <= kindasmall)
		{
			sLabel += STR_GOOD;
			tColor = CLR_GOOD;
		}
	}
	sLabel += _T(" ");
	return bActionAvailable;
}

void UpdateRow(HWND hwnd, UINT pIndex, int nRow, std::wstring str = _T(""))
{
	std::wstring sValue = str;
	bool bHasRecommended = carproperties[pIndex].recommended == carproperties[pIndex].recommended;
	if (str.empty())
	{
		float fValue = bHasRecommended ? carproperties[pIndex].recommended : carproperties[pIndex].optimum;
		sValue = std::to_wstring(fValue);
	}
	else if (!bHasRecommended && (carproperties[pIndex].worst != -1 && carproperties[pIndex].optimum != -1))
	{
		float fValue = static_cast<float>(::strtod(WStringToString(sValue).c_str(), NULL));
		float fMin = std::min(carproperties[pIndex].worst, carproperties[pIndex].optimum);
		float fMax = std::max(carproperties[pIndex].worst, carproperties[pIndex].optimum);
		fValue = std::max(fMin, std::min((fValue - fMin), fMax));
		sValue = std::to_wstring(fValue);
	}
	TruncFloatStr(sValue);
	UpdateValue(sValue, carproperties[pIndex].index);

	// Update state label
	HWND hLabel = GetDlgItem(hwnd, ID_PL_STATE + nRow);
	std::wstring labeltxt;
	COLORREF labelclr;
	bool bActionAvailable = GetStateLabelSpecs(&carproperties[pIndex], labeltxt, labelclr);
	SendMessage(hLabel, WM_SETTEXT, 0, (LPARAM)labeltxt.c_str());
	PLID *plID = (PLID *)GetWindowLong((HWND)hLabel, GWL_USERDATA);
	if (plID != nullptr)
		plID->cColor = labelclr;
	ClearStatic(hLabel, hwnd);

	HWND hFixButton = GetDlgItem(hwnd, ID_PL_BUTTON + nRow);
	if (bActionAvailable)
	{
		if (hFixButton != NULL)
			ShowWindow(hFixButton, SW_SHOW);
		else
		{
			// Create fix button
			RECT StaticRekt;
			GetWindowRect(GetDlgItem(hwnd, ID_PL_EDIT + nRow), &StaticRekt);
			MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&StaticRekt, 2);

			StaticRekt.top -= 2;
			StaticRekt.left -= 2;
			StaticRekt.bottom -= 2;
			StaticRekt.right -= 2;

			StaticRekt.bottom -= StaticRekt.top;
			StaticRekt.right -= StaticRekt.left;

			hFixButton = CreateWindowEx(0, WC_BUTTON, _T("fix"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, StaticRekt.left + StaticRekt.right + 16, StaticRekt.top, 20, StaticRekt.bottom, hwnd, (HMENU)(ID_PL_BUTTON + nRow), hInst, NULL);
			SendMessage(hFixButton, WM_SETFONT, (WPARAM)hFont, TRUE);
		}
	}
	else if (hFixButton != NULL)
		DestroyWindow(hFixButton);

	// Update edit static
	SendMessage(GetDlgItem(hwnd, ID_PL_EDIT + nRow), WM_SETTEXT, 0, (LPARAM)sValue.c_str());
	ClearStatic(GetDlgItem(hwnd, ID_PL_EDIT + nRow), hwnd);
}

// ======================
// dlg procs
// ======================

BOOL CALLBACK AboutProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwnd, IDC_VERSION), WM_SETTEXT, 0, (LPARAM)Title.c_str());
		break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;

		case IDC_FEEDBACK:
			ShellExecute(NULL, _T("open"), _T("http://steamcommunity.com/app/516750/discussions/2/224446614463764765/"), NULL, NULL, SW_SHOWDEFAULT);
			break;
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK HelpProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			if (bFirstStartup)
				SendMessage(GetDlgItem(hwnd, IDC_HELPNEW), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[42].c_str());
			
			PostMessage(hwnd, WM_DRAWINFOICON, 0, 0);
			MessageBeep(MB_ICONASTERISK);
			bFirstStartup = FALSE;
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					break;

				case IDC_FEEDBACK:
				{
					ShellExecute(NULL, _T("open"), _T("http://steamcommunity.com/sharedfiles/filedetails/?id=912990655"), NULL, NULL, SW_SHOWDEFAULT);
					break;
				}
			}
		}
		case WM_DRAWINFOICON:
		{
			HDC hdc = GetDC(hwnd);

			HICON hIcon = LoadIcon(NULL, IDI_INFORMATION);
			if (hIcon != NULL)
			{
				DrawIconEx(hdc, 105, 10, hIcon, 32, 32, 0, NULL, DI_NORMAL);
			}
			break;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

BOOL ReportBoltsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		LVCOLUMN lvc;
		HWND hList3 = GetDlgItem(hwnd, IDC_BLIST);
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 4; lvc.pszText = _T("Stuck"); lvc.cx = 75; lvc.fmt = LVCFMT_LEFT;
		SendMessage(hList3, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 3; lvc.pszText = _T("Installed"); lvc.cx = 70;
		SendMessage(hList3, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 2; lvc.pszText = _T("Damaged"); lvc.cx = 70;
		SendMessage(hList3, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 1; lvc.pszText = _T("Bolts"); lvc.cx = 70;
		SendMessage(hList3, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		lvc.iSubItem = 0; lvc.pszText = _T("Carpart"); lvc.cx = 120;
		SendMessage(hList3, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		UINT item = 0;
		Overview ov;

		SendMessage(hList3, WM_SETREDRAW, 0, 0);

		if (!carparts.empty())
		{
			for (UINT i = 0; i < carparts.size(); i++)
			{
				PopulateBList(hwnd, &carparts[i], item, &ov);
			}
		}
		else
		{
			for (UINT i = 0; i < variables.size(); i++)
			{
				std::wstring prefix;
				if (ContainsStr(variables[i].key, partIdentifiers[3])) // Contains installed?
				{
					prefix = variables[i].key;
					prefix.resize(prefix.size() - partIdentifiers[3].size());
				}
				else if (ContainsStr(variables[i].key, partIdentifiers[1])) // Contains bolts?
				{
					bool valid = TRUE;
					prefix = variables[i].key;
					prefix.resize(prefix.size() - partIdentifiers[1].size());

					std::wstring _strInstalled = prefix + partIdentifiers[3];	
					for (UINT j = 0; j < variables.size(); j++)
					{
						if (variables[j].key == _strInstalled)
						{
							valid = FALSE;
							break;
						}
					}

					if (!valid)
					{
						prefix.clear();
					}
				}

				if (!prefix.empty())
				{

					//filter out special cases
					bool valid = TRUE;
					if (!partSCs.empty())
					{
						for (UINT j = 0; j < partSCs.size(); j++)
						{
							if (partSCs[j].id == 2)
							{
								if (partSCs[j].str == prefix)
								{
									valid = FALSE;
									break;
								}
							}
						}
					}

					if (valid)
					{
						CarPart part;

						std::wstring _str1 = prefix + partIdentifiers[3];
						std::wstring _str2 = prefix + partIdentifiers[1];
						std::wstring _str3 = prefix + partIdentifiers[4];
						std::wstring _str4 = prefix + partIdentifiers[2];
						std::wstring _str5 = prefix + partIdentifiers[0];
						std::wstring _str6 = prefix + partIdentifiers[5];

						for (UINT j = 0; j < variables.size(); j++)
						{
							if (variables[j].key == _str1)
								part.iInstalled = j;

							else if (variables[j].key == _str2)
								part.iBolts = j;

							else if (variables[j].key == _str3)
								part.iTightness = j;

							else if (variables[j].key == _str4)
								part.iDamaged = j;

							else if (variables[j].key == _str5)
								part.iBolted = j;

							else if (variables[j].key == _str6)
								part.iCorner = j;
						}
						part.name = prefix;

						carparts.push_back(part);

						PopulateBList(hwnd, &part, item, &ov);
					}
				}
			}
		}
		// Draw list
		SendMessage(hList3, WM_SETREDRAW, 1, 0);

		// Draw overview
		UpdateBOverview(hwnd, &ov);

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_DISCARD:
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;

		case IDC_BBUT1:
			// Fix loose bolts
			if (!carparts.empty())
			{
				BatchProcessBolts(TRUE);
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT2:
			// Loosen all bolts
			if (!carparts.empty())
			{
				BatchProcessBolts(FALSE);
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT6:
			// Repair parts
			if (!carparts.empty())
			{
				BatchProcessDamage(FALSE);
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT4:
			// Repair all parts
			if (!carparts.empty())
			{
				BatchProcessDamage(TRUE);
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT3:
			// Fix stuck parts
			if (!carparts.empty())
			{
				BatchProcessStuck();
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT5:
			// Uninstall all
			if (!carparts.empty())
			{
				BatchProcessUninstall();
				UpdateBDialog(hwnd);
			}
			break;
		}
		break;
	}
	case WM_NOTIFY:
		if ((((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwnd, IDC_BLIST)) && (((LPNMHDR)lParam)->code == LVN_COLUMNCLICK))
		{
			OnSortHeader((LPNMLISTVIEW)lParam);
			return TRUE;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

/*
LRESULT CALLBACK PropertyListEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE;

	else if (message == WM_DESTROY)
	{
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
		plID = nullptr;
		return TRUE;
	}
	return CallWindowProc(plID->hDefProc, hwnd, message, wParam, lParam);
}
*/

// Some controls inside the property list allocated some data on the heap, so we need to clean it up upon destroying
LRESULT CALLBACK PropertyListControlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE; 

	else if (message == WM_DESTROY)
	{
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
		plID = nullptr;
		return TRUE;
	}
	return CallWindowProc(plID->hDefProc, hwnd, message, wParam, lParam);
}

BOOL CALLBACK PropertyListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static int s_scrollYdelta = 0;
	static HBRUSH s_backgroundBrush;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		s_backgroundBrush = CreateSolidBrush((COLORREF)GetSysColor(COLOR_WINDOW));
		ShowScrollBar(hwnd, SB_VERT, FALSE);
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hwnd, &ps);

		POINT origin;
		GetWindowOrgEx(hdc, &origin);
		SetWindowOrgEx(hdc, origin.x, origin.y + s_scrollYdelta, 0);

		// Move the paint rectangle into the new coordinate system
		OffsetRect(&ps.rcPaint, 0, s_scrollYdelta);

		// Restore coordinates
		SetWindowOrgEx(hdc, origin.x, origin.y, 0);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_COMMAND:
	{
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
		{
			DWORD CtrlID = GetDlgCtrlID((HWND)lParam);
			// if it's ID of a property button (fix)
			if (CtrlID >= ID_PL_BUTTON && CtrlID < (ID_PL_BUTTON + ID_PL_BASE_OFFSET))
			{
				// To get the index, we take it from the edit static because it's always present
				HWND hEditValue = GetDlgItem(hwnd, ID_PL_EDIT + (CtrlID - ID_PL_BUTTON));
				PLID *plID = (PLID *)GetWindowLong(hEditValue, GWL_USERDATA);
				if (plID == nullptr)
					break;

				UpdateRow(hwnd, plID->nPID, CtrlID - ID_PL_BUTTON);
			}
			else if (CtrlID == IDC_PLSET)
			{
				PLID *plID = (PLID *)GetWindowLong(GetDlgItem(hwnd, IDC_PLEDIT), GWL_USERDATA);
				if (plID == nullptr)
					break;
				int nRow = plID->nPID;

				// plID->nPID is the row in this case, not the pIndex
				plID = (PLID *)GetWindowLong(GetDlgItem(hwnd, ID_PL_EDIT + nRow), GWL_USERDATA);
				if (plID == nullptr)
					break;

				std::wstring lvalue = GetEditboxString(GetDlgItem(hwnd, IDC_PLEDIT));

				// Now plID->nPID contains the index, so we can update the row
				UpdateRow(hwnd, plID->nPID, nRow, lvalue);

				DestroyWindow(GetDlgItem(hwnd, IDC_PLEDIT));
				DestroyWindow(GetDlgItem(hwnd, IDC_PLSET));
			}
			break;
		}
		}
		break;
	}
	case WM_VSCROLL:
	{
		SCROLLINFO si = {};
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		GetScrollInfo(hwnd, SB_VERT, &si);

		int scrollY = GetScrollbarPos(hwnd, SB_VERT, LOWORD(wParam));
		if (scrollY == -1)
			break;

		s_scrollYdelta = si.nPos - scrollY;

		//std::wcout << L"Pos: " << si.nPos << L" Trackpos: " << si.nTrackPos << L" ScrollY: " << scrollY << L" Max: " << si.nMax << L" Page: " << si.nPage << std::endl;

		ScrollWindowEx(hwnd, 0, s_scrollYdelta, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);
		SetScrollPos(hwnd, SB_VERT, scrollY, TRUE);

		UpdateWindow(hwnd);
		break;
	}
	case WM_CTLCOLORDLG:
	{
		// Paint background white
		return (INT_PTR)(s_backgroundBrush);
	}
	case WM_CTLCOLORSTATIC:
	{
		DWORD CtrlID = GetDlgCtrlID((HWND)lParam);

		
		// if it's ID of a property state label static
		if (CtrlID >= ID_PL_STATE && CtrlID < (ID_PL_STATE + ID_PL_BASE_OFFSET))
		{
			HDC hdcStatic = (HDC)wParam;

			PLID *plID = (PLID *)GetWindowLong((HWND)lParam, GWL_USERDATA);
			if (plID != nullptr)
			{
				SetTextColor(hdcStatic, RGB(255, 255, 255));
				//SetBkColor(hdcStatic, (COLORREF)GetSysColor(COLOR_WINDOW));
				SetBkColor(hdcStatic, plID->cColor);
				return (INT_PTR)(s_backgroundBrush);		// probably obsolete
			}
		}
		// Always return true to avoid gray background
		return TRUE;
	}
	case WM_UPDATESIZE:
	{
		RECT ScrollRekt;
		GetWindowRect(hwnd, &ScrollRekt);
		ScrollRekt.bottom -= ScrollRekt.top;

		int windowsize = wParam - ScrollRekt.bottom;

		if (windowsize <= 0)
			ShowScrollBar(hwnd, SB_VERT, FALSE);
		else
		{
			int pagesize = static_cast<int>(pow(ScrollRekt.bottom, 2)/ wParam);
			SCROLLINFO si = {};
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMax = windowsize + pagesize;
			si.nPage = pagesize;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		POINT clkloc;
		clkloc.x = LOWORD(lParam);
		clkloc.y = HIWORD(lParam);
		HWND hClickedWindow = ChildWindowFromPoint(hwnd, clkloc);
		HWND hEditValue = GetDlgItem(hwnd, IDC_PLEDIT);
		if (hEditValue != NULL)
			break;

		int cID = GetDlgCtrlID(hClickedWindow);
		if (cID >= ID_PL_EDIT && cID < (ID_PL_EDIT + ID_PL_BASE_OFFSET))
		{
			// First we retrieve the index of the property. We just use the fix button for it as it stores the index anyway
			PLID *plID = (PLID *)GetWindowLong(hClickedWindow, GWL_USERDATA);
			int pIndex = plID->nPID;

			// Then we get the dimensions of the window we just clicked on
			RECT StaticRekt;
			GetWindowRect(hClickedWindow, &StaticRekt);
			MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&StaticRekt, 2);

			StaticRekt.top -= 2;
			StaticRekt.left -= 2;
			StaticRekt.bottom -= 2;
			StaticRekt.right -= 2;

			StaticRekt.bottom -= StaticRekt.top;
			StaticRekt.right -= StaticRekt.left;

			// Fetch the value and create edit-box
			std::wstring vStr;
			FormatString(vStr, variables[carproperties[pIndex].index].value, ID_FLOAT);
			hEditValue = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, vStr.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, StaticRekt.left, StaticRekt.top, StaticRekt.right, StaticRekt.bottom, hwnd, (HMENU)IDC_PLEDIT, hInst, NULL);
			SendMessage(hEditValue, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hEditValue, EM_SETLIMITTEXT, 16, 0);

			// Alloc memory on heap to store edit controls property ID and default window process
			plID = (PLID *)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(PLID));
			plID->hDefProc = (WNDPROC)SetWindowLong(hEditValue, GWL_WNDPROC, (LONG)PropertyListControlProc);
			plID->nPID = (cID - ID_PL_EDIT); // We don't put the index here, because we can just get the index from the row
			SetWindowLong(hEditValue, GWL_USERDATA, (LONG)plID);

			// Create apply button
			HWND hApplyButton = CreateWindowEx(0, WC_BUTTON, _T("set"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, StaticRekt.left + StaticRekt.right + 16, StaticRekt.top, 35, StaticRekt.bottom, hwnd, (HMENU)IDC_PLSET, hInst, NULL);
			SendMessage(hApplyButton, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Hide fix button if it's present
			HWND hFixButton = GetDlgItem(hwnd, ID_PL_BUTTON + (cID - ID_PL_EDIT));
			if (hFixButton != NULL)
				ShowWindow(hFixButton, SW_HIDE);
		}
		break;
	}
	case WM_DESTROY:
	{
		DeleteObject(s_backgroundBrush);
		break;
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL ReportMaintenanceProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		// Create the child list
		int ListWidth = 380;
		HWND hProperties = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROPERTYLIST), hwnd, PropertyListProc);
		ShowWindow(hProperties, SW_SHOW);
		SetWindowPos(hProperties, NULL, 12, 13, ListWidth, 423, SWP_SHOWWINDOW);

		// Create description text
		int sbSize = GetSystemMetrics(SM_CXVSCROLL);
		RECT DlgRekt;
		GetWindowRect(hwnd, &DlgRekt);
		int dlgSize = DlgRekt.right - DlgRekt.left;
		int dtLeft = ListWidth + sbSize + 12;

		HWND hDescriptionText = CreateWindowEx(0, WC_STATIC, GLOB_STRS[45].c_str(), WS_CHILD | WS_VISIBLE, dtLeft, 13, dlgSize - dtLeft - 13, 423, hwnd, NULL, hInst, NULL);
		SendMessage(hDescriptionText, WM_SETFONT, (WPARAM)hFont, TRUE);

		// Don't do anything if we don't have anything loaded
		if (variables.size() == 0)
			break;

		// If we haven't processed the property vector yet, do this here. This only happens once per opened file
		if (!bListProcessed)
		{
			UINT initial_size = carproperties.size();
			for (UINT i = 0; i < variables.size(); i++)
			{
				// Here we set indices of elements read in from the datafile for faster lookup later
				for (UINT j = 0; j < carproperties.size(); j++)
				{
					if (variables[i].key == carproperties[j].lookupname)
					{
						bool bRelevant = carparts.empty();
						if (!bRelevant)
							for (UINT k = 0; k < carparts.size(); k++)
								if (StartsWithStr(carproperties[j].lookupname, carparts[k].name) && carparts[k].iInstalled != UINT_MAX)
									bRelevant = (variables[carparts[k].iInstalled].value.c_str()[0] != NULL);

						if (bRelevant)
							carproperties[j].index = i;
						break;
					}
				}

				// Now we add elements that contain wear in their key to the property-list
				if (ContainsStr(variables[i].key, STR_WEAR) && variables[i].type == ID_FLOAT)
				{
					bool bAlreadyExists = FALSE;
					for (UINT j = 0; j < initial_size; j++)
					{
						if (variables[i].key == carproperties[j].lookupname)
						{
							bAlreadyExists = TRUE;
							break;
						}
					}
					if (!bAlreadyExists)
					{
						// Build display string
						std::wstring displayname = variables[i].key;
						std::size_t pos1 = displayname.find(STR_WEAR);
						if (pos1 != std::wstring::npos)
						{
							displayname.replace(pos1, 4, _T(""));
							displayname += _T(" ");
							displayname += STR_STATE;
							std::transform(displayname.begin(), displayname.begin() + 1, displayname.begin(), ::toupper);
							CarProperty cp = CarProperty(displayname, _T(""), 0.f, 100.f);
							cp.index = i;
							carproperties.push_back(cp);
						}
					}
				}
			}
			bListProcessed = TRUE;
		}


		TEXTMETRIC tm;
		HDC hdc = GetDC(hwnd);
		GetTextMetrics(hdc, &tm);
		//int xChar = tm.tmAveCharWidth;
		//int xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * xChar / 2;
		int yChar = tm.tmHeight + tm.tmExternalLeading;

		int offset = 6;
		int RowID = 0;

		// For every entry in carproperties we add a row
		for (UINT i = 0; i < carproperties.size(); i++)
		{
			// But make sure it has an index set, otherwise it doesn't exist in the variable vector
			if (carproperties[i].index != UINT_MAX)
			{
				// Draw display text
				std::wstring value;
				COLORREF tColor;
				bool bActionAvailable;

				HWND DisplayText = CreateWindowEx(0, WC_STATIC, carproperties[i].displayname.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 6, 0 + offset, 150, yChar + 1, hProperties, (HMENU)ID_PL_DTXT + RowID, hInst, NULL);
				SendMessage(DisplayText, WM_SETFONT, (WPARAM)hFont, TRUE);

				bActionAvailable = GetStateLabelSpecs(&carproperties[i], value, tColor);

				HWND DisplayState = CreateWindowEx(0, WC_STATIC, value.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 162, 0 + offset, 40, yChar + 1, hProperties, (HMENU)(ID_PL_STATE + RowID) , hInst, NULL);
				SendMessage(DisplayState, WM_SETFONT, (WPARAM)hFont, TRUE);

				// Alloc memory on heap to store state's color and default window process
				PLID *plID = (PLID *)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(PLID));
				plID->hDefProc = (WNDPROC)SetWindowLong(DisplayState, GWL_WNDPROC, (LONG)PropertyListControlProc);
				plID->cColor = tColor;
				SetWindowLong(DisplayState, GWL_USERDATA, (LONG)plID);

				FormatString(value, variables[carproperties[i].index].value, ID_FLOAT);
				HWND DisplayValue = CreateWindowEx(0, WC_STATIC, value.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 210, 0 + offset, 100, yChar + 1, hProperties, (HMENU)(ID_PL_EDIT + RowID), hInst, NULL);
				SendMessage(DisplayValue, WM_SETFONT, (WPARAM)hFont, TRUE);

				// Alloc memory on heap to store DisplayValues property ID and default window process (ID needed by fix button)
				plID = (PLID *)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(PLID));
				plID->hDefProc = (WNDPROC)SetWindowLong(DisplayValue, GWL_WNDPROC, (LONG)PropertyListControlProc);
				plID->nPID = i;
				SetWindowLong(DisplayValue, GWL_USERDATA, (LONG)plID);

				if (bActionAvailable)
				{
					HWND FixButton = CreateWindowEx(0, WC_BUTTON, _T("fix"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 324, 0 + offset - 2, 20, yChar + 1, hProperties, (HMENU)(ID_PL_BUTTON + RowID), hInst, NULL);
					SendMessage(FixButton, WM_SETFONT, (WPARAM)hFont, TRUE);
				}
				RowID += 1;
				offset += yChar;
			}

		}
		// Apply size
		PostMessage(hProperties, WM_UPDATESIZE, offset + 3, NULL);

		// Invalidate and redraw to avoid transparency issues
		RECT rekt;
		GetClientRect(hProperties, &rekt);
		InvalidateRect(hProperties, &rekt, TRUE);
		RedrawWindow(hProperties, &rekt, NULL, RDW_ERASE | RDW_INVALIDATE);

		break;
	}

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ReportChildrenProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// Get the index of the selected tab to determine which child proc will be executed
	HWND hwndParent = GetParent(hwnd);
	if (hwndParent == NULL)
		return FALSE;

	DLGHDR *pHdr = (DLGHDR *)GetWindowLong(hwndParent, GWL_USERDATA);
	int iSel = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Positions the child dialog box to occupy the display area of the tab control.
	if (Message == WM_INITDIALOG)
		SetWindowPos(hwnd, NULL, pHdr->rcDisplay.left, pHdr->rcDisplay.top, (pHdr->rcDisplay.right - pHdr->rcDisplay.left), (pHdr->rcDisplay.bottom - pHdr->rcDisplay.top), SWP_SHOWWINDOW);

	return iSel == 0 ? ReportBoltsProc(hwnd, Message, wParam, lParam) : ReportMaintenanceProc(hwnd, Message, wParam, lParam);
}

BOOL CALLBACK TeleportProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		for (UINT i = 0; i < locations.size(); i++)
		{
			SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_ADDSTRING, 0, (LPARAM)locations[i].badstring.c_str());
		}

		for (UINT i = 0; i < variables.size(); i++)
		{
			if (variables[i].type == ID_TRANSFORM)
			{
				SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (UINT)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
				SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
			}
		}

		HWND hOffset = GetDlgItem(hwnd, IDC_OFFSET);
		SendMessage(hOffset, EM_SETLIMITTEXT, 12, 0);
		SendMessage(hOffset, WM_SETTEXT, 0, (LPARAM)_T("0"));
		
		CheckDlgButton(hwnd, IDC_TELEC, BST_CHECKED);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_APPLY:
		{
			int oSize = GetWindowTextLength(GetDlgItem(hwnd, IDC_OFFSET)) + 1;
			std::wstring offsetStr(oSize - 1, '\0');
			GetWindowText(GetDlgItem(hwnd, IDC_OFFSET), (LPWSTR)offsetStr.data(), oSize);
			if (!IsValidFloatStr(offsetStr))
				offsetStr = L"0";
			else
				TruncFloatStr(offsetStr);
			float offset = static_cast<float>(::strtod((WStringToString(offsetStr)).c_str(), NULL));

			UINT index = SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			UINT index_from = 0;
			std::string bin;
			for (UINT i = 0; i < variables.size(); i++)
			{
				if (variables[i].type == ID_TRANSFORM)
				{
					if (index_from == index)
					{
						index_from = i;
						break;
					}

					index_from++;
				}
			}
			index = SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			UINT index_to = 0;

			if (index < locations.size())
			{
				bin = WStringToString(locations[index].newstring) + variables[index_from].value.substr(40);
				if (IsDlgButtonChecked(hwnd, IDC_TELEC) == BST_CHECKED)
					bin = bin.substr(0, 12) + variables[index_from].value.substr(12);
			}
			else
			{
				for (UINT i = 0; i < variables.size(); i++)
				{
					if (variables[i].type == ID_TRANSFORM)
					{
						if (index_to == index - locations.size())
						{
							index_to = i;
							break;
						}
						index_to++;
					}
				}
				bin = variables[index_from].value;
				int len = IsDlgButtonChecked(hwnd, IDC_TELEC) == BST_CHECKED ? 12 : 40;
				bin.replace(0, len, variables[index_to].value.substr(0, len));
			}
			if (offset != 0)
			{
				float height = BinToFloat((bin.substr(4, 4))) + offset;
				bin.replace(4, 4, FloatStrToBin(std::to_string(height)));
			}
			
			UpdateValue(_T(""), index_from, bin);

			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}

		case IDC_DISCARD:
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);

			break;

		case IDC_LOCFROM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1) ::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			}
			break;

		case IDC_LOCTO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1) ::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			}
			break;
		}
	}

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK TransformProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		hTransform = hwnd;

		UINT type = variables[indextable[iItem].index2].type;
		if (type != ID_TRANSFORM)
		{
			//something went seriously wrong here
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			EnableWindow(hDialog, TRUE);
			break;
		}

		std::string value = variables[indextable[iItem].index2].value;

		//location 
		std::string str;
		std::string tstr;
		for (UINT i = 0; i < 3; i++)
		{
			tstr = BinToFloatStr(value.substr((4 * i), 4));
			TruncFloatStr(tstr);
			str += (tstr + (", "));
		}
		str.resize(str.size() - 2);
		SendMessage((GetDlgItem(hTransform, IDC_LOC)), WM_SETTEXT, 0, (LPARAM)StringToWString(str).c_str());

		//rotation
		str.clear();
		tstr.clear();

		if (!bEulerAngles)
		{
			for (UINT i = 0; i < 4; i++)
			{
				tstr = BinToFloatStr(value.substr(12 + (4 * i), 4));
				TruncFloatStr(tstr);
				str += (tstr + (", "));
			}
			SendMessage((GetDlgItem(hTransform, IDC_ROTBOX)), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[27].c_str());
		}
		else
		{
			QTRN q;
			ANGLES a;
			q.x = BinToFloat(value.substr(12 + 0, 4));
			q.y = BinToFloat(value.substr(12 + 4, 4));
			q.z = BinToFloat(value.substr(12 + 8, 4));
			q.w = BinToFloat(value.substr(12 + 12, 4));
			a = QuatToEuler(&q);
			double list[] = { a.yaw, a.pitch, a.roll };
			for (UINT i = 0; i < 3; i++)
			{
				tstr = std::to_string(list[i]);
				TruncFloatStr(tstr);
				str += (tstr + (", "));
			}
			SendMessage((GetDlgItem(hTransform, IDC_ROTBOX)), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[28].c_str());
		}

		str.resize(str.size() - 2);
		SendMessage((GetDlgItem(hTransform, IDC_ROT)), WM_SETTEXT, 0, (LPARAM)StringToWString(str).c_str());

		//scale
		str.clear();
		tstr.clear();
		for (UINT i = 0; i < 3; i++)
		{
			tstr = BinToFloatStr(value.substr(28 + (4 * i), 4));
			TruncFloatStr(tstr);
			str += (tstr + (", "));
		}
		str.resize(str.size() - 2);
		SendMessage((GetDlgItem(hTransform, IDC_SCA)), WM_SETTEXT, 0, (LPARAM)StringToWString(str).c_str());
		//MSC dev requested removal of scale property
		if (!bAllowScale)
			::EnableWindow(GetDlgItem(hTransform, IDC_SCA), FALSE);

		//tag
		if (value.size() > 40)
		{
			str.clear();
			str = std::move(value.substr(41));
			SendMessage((GetDlgItem(hTransform, IDC_TAG)), WM_SETTEXT, 0, (LPARAM)StringToWString(str).c_str());
			SendMessage((GetDlgItem(hTransform, IDC_TAG)), EM_SETLIMITTEXT, 255, 0);
		}

		break;

	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_APPLY:
			{
				UINT error;
				bool valid = TRUE;
				std::wstring value;
				std::string bin;

				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = _T("Error!");
				ebt.ttiIcon = TTI_ERROR_LARGE;

				//location
				std::wstring lvalue = GetEditboxString(GetDlgItem(hTransform, IDC_LOC));

				error = VectorStrToBin(lvalue, 3, bin);
				if (error > 0)
				{
					valid = FALSE;
					ebt.pszText = GLOB_STRS[error].c_str();
					SendMessage(GetDlgItem(hTransform, IDC_LOC), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}

				//rotation
				value = GetEditboxString(GetDlgItem(hTransform, IDC_ROT));

				if (!bEulerAngles)
				{
					error = VectorStrToBin(value, 4, bin, TRUE, TRUE);
					if (error > 0)
					{
						valid = FALSE;
						ebt.pszText = GLOB_STRS[error].c_str();
						SendMessage(GetDlgItem(hTransform, IDC_ROT), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}
				else
				{
					std::string oldvar = variables[indextable[iItem].index2].value;
					QTRN qold;
					qold.x = BinToFloat(oldvar.substr(12 + 0, 4));
					qold.y = BinToFloat(oldvar.substr(12 + 4, 4));
					qold.z = BinToFloat(oldvar.substr(12 + 8, 4));
					qold.w = BinToFloat(oldvar.substr(12 + 12, 4));

					error = VectorStrToBin(value, 3, bin, TRUE, FALSE, TRUE, &qold, oldvar.substr(12, 16));
					if (error > 0)
					{
						valid = FALSE;
						ebt.pszText = GLOB_STRS[error].c_str();
						SendMessage(GetDlgItem(hTransform, IDC_ROT), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}

				//scale
				value = GetEditboxString(GetDlgItem(hTransform, IDC_SCA));

				if (bAllowScale)
				{
					error = VectorStrToBin(value, 3, bin);
					if (error > 0)
					{
						valid = FALSE;
						ebt.pszText = GLOB_STRS[error].c_str();
						SendMessage(GetDlgItem(hTransform, IDC_SCA), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}
				else
					bin += variables[indextable[iItem].index2].value.substr(28, 12);

				if (valid)
				{
					value = GetEditboxString(GetDlgItem(hTransform, IDC_TAG));

					bin += char(value.size()) + WStringToString(value);

					UpdateValue(lvalue, indextable[iItem].index2, bin);

					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
				}
				break;
			}
			case IDC_DISCARD:
			{
				EndDialog(hwnd, 0);
				DestroyWindow(hwnd);
				EnableWindow(hDialog, TRUE);
				break;
			}
			}
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ColorProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static bool bIsColorProc = TRUE;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		hColor = hwnd;
		bIsColorProc = (lParam == 1);

		UINT type = variables[indextable[iItem].index2].type;
		if (type != (bIsColorProc ? ID_COLOR : ID_VECTOR))
		{
			//something seriously went wrong here
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			EnableWindow(hDialog, TRUE);
			break;
		}

		std::wstring str;
		FormatString(str, variables[indextable[iItem].index2].value, bIsColorProc ? ID_COLOR : ID_VECTOR);

		SendMessage((GetDlgItem(hColor, IDC_COL)), WM_SETTEXT, 0, (LPARAM)str.c_str());
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_APPLY:
			{
				UINT error, size;
				std::wstring value;
				std::string bin;
				HWND hEdit;
				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = _T("Error!");
				ebt.ttiIcon = TTI_ERROR_LARGE;

				hEdit = GetDlgItem(hColor, IDC_COL);
				size = GetWindowTextLength(hEdit) + 1;
				TCHAR *LocEditText = new TCHAR[size];
				memset(LocEditText, 0, size);
				GetWindowText(hEdit, (LPWSTR)LocEditText, size);
				value = LocEditText;
				delete[] LocEditText;

				error = VectorStrToBin(value, bIsColorProc ? 4 : 3, bin, bIsColorProc ? FALSE : TRUE, bIsColorProc ? TRUE : FALSE);
				if (error > 0)
				{
					ebt.pszText = GLOB_STRS[error].c_str();
					SendMessage(GetDlgItem(hColor, IDC_COL), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}
				else
				{
					UpdateValue(value, indextable[iItem].index2, bin);

					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
				}
				break;
			}
			case IDC_DISCARD:
			{
				EndDialog(hwnd, 0);
				DestroyWindow(hwnd);
				EnableWindow(hDialog, TRUE);
				break;
			}
			}
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK StringProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		std::wstring wstr = StringToWString(variables[indextable[iItem].index2].value);
		std::wstring PrntStr;

		UINT i = 4;
		while (TRUE)
		{
			if (i + 1 > wstr.size()) break;
			UINT len = static_cast<int>(wstr[i]);
			PrntStr += wstr.substr(i + 1, len);
			PrntStr += {char(13), char(10)};
			i += len + 1;
		}

		SetDlgItemText(hwnd, IDC_STRINGEDIT, PrntStr.c_str());
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
				case IDC_APPLY:
				{
					UINT max = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_GETLINECOUNT, 0, 0);

					int len = GetWindowTextLength(GetDlgItem(hwnd, IDC_STRINGEDIT)) + 1;
					std::wstring ctrlText(len, '\0');
					//TCHAR* ctrlText = new TCHAR[len];
					//memset(ctrlText, 0, len);
					GetWindowText(GetDlgItem(hwnd, IDC_STRINGEDIT), (LPTSTR)ctrlText.data(), len);

					UINT lines = 0;
					std::string value;
					for (UINT i = 0; i < max; i++)
					{
						UINT charindex = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_LINEINDEX, i, 0);
						UINT linelen = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_LINELENGTH, charindex, 0);
						if (linelen != 0)
						{
							if (linelen > 255) linelen = 255;
							value += char(linelen);
							value += WStringToString(ctrlText.substr(charindex, linelen));
							lines++;
						}
					}
					value.insert(0, IntStrToBin(std::to_string(lines)));
					UpdateValue(_T(""), indextable[iItem].index2, value);

					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
					break;
				}
				case IDC_DISCARD:
				{
					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
					break;
				}
			}
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK SpawnItemProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (itemTypes.empty() || itemAttributes.empty())
			{
				MessageBox(hDialog, GLOB_STRS[26].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
				EndDialog(hwnd, 0);
				DestroyWindow(hwnd);

				break;
			}

			for (UINT i = 0; i < itemTypes.size(); i++)
			{
				if (EntryExists(itemTypes[i].GetID()) >= 0)
					SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (UINT)CB_ADDSTRING, 0, (LPARAM)StringToWString(itemTypes[i].GetDisplayName()).c_str());
			}

			for (UINT i = 0; i < locations.size(); i++)
			{
				SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_ADDSTRING, 0, (LPARAM)locations[i].badstring.c_str());
			}

			for (UINT i = 0; i < variables.size(); i++)
			{
				if (variables[i].type == ID_TRANSFORM)
					SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
			}

			HWND hOffset = GetDlgItem(hwnd, IDC_AMOUNT);
			SendMessage(hOffset, EM_SETLIMITTEXT, 12, 0);
			SendMessage(hOffset, WM_SETTEXT, 0, (LPARAM)_T("1"));
			break;
		}

		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
				case IDC_APPLY:
				{
					UINT index_to = 0;
					UINT index_what = 0;
					UINT index = 0;  
					std::string location;

					index = SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

					for (UINT i = 0; i < itemTypes.size(); i++)
					{
						if (EntryExists(itemTypes[i].GetID()) >= 0)
						{
							if (index_what == index)
								break;
							index_what++;
						}
					}

					index = SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

					if (index < locations.size())
						location = WStringToString(locations[index].newstring);
					else
					{
						for (UINT i = 0; i < variables.size(); i++)
						{
							if (variables[i].type == ID_TRANSFORM)
							{
								if (index_to == index - locations.size())
								{
									index_to = i;
									break;
								}
								index_to++;
							}
						}
						location = variables[index_to].value;
					}
					location += itemTypes[index_what].GetLayer();

					int oSize = GetWindowTextLength(GetDlgItem(hwnd, IDC_AMOUNT)) + 1;
					std::wstring amountStr(oSize - 1, '\0');
					GetWindowText(GetDlgItem(hwnd, IDC_AMOUNT), (LPWSTR)amountStr.data(), oSize);
					int amount = static_cast<int>(::strtol((WStringToString(amountStr)).c_str(), NULL, 10));
					if (amount < 1)
						amount = 1;

					index = 0;
					for (UINT i = 0; i < (UINT)amount; i++)
					{
						index = EntryExists(itemTypes[index_what].GetID());
						std::wstring idStr;
						FormatString(idStr, variables[index].value, ID_INT);
						int a = static_cast<int>(::strtol(WStringToString(idStr).c_str(), NULL, 10));
						a++;
						UpdateValue(std::to_wstring(a), index);
						std::wstring keyprefix = StringToWString(itemTypes[index_what].GetName()) + std::to_wstring(a);

						Variables_add(Variable(location, UINT_MAX, ID_TRANSFORM, keyprefix + StringToWString(itemAttributes[0].GetName())));

						std::vector<char> attributes = itemTypes[index_what].GetAttributes(itemAttributes.size());

						for (UINT j = 0; j < attributes.size(); j++)
						{
							std::string bin;
							UINT type = itemAttributes[(UINT)attributes[j]].GetType();
							std::string max = std::to_string(itemAttributes[(UINT)attributes[j]].GetMax());
							TruncFloatStr(max);
							FormatValue(bin, max, type);
							
							Variables_add(Variable(bin, UINT_MAX, type, keyprefix + StringToWString(itemAttributes[(UINT)attributes[j]].GetName())));
						}

					}

					UINT size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
					std::wstring str(size, '\0');
					GetWindowText(GetDlgItem(hwnd, IDC_FILTER), (LPWSTR)str.data(), size);
					str.resize(size - 1);

					UpdateList(str);

					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
					break;
				}

				case IDC_DISCARD:
				{
					EndDialog(hwnd, 0);
					DestroyWindow(hwnd);
					EnableWindow(hDialog, TRUE);
					break;
				}
				}
			}

			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				switch (LOWORD(wParam))
				{
					case IDC_SPAWNWHAT:
						if (SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
							::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
						break;
					case IDC_LOCTO:
						if (SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
							::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
						break;
				}
			}
		}

		default:
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK CleanItemProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			for (UINT i = 0; i < itemTypes.size(); i++)
			{
				if (EntryExists(itemTypes[i].GetID()) >= 0)
					SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (UINT)CB_ADDSTRING, 0, (LPARAM)StringToWString(itemTypes[i].GetName()).c_str());
			}
		}
		case WM_COMMAND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				UINT index_what = 0, index = SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				for (UINT i = 0; i < itemTypes.size(); i++)
				{
					if (EntryExists(itemTypes[i].GetID()) >= 0)
					{
						if (index_what == index)
							break;
						index_what++;
					}
				}
				for (UINT i = 0; i < variables.size(); i++)
				{
					if (StartsWithStr(variables[i].key, StringToWString(itemTypes[index_what].GetName())))
					{
						UINT id = ParseItemID(variables[i].key, itemTypes[index_what].GetName().size() + 1);
						if (id != 0)
						{
							std::wstring idStr = std::to_wstring(id);

							std::vector<char> attributes = itemTypes[index_what].GetAttributes(itemAttributes.size());

							for (UINT j = 0; j < attributes.size(); j++)
							{
								std::string bin;
								//UINT type = itemAttributes[(UINT)attributes[j]].GetName();

							//if (variables[i].key.substr(itemTypes[index_what].GetName().size() + idStr.size()) == )
							}
						}
					}
				}
				break;
			}
		default:
			return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_KILLFOCUS:
		{
			return CloseEditbox(hwnd, GetDlgItem(hDialog, IDC_OUTPUTBAR2));
		}
		case WM_GETDLGCODE:
			switch (wParam)
			{
				case VK_ESCAPE: 
				case VK_RETURN: 
					return CloseEditbox(hwnd, GetDlgItem(hDialog, IDC_OUTPUTBAR2));
			}
	}

	return CallWindowProc(DefaultEditProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ComboProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_KILLFOCUS:
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			hCEdit = NULL;
			return 0;
		}
	}
	return CallWindowProc(DefaultComboProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
			if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
		}
	}
	return CallWindowProc(DefaultListViewProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ListCtrlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_RBUTTONDOWN:
	{
		if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
		if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
		LVHITTESTINFO itemclicked;
		long x, y;
		x = (long)LOWORD(lParam);
		y = (long)HIWORD(lParam);
		itemclicked.pt.x = x;
		itemclicked.pt.y = y;
		int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		if (lResult != -1)
		{
			RECT rekt;
			iItem = itemclicked.iItem;
			GetWindowRect(hwnd, &rekt);
			rekt.top += HIWORD(lParam);
			rekt.left += LOWORD(lParam);

			PostMessage(hwnd, WM_L2CONTEXTMENU, rekt.top, rekt.left);

		}
		break;
	}

	case WM_LBUTTONDOWN:
	{
		if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); return 0; };
		if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); return 0; };
		if (IsWindow(hTransform)) return 0;

		LVHITTESTINFO itemclicked;
		long x, y;
		x = (long)LOWORD(lParam);
		y = (long)HIWORD(lParam);
		itemclicked.pt.x = x;
		itemclicked.pt.y = y;
		int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		if (lResult != -1)
		{
			if (itemclicked.iSubItem == 1) { break; }

			RECT rekt;
			std::string value = variables[indextable[itemclicked.iItem].index2].value;
			ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_BOUNDS, &rekt);
			int height = rekt.bottom - rekt.top;
			int width = rekt.right - rekt.left;

			int type = variables[indextable[itemclicked.iItem].index2].type;
			ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT2), hDialog);
			SendMessage(GetDlgItem(hDialog, IDC_OUTPUT2), WM_SETTEXT, 0, (LPARAM)TypeStrs[type + 1].c_str());

			switch (type)
			{
			case ID_STRING:
			case ID_INT:
			case ID_FLOAT:
			{
				iItem = itemclicked.iItem;
				iSubItem = itemclicked.iSubItem;
				if (iSubItem == 0) { width = width / 4; };
				hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), (LPWSTR)_T(""),
					WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
					rekt.left, rekt.top, width, height, hwnd, 0, hInst, NULL);

				SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

				int size;
				(type == ID_STRING) ? size = 128 : size = 12;
				SendMessage(hEdit, EM_SETLIMITTEXT, size, 0);
				TCHAR *buffer = new TCHAR[size + 1];
				memset(buffer, 0, size + 1);
				ListView_GetItemText(hwnd, iItem, 0, buffer, size);
				SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer);
				delete[] buffer;

				SetFocus(hEdit);
				SendMessage(hEdit, EM_SETSEL, 0, -1);
				SendMessage(hEdit, EM_SETSEL, -1, 0);

				DefaultEditProc = (WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC, (LONG)EditProc);
				break;
			}
			case ID_BOOL:
			{
				iItem = itemclicked.iItem;
				iSubItem = itemclicked.iSubItem;
				int boolindex;
				value.c_str()[0] == 0x00 ? boolindex = 0 : boolindex = 1;
				hCEdit = CreateWindow(WC_COMBOBOX, (LPWSTR)_T(""),
					CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
					rekt.left, rekt.top, 100, height, hwnd, NULL, GetModuleHandle(NULL), NULL);

				SendMessage(hCEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
				SendMessage(hCEdit, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)bools[boolindex].c_str());
				SendMessage(hCEdit, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(boolindex == 0 ? bools[1].c_str() : bools[0].c_str()));
				SendMessage(hCEdit, CB_SETCURSEL, 0, 0);
				DefaultComboProc = (WNDPROC)SetWindowLong(hCEdit, GWL_WNDPROC, (LONG)ComboProc);
				break;
			}
			case ID_TRANSFORM:
			{
				if (!IsWindow(hTransform))
				{
					iItem = itemclicked.iItem;
					iSubItem = itemclicked.iSubItem;
					EnableWindow(hDialog, FALSE);
					hTransform = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TRANSFORM), hDialog, TransformProc);
					ShowWindow(hTransform, SW_SHOW);
				}
				break;
			}
			case ID_COLOR:
			{
				if (!IsWindow(hColor))
				{
					iItem = itemclicked.iItem;
					iSubItem = itemclicked.iSubItem;
					EnableWindow(hDialog, FALSE);
					hColor = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_COLOR), hDialog, ColorProc, TRUE);
					//hColor = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLOR), hDialog, ColorProc);
					ShowWindow(hColor, SW_SHOW);
				}
				break;
			}
			case ID_VECTOR:
			{
				if (!IsWindow(hColor))
				{
					iItem = itemclicked.iItem;
					iSubItem = itemclicked.iSubItem;
					EnableWindow(hDialog, FALSE);
					hColor = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_VECTOR), hDialog, ColorProc, NULL);
					ShowWindow(hColor, SW_SHOW);
				}
				break;
			}
			case ID_STRINGL:
			{
				if (!IsWindow(hString))
				{

					iItem = itemclicked.iItem;
					iSubItem = itemclicked.iSubItem;
					EnableWindow(hDialog, FALSE);
					hString = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STRING), hDialog, StringProc);
					ShowWindow(hString, SW_SHOW);
				}
				break;
			}
			}
		}
		return 0;
		break;
	}

	case WM_L2CONTEXTMENU:
	{
		HMENU hPopupMenu = CreatePopupMenu();
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING | ((!variables[indextable[iItem].index2].IsModified() && !variables[indextable[iItem].index2].IsRemoved()) ? MF_DISABLED : 0), ID_RESET, _T("Reset"));
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING | (variables[indextable[iItem].index2].IsRemoved() ? MF_DISABLED : 0), ID_DELETE, _T("Delete"));
		//SetForegroundWindow(hwnd);
		BOOL fuck = TrackPopupMenuEx(hPopupMenu, TPM_LEFTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_VERPOSANIMATION, lParam, wParam, hwnd, NULL);
		DestroyMenu(hPopupMenu);
		break;
	}

	case WM_COMMAND:
	{
		// handle hcedits messages in here because I'm messy like that 
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			int oSize = SendMessage((HWND)lParam, (UINT)CB_GETLBTEXTLEN, (WPARAM)ItemIndex, 0) + 1;
			std::wstring value(oSize - 1, '\0');
			(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)value.data());

			SendMessage(hCEdit, CB_RESETCONTENT, 0, 0);
			ShowWindow(hCEdit, SW_HIDE);
			UpdateValue(value, indextable[iItem].index2);
			break;
		}
		
		switch (LOWORD(wParam))
		{
			case ID_RESET:
			{
				UINT index = indextable[iItem].index2;

				variables[index].SetRemoved(FALSE);
				UpdateValue(_T(""), index, variables[index].static_value);
				break;
			}
			case ID_DELETE:
			{
				UINT index = indextable[iItem].index2;
				Variables_remove(indextable[iItem].index2);
				break;
			}
		}
	}
	}
	return CallWindowProc(DefaultListCtrlProc, hwnd, message, wParam, lParam);
}

BOOL CALLBACK ReportProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		DWORD dwDlgBase = GetDialogBaseUnits();
		int cxMargin = LOWORD(dwDlgBase) / 4;
		int cyMargin = HIWORD(dwDlgBase) / 8;

		TCITEM tie;
		RECT rcTab;
		HWND hwndButton;
		RECT rcButton;
		RECT rcButtonText;
		int i;

		// Allocate memory for the DLGHDR structure. 
		DLGHDR *pHdr = (DLGHDR *)LocalAlloc(LPTR, sizeof(DLGHDR));

		// Save a pointer to the DLGHDR structure in the window data of the dialog box. 
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)pHdr);

		// Create the tab control.
		pHdr->hwndTab = CreateWindowEx(0, WC_TABCONTROL, 0,
			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
			0, 0, 100, 100,
			hwnd, NULL, hInst, NULL
		);
		SendMessage(pHdr->hwndTab, WM_SETFONT, (WPARAM)hFont, TRUE);

		if (pHdr->hwndTab == NULL)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		// Add a tab for each of the child dialog boxes. 
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;
		tie.pszText = _T("Bolts and bits");
		TabCtrl_InsertItem(pHdr->hwndTab, 0, &tie);
		tie.pszText = _T("Maintenance");
		TabCtrl_InsertItem(pHdr->hwndTab, 1, &tie);

		// Lock the resources for the child dialog boxes. 
		pHdr->apRes[0] = DoLockDlgRes(MAKEINTRESOURCE(IDD_BOLTS));
		pHdr->apRes[1] = DoLockDlgRes(MAKEINTRESOURCE(IDD_MAINTENANCE));

		// Determine a bounding rectangle that is large enough to contain the largest child dialog box. 
		SetRectEmpty(&rcTab);
		for (i = 0; i < 2; i++)
		{
			if (pHdr->apRes[i]->cx > rcTab.right)
				rcTab.right = pHdr->apRes[i]->cx;
			if (pHdr->apRes[i]->cy > rcTab.bottom)
				rcTab.bottom = pHdr->apRes[i]->cy;
		}

		// Map the rectangle from dialog box units to pixels.
		MapDialogRect(hwnd, &rcTab);

		// Calculate how large to make the tab control, so the display area can accommodate all the child dialog boxes. 
		TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);
		OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

		// Calculate the display rectangle. 
		CopyRect(&pHdr->rcDisplay, &rcTab);
		TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

		// Set the size and position of the tab control, buttons, and dialog box. 
		SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top,
			rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
			SWP_NOZORDER);

		// Move the first button below the tab control. 
		hwndButton = GetDlgItem(hwnd, IDC_DISCARD);
		SetWindowPos(hwndButton, NULL,
			rcTab.left, rcTab.bottom + cyMargin, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER);

		// Determine the size of the button. 
		GetWindowRect(hwndButton, &rcButton);
		rcButton.right -= rcButton.left;
		rcButton.bottom -= rcButton.top;

		// Move text right of the button
		HWND hwndButtonText = GetDlgItem(hwnd, IDC_DISCARDTEXT);
		GetWindowRect(hwndButtonText, &rcButtonText);
		rcButtonText.bottom -= rcButtonText.top;
		SetWindowPos(hwndButtonText, NULL,
			rcTab.left + cyMargin + rcButton.right, rcTab.bottom + cyMargin + (int)((rcButton.bottom - rcButtonText.bottom) / 2), 0, 0,
			SWP_NOSIZE | SWP_NOZORDER);

		// Size the dialog box. 
		SetWindowPos(hwnd, NULL, 0, 0,
			rcTab.right + cyMargin + (2 * GetSystemMetrics(SM_CXDLGFRAME)),
			rcTab.bottom + rcButton.bottom + (2 * cyMargin)
			+ (2 * GetSystemMetrics(SM_CYDLGFRAME))
			+ (int)(1.5 * GetSystemMetrics(SM_CYCAPTION)),
			SWP_NOMOVE | SWP_NOZORDER);

		// Simulate selection of the first item. 
		NMHDR nmh;
		nmh.code = TCN_SELCHANGE;  
		nmh.idFrom = GetDlgCtrlID(pHdr->hwndTab);
		nmh.hwndFrom = hwnd;
		SendMessage(GetParent(pHdr->hwndTab), WM_NOTIFY, nmh.idFrom,(LPARAM)&nmh);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_DISCARD:
		{
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}
		}
		break;
	}
	case WM_NOTIFY:
		if ((((LPNMHDR)lParam)->code == TCN_SELCHANGE))
		{
			// Get the dialog header data.
			DLGHDR *pHdr = (DLGHDR *)GetWindowLong(hwnd, GWL_USERDATA);

			// Get the index of the selected tab.
			int iSel = TabCtrl_GetCurSel(pHdr->hwndTab);

			// Destroy the current child dialog box, if any. 
			if (pHdr->hwndDisplay != NULL)
				DestroyWindow(pHdr->hwndDisplay);

			// Create the new child dialog box.
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE *)pHdr->apRes[iSel], hwnd, ReportChildrenProc);
			return TRUE;
		}
		break;
	case WM_DESTROY:
	{
		// Free memory
		DLGHDR *pHdr = (DLGHDR *)GetWindowLong(hwnd, GWL_USERDATA);
		LocalFree(pHdr);
		return 0;
	}
	default:
		return FALSE;
	}
	return TRUE;
}