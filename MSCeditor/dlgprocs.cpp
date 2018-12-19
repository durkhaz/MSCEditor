#include "dlgprocs.h"
#include <CommCtrl.h>

#define		WM_L2CONTEXTMENU			(WM_USER + 10)
#define		WM_DRAWINFOICON				(WM_USER + 11)
#define		WM_UPDATESIZE				(WM_USER + 12)
#define		WM_CONTAINERUPDATEINDECES	(WM_USER + 13)
#define		WM_DESTROYBUTTONS			(WM_USER + 14)

#define		STR_WEAR			_T("wear")
#define		STR_STATE			_T("condition")
#define		STR_GOOD			_T("Good")
#define		STR_BAD				_T("Bad")
#define		CLR_GOOD			RGB(153, 185, 152)
#define		CLR_OK				RGB(253, 206, 170)
#define		CLR_BAD				RGB(244, 131, 125)

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

void AllocWindowUserData(HWND hwnd, LONG DefProc, uint32_t id = 0, COLORREF clr = RGB(255, 255, 255), uint32_t dat = 0)
{
	PLID *plID = (PLID *)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(PLID));
	plID->hDefProc = (WNDPROC)SetWindowLongPtr(hwnd, GWL_WNDPROC, DefProc);
	plID->cColor = clr;
	plID->nPID = id;
	plID->dat = dat;
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)plID);
}

inline void ShowBalloontip(HWND hwnd, uint32_t msgIndex)
{
	EDITBALLOONTIP ebt;
	ebt.cbStruct = sizeof(EDITBALLOONTIP);
	ebt.pszTitle = ErrorTitle.c_str();
	ebt.ttiIcon = TTI_ERROR_LARGE;
	ebt.pszText = GLOB_STRS[msgIndex].c_str();
	SendMessage(hwnd, EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
}

inline std::wstring GetEditboxString(HWND hEdit)
{
	uint32_t size = GetWindowTextLength(hEdit);
	std::wstring str(size, '\0');
	GetWindowText(hEdit, (LPWSTR)str.data(), size + 1);
	return str;
}

bool CloseEditbox(HWND hwnd, HWND hOutput, bool bIgnoreContent = FALSE)
{
	// Setting this to 1 makes sure we won't execute twice. Losing focus on a window triggers twice sometimes for some godforsaken reason
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID)
		plID->nPID = 1;

	if (!bIgnoreContent)
	{
		bool valid = TRUE;
		std::wstring value = GetEditboxString(hwnd);
		auto type = variables[indextable[iItem].second].header.GetValueType();
		if (type == EntryValue::Float)
		{
			valid = IsValidFloatStr(value);
			if (valid)
				TruncFloatStr(value);
		}
		valid ? UpdateValue(value, indextable[iItem].second) : ShowBalloontip(hOutput, 8);
	}
	DestroyWindow(GetDlgItem(GetParent(hwnd), IDC_BUTNEGINF));
	DestroyWindow(GetDlgItem(GetParent(hwnd), IDC_BUTPOSINF));
	DestroyWindow(hwnd);
	hEdit = NULL;
	return TRUE;
}

// Returns whether actions are required (for the fix button)
bool GetStateLabelSpecs(const CarProperty *cProp, std::wstring &sLabel, COLORREF &tColor)
{
	if (cProp == nullptr)
		return FALSE;

	sLabel = _T(" ");
	tColor = (COLORREF)GetSysColor(COLOR_WINDOW);
	bool bActionAvailable = FALSE;

	switch (variables[cProp->index].header.GetValueType())
	{
	case EntryValue::Vector3:
	{
		bActionAvailable = variables[cProp->index].value != cProp->recommendedBin;
		sLabel += bActionAvailable ? STR_BAD : STR_GOOD;
		tColor = bActionAvailable ? CLR_BAD : CLR_GOOD;
		break;
	}
	case EntryValue::Float:
	{
		if (!cProp->worstBin.empty())
		{
			float fMin = std::min(BinToFloat(cProp->worstBin), BinToFloat(cProp->optimumBin));
			float fMax = std::max(BinToFloat(cProp->worstBin), BinToFloat(cProp->optimumBin));
			float fValue = BinToFloat(variables[cProp->index].value);

			// If recommended is defined (not empty), then we check if it's in range of min max
			if (!cProp->recommendedBin.empty())
			{
				if (fValue > fMax + kindasmall || fValue < fMin - kindasmall)
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
				if (BinToFloat(cProp->worstBin) > BinToFloat(cProp->optimumBin))
					fValue = BinToFloat(cProp->worstBin) - fValue;
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
			float fDelta = std::abs(BinToFloat(cProp->optimumBin) - BinToFloat(variables[cProp->index].value));
			if (fDelta <= kindasmall)
			{
				sLabel += STR_GOOD;
				tColor = CLR_GOOD;
			}
		}
		break;
	}
	}

	sLabel += _T(" ");
	return bActionAvailable;
}

void UpdateRow(HWND hwnd, uint32_t pIndex, int nRow, std::wstring str = _T(""))
{
	std::wstring sValue = str;
	std::string binValue = "";
	auto dataType = variables[carproperties[pIndex].index].header.GetValueType();
	bool bHasRecommended = !carproperties[pIndex].recommendedBin.empty();

	// If the change didn't come from the edit box, but from the fix button, str is empty
	// Order of consideration: recommended > optimum
	if (str.empty())
	{
		binValue = bHasRecommended ? carproperties[pIndex].recommendedBin : carproperties[pIndex].optimumBin;
		sValue = Variable::ValueBinToStr(binValue, dataType);
	}

	// If we don't have a recommended value, and we have a range defined, we clamp
	else if (!bHasRecommended && !carproperties[pIndex].worstBin.empty() && carproperties[pIndex].optimumBin.empty())
	{
		float fValue = static_cast<float>(::strtod(WStringToString(sValue).c_str(), NULL));
		float fMin = std::min(BinToFloat(carproperties[pIndex].worstBin), BinToFloat(carproperties[pIndex].optimumBin));
		float fMax = std::max(BinToFloat(carproperties[pIndex].worstBin), BinToFloat(carproperties[pIndex].optimumBin));
		fValue = std::max(fMin, std::min((fValue - fMin), fMax));
		sValue = std::to_wstring(fValue);
	}
	//TruncFloatStr(sValue);
	UpdateValue(sValue, carproperties[pIndex].index, binValue);

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

	// Check if the change made an actions available
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
			SendMessage(hFixButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
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

BOOL CALLBACK AboutProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK HelpProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK TimeWeatherProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static const std::vector<std::wstring> weathertypes = { L"Clear Sky", L"Overcast", L"Rainy", L"Thunderstorm" };
	static const std::wstring worldtime =		L"worldtime";
	static const std::wstring worldday =		L"worldday";
	static const std::wstring weathercloud =	L"weathercloudid";
	static const std::wstring weatherpos =		L"weatherpos";
	static const std::wstring weathertype =		L"weathertype";
	static const int cloudtypes[] = { 4, 1, 2 };

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		// Timetable
		{
			const HWND hTimetable = GetDlgItem(hwnd, IDC_TTIMETABLE);
			ListView_SetExtendedListViewStyle(hTimetable, LVS_EX_GRIDLINES);

			RECT rekt;
			GetWindowRect(hTimetable, &rekt);
			const int width = rekt.right - rekt.left - 4 - GetSystemMetrics(SM_CXVSCROLL);

			// Column headers
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.iSubItem = 2; lvc.pszText = _T("Event"); lvc.cx = width - 140; lvc.fmt = LVCFMT_LEFT;
			SendMessage(hTimetable, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
			lvc.iSubItem = 1; lvc.pszText = _T("Day"); lvc.cx = 70;
			SendMessage(hTimetable, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
			lvc.iSubItem = 0; lvc.pszText = _T("Time"); lvc.cx = 70;
			SendMessage(hTimetable, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

			// Entries
			int item = 0;
			for (auto& entry : timetableEntries)
			{
				LVITEM lvi;
				lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
				lvi.iItem = item; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)entry.time.c_str(), lvi.lParam = 0;
				SendMessage(hTimetable, LVM_INSERTITEM, 0, (LPARAM)&lvi);

				lvi.mask = LVIF_TEXT | LVIF_STATE; lvi.state = 0; lvi.stateMask = 0;
				lvi.iItem = item; lvi.iSubItem = 1; lvi.pszText = (LPWSTR)entry.day.c_str();
				SendMessage(hTimetable, LVM_SETITEM, 0, (LPARAM)&lvi);

				lvi.iItem = item; lvi.iSubItem = 2; lvi.pszText = (LPWSTR)entry.name.c_str();
				SendMessage(hTimetable, LVM_SETITEM, 0, (LPARAM)&lvi);
				item++;
			}
		}
		// Time dropdown menu
		{
			const HWND hTime = GetDlgItem(hwnd, IDC_TTIME);
			const int EntryIndex = EntryExists(worldtime);

			if (EntryIndex < 0)
				::EnableWindow(hTime, FALSE);
			else
			{
				const int fuck = *reinterpret_cast<const int*>(variables[EntryIndex].value.data()) + 2;
				const int worldtime = fuck > 24 ? 2 : fuck;
				int time = 2;
				while (time <= 24)
				{
					std::wstring timeStr = std::to_wstring(time);
					SendMessage(hTime, (uint32_t)CB_ADDSTRING, 0, (LPARAM)timeStr.c_str());

					// Set current time
					if (std::abs((float)(worldtime - time) - kindasmall) < 1.f)
						SendMessage(hTime, CB_SETCURSEL, (time - 2) / 2, 0);

					time = time + 2;
				}
			}
		}
		// Day dropdown menu
		{
			const HWND hDay = GetDlgItem(hwnd, IDC_TDAY);
			const int EntryIndex = EntryExists(worldday);

			if (EntryIndex < 0)
				::EnableWindow(hDay, FALSE);
			else
			{
				const int worldday = *reinterpret_cast<const int*>(variables[EntryIndex].value.data()) - 1;
				int entry = 0;
				for (auto& day : { L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday", L"Sunday" })
				{
					SendMessage(hDay, (uint32_t)CB_ADDSTRING, 0, (LPARAM)day);
					// Set current day
					if (worldday == entry)
						SendMessage(hDay, CB_SETCURSEL, entry, 0);
					entry++;
				}
			}
		}
		// Weather dropdown menu
		{
			const HWND hWeather = GetDlgItem(hwnd, IDC_TWEATHER);
			const int WeatherCloudEntry = EntryExists(weathercloud);
			const int WeatherPosEntry = EntryExists(weatherpos);
			const int WeatherTypeEntry = EntryExists(weathertype);
			if (WeatherCloudEntry < 0 || WeatherPosEntry < 0 || WeatherTypeEntry < 0)
				::EnableWindow(hWeather, FALSE);
			else
			{
				for (auto& weather : weathertypes)
				{
					SendMessage(hWeather, (uint32_t)CB_ADDSTRING, 0, (LPARAM)weather.c_str());
				}
			}
		}
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_APPLY:
		{
			// Time
			{
				auto sel = SendMessage(GetDlgItem(hwnd, IDC_TTIME), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				if (sel != CB_ERR)
				{
					int time = (sel + 1) * 2 - 2;
					UpdateValue(L"", EntryExists(worldtime), IntToBin(time = 0 ? 24 : time));
				}
			}
			// Day
			{
				auto sel = SendMessage(GetDlgItem(hwnd, IDC_TDAY), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				if (sel != CB_ERR)
					UpdateValue(L"", EntryExists(worldday), IntToBin(sel + 1));
			}
			// Weather
			{
				auto sel = SendMessage(GetDlgItem(hwnd, IDC_TWEATHER), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0); // 0: clear, 1: overcast, 2: rainy, 3: thunderstorm

				// Clear Sky
				if (sel == 0) 
				{
					// Move clouds off the map
					int PosIndex = EntryExists(weatherpos);
					std::string tval = variables[PosIndex].value;
					tval.replace(1, 4, FloatToBin(17000.f));
					UpdateValue(L"", PosIndex, tval);

					// Set clouds to clear
					UpdateValue(L"", EntryExists(weathertype), IntToBin(0));
				}
				// Not clear sky
				else if (sel > 0)
				{
					// Center clouds on player
					int PosIndex = EntryExists(weatherpos);
					int PlayerIndex = EntryExists(L"player");

					std::string tval = variables[PosIndex].value;
					tval.replace(1, 4, FloatToBin(PlayerIndex >= 0 ? BinToFloat(variables[PlayerIndex].value.substr(1, 4)) : 0.f));
					tval.replace(9, 4, FloatToBin(PlayerIndex >= 0 ? BinToFloat(variables[PlayerIndex].value.substr(9, 4)) : 0.f));
					UpdateValue(L"", PosIndex, tval);

					// Set weather appropiately
					UpdateValue(L"", EntryExists(weathertype), IntToBin(sel - 1)); // 0: dry, 1: rain, 2: thunder

					// Set cloud type
					UpdateValue(L"", EntryExists(weathercloud), IntToBin(cloudtypes[sel - 1]));
				}
			}
			// leak into DISCARD
		}
		case IDC_DISCARD:
		{
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}
		}
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL ReportBoltsProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
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

		uint32_t item = 0;
		Overview ov;

		SendMessage(hList3, WM_SETREDRAW, 0, 0);

		// Re-Populate carparts, just in case
		PopulateCarparts();
		for (uint32_t i = 0; i < carparts.size(); i++)
			PopulateBList(hwnd, &carparts[i], item, &ov);

		// Draw list
		SendMessage(hList3, WM_SETREDRAW, 1, 0);

		// Draw overview
		UpdateBOverview(hwnd, &ov);

		// Enable mesh button if file exists
		::EnableWindow(GetDlgItem(hwnd, IDC_BBUT4), FALSE);
		std::size_t found = filepath.find_last_of(L"\\");
		if (found != std::string::npos && ContainsStr(filepath, L"Amistech"))
		{
			std::wstring meshsavepath = filepath.substr(0, found + 1) + L"meshsave.txt";
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind = FindFirstFile(meshsavepath.c_str(), &FindFileData);
			if (hFind != INVALID_HANDLE_VALUE)
				::EnableWindow(GetDlgItem(hwnd, IDC_BBUT4), TRUE);
		}
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
			// Repair all parts
			if (!carparts.empty())
			{
				BatchProcessDamage(TRUE);
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT4: 
		{
			// Repair bodywork
			const std::wstring filename = L"meshsave.txt";

			std::wstring buffer(512, '\0');
			buffer.resize(swprintf(&buffer[0], 512, GLOB_STRS[48].c_str(), filename.c_str()));
			if (MessageBox(NULL, buffer.c_str(), ErrorTitle.c_str(), MB_YESNO | MB_ICONINFORMATION) == IDYES)
			{
				std::size_t found = filepath.find_last_of(L"\\");
				if (found != std::string::npos && ContainsStr(filepath, L"Amistech"))
				{
					std::wstring meshsavepath = filepath.substr(0, found + 1) + filename;
					if (DeleteFile(meshsavepath.c_str()) == 0)
						MessageBox(hwnd, GLOB_STRS[46].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
					else
						::EnableWindow(GetDlgItem(hwnd, IDC_BBUT4), FALSE);
				}
			}
			break;
		}
		case IDC_BBUT3:
			// Fix stuck parts
			if (!carparts.empty())
			{
				BatchProcessStuck();
				UpdateBDialog(hwnd);
			}
			break;
		case IDC_BBUT5:
			// Install wiring
			if (!carparts.empty())
			{
				BatchProcessWiring();
				//BatchProcessUninstall();
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

// Some controls inside the property list allocated some data on the heap, so we need to clean it up upon destroying
LRESULT CALLBACK PropertyListControlProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK PropertyListProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static int s_scrollYdelta = 0;
	static HBRUSH s_backgroundBrush;
	static uint32_t initial_size;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		initial_size = carproperties.size();
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
			SecureZeroMemory(&si, sizeof(SCROLLINFO));
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
		clkloc.x = GET_X_LPARAM(lParam);
		clkloc.y = GET_Y_LPARAM(lParam);
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
			StaticRekt.bottom -= 2 + StaticRekt.top;
			StaticRekt.right -= 2 + StaticRekt.left;

			// Fetch the value and create edit-box
			std::wstring vStr = variables[carproperties[pIndex].index].GetDisplayString();
			hEditValue = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, vStr.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, StaticRekt.left, StaticRekt.top, StaticRekt.right, StaticRekt.bottom, hwnd, (HMENU)IDC_PLEDIT, hInst, NULL);
			SendMessage(hEditValue, WM_SETFONT, (WPARAM)hListFont, TRUE);
			SendMessage(hEditValue, EM_SETLIMITTEXT, 16, 0);

			// Alloc memory on heap to store edit controls property ID and default window process
			AllocWindowUserData(hEditValue, (LONG)PropertyListControlProc, cID - ID_PL_EDIT /* We don't put the index here, because we can just get the index from the row*/);

			// Create apply button
			HWND hApplyButton = CreateWindowEx(0, WC_BUTTON, _T("set"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, StaticRekt.left + StaticRekt.right + 16, StaticRekt.top, 35, StaticRekt.bottom, hwnd, (HMENU)IDC_PLSET, hInst, NULL);
			SendMessage(hApplyButton, WM_SETFONT, (WPARAM)hListFont, TRUE);

			// Hide fix button if it's present
			HWND hFixButton = GetDlgItem(hwnd, ID_PL_BUTTON + (cID - ID_PL_EDIT));
			if (hFixButton != NULL)
				ShowWindow(hFixButton, SW_HIDE);
		}
		break;
	}
	case WM_DESTROY:
	{
		if (carproperties.size() > 0 && initial_size > 0)
			carproperties.resize(initial_size);
		DeleteObject(s_backgroundBrush);
		break;
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL ReportMaintenanceProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		// Create the child list
		const int ListWidth = 380;
		HWND hProperties = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BLANK), hwnd, PropertyListProc);
		ShowWindow(hProperties, SW_SHOW);
		SetWindowPos(hProperties, NULL, 12, 13, ListWidth, 423, SWP_SHOWWINDOW);

		// Create description text
		int sbSize = GetSystemMetrics(SM_CXVSCROLL);
		RECT DlgRekt;
		GetWindowRect(hwnd, &DlgRekt);
		int dlgSize = DlgRekt.right - DlgRekt.left;
		int dtLeft = ListWidth + sbSize + 12;

		HWND hDescriptionText = CreateWindowEx(0, WC_STATIC, GLOB_STRS[45].c_str(), WS_CHILD | WS_VISIBLE, dtLeft, 13, dlgSize - dtLeft - 13, 423, hwnd, NULL, hInst, NULL);
		SendMessage(hDescriptionText, WM_SETFONT, (WPARAM)hListFont, TRUE);

		// Don't do anything if we don't have anything loaded
		if (variables.size() == 0)
			break;

		// Process the carproperty vector
		uint32_t initial_size = carproperties.size();
		for (uint32_t i = 0; i < variables.size(); i++)
		{
			// Here we set indices of elements read in from the datafile for faster lookup later
			for (uint32_t j = 0; j < carproperties.size(); j++)
			{
				// If bool is > 1 then it's a wildcard, which we're not going to assign an index to directly
				BOOL IdentifiersEqual = CompareStrsWithWildcard(variables[i].key, carproperties[j].lookupname);
				if (IdentifiersEqual > 0)
				{
					bool bRelevant = TRUE;
					if (!carparts.empty())
					{
						for (uint32_t k = 0; k < carparts.size(); k++)
						{
							if (StartsWithStr(variables[i].key, carparts[k].name) && carparts[k].iInstalled != UINT_MAX)
							{
								bRelevant = variables[carparts[k].iInstalled].value.c_str()[0];
								break;
							}
						}
					}
					if (bRelevant)
					{
						if (IdentifiersEqual == 2)
							carproperties.push_back(CarProperty(carproperties[j].displayname, variables[i].key, carproperties[j].datatype ,carproperties[j].worstBin, carproperties[j].optimumBin, carproperties[j].recommendedBin, i));
						else
							carproperties[j].index = i;
					}
					break;
				}
			}

			// Now we add elements that contain wear in their key to the property-list
			if (ContainsStr(variables[i].key, STR_WEAR) && variables[i].header.IsNonContainerOfValueType(EntryValue::Float) || variables[i].header.IsNonContainerOfValueType(EntryValue::Vector3))
			{
				bool bAlreadyExists = FALSE;
				for (uint32_t j = 0; j < initial_size; j++)
				{
					if (CompareStrsWithWildcard(variables[i].key, carproperties[j].lookupname))
					{
						bAlreadyExists = TRUE;
						break;
					}
				}
				// If we have no defined carparts, we just assume everything is installed and display everything
				bool bIsInstalled = carparts.empty();
				for (uint32_t j = 0; ; j++)
				{
					if (j >= carparts.size() || bIsInstalled)
					{
						// If we couldn't determine if the related carpart is installed, we just assume it is
						bIsInstalled = TRUE;
						break;
					}
					if (StartsWithStr(variables[i].key, carparts[j].name))
					{
						// If the part has installed key, and is installed, we consider it as installed
						if (bIsInstalled = carparts[j].iInstalled != UINT_MAX && variables[carparts[j].iInstalled].value.c_str()[0])
							break;

						// If the wheel has corner key, and is not empty, we consider it as installed
						bIsInstalled = carparts[j].iCorner != UINT_MAX && variables[carparts[j].iCorner].value.size() > 1;
						break;
					}
				}

				// If the variable wasn't already in the maintenance entries read in from file, and is installed 
				if (!bAlreadyExists && bIsInstalled)
				{
					// Build display string
					std::wstring displayname = variables[i].key;
					std::size_t pos1 = variables[i].key.find(STR_WEAR);
					if (pos1 != std::wstring::npos)
					{
						displayname.replace(pos1, 4, _T(""));
						displayname += _T(" ");
						displayname += STR_STATE;
						std::transform(displayname.begin(), displayname.begin() + 1, displayname.begin(), ::toupper);
						CarProperty cp = CarProperty(displayname, L"", EntryValue::Float, FloatToBin(0), FloatToBin(100)); 
						cp.index = i;
						carproperties.push_back(cp);
					}
				}
			}
		}

		TEXTMETRIC tm;
		HDC hdc = GetDC(hwnd);
		GetTextMetrics(hdc, &tm);
		int yChar = tm.tmHeight + tm.tmExternalLeading;

		int offset = 6;
		int RowID = 0;

		// For every entry in carproperties we add a row
		for (uint32_t i = 0; i < carproperties.size(); i++)
		{
			// But make sure it has an index set, otherwise it doesn't exist in the variable vector
			if (carproperties[i].index != UINT_MAX)
			{
				// Draw display text
				std::wstring value;
				COLORREF tColor;
				bool bActionAvailable;

				HWND hDisplayText = CreateWindowEx(0, WC_STATIC, carproperties[i].displayname.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 6, 0 + offset, 150, yChar + 1, hProperties, (HMENU)ID_PL_DTXT + RowID, hInst, NULL);
				SendMessage(hDisplayText, WM_SETFONT, (WPARAM)hListFont, TRUE);

				bActionAvailable = GetStateLabelSpecs(&carproperties[i], value, tColor);

				HWND hDisplayState = CreateWindowEx(0, WC_STATIC, value.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 162, 0 + offset, 40, yChar + 1, hProperties, (HMENU)(ID_PL_STATE + RowID) , hInst, NULL);
				SendMessage(hDisplayState, WM_SETFONT, (WPARAM)hListFont, TRUE);

				// Alloc memory on heap to store state's color and default window process
				AllocWindowUserData(hDisplayState, (LONG)PropertyListControlProc, 0, tColor);

				value = variables[carproperties[i].index].GetDisplayString();
				HWND hDisplayValue = CreateWindowEx(0, WC_STATIC, value.c_str(), SS_SIMPLE | WS_CHILD | WS_VISIBLE, 210, 0 + offset, 100, yChar + 1, hProperties, (HMENU)(ID_PL_EDIT + RowID), hInst, NULL);
				SendMessage(hDisplayValue, WM_SETFONT, (WPARAM)hListFont, TRUE);

				// Alloc memory on heap to store DisplayValues property ID and default window process (ID needed by fix button)
				AllocWindowUserData(hDisplayValue, (LONG)PropertyListControlProc, i, tColor);

				if (bActionAvailable)
				{
					HWND hFixButton = CreateWindowEx(0, WC_BUTTON, _T("fix"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 324, 0 + offset - 2, 20, yChar + 1, hProperties, (HMENU)(ID_PL_BUTTON + RowID), hInst, NULL);
					SendMessage(hFixButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
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

BOOL CALLBACK ReportChildrenProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK TeleportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static std::string TargetLocationBin;
	static std::vector<uint32_t> vfrom;
	static std::vector<std::pair<uint32_t, uint32_t>> vto;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		if (lParam == 0)
			TargetLocationBin.clear();
		else
			TargetLocationBin = std::move(*reinterpret_cast<std::string*>(lParam));

		if (TargetLocationBin.empty())
			for (uint32_t i = 0; i < locations.size(); i++)
			{
				SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_ADDSTRING, 0, (LPARAM)locations[i].first.first.c_str());
				vto.push_back(std::pair<uint32_t, uint32_t>(0, i));
			}

		for (uint32_t i = 0; i < variables.size(); i++)
		{
			if (variables[i].header.IsNonContainerOfValueType(EntryValue::Transform))
			{
				SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (uint32_t)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
				if (TargetLocationBin.empty())
				{
					SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
					vto.push_back(std::pair<uint32_t, uint32_t>(1, i));
				}
				vfrom.push_back(i);
			}
		}
		if (!TargetLocationBin.empty())
		{
			SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_ADDSTRING, 0, (LPARAM)Variable::ValueBinToStr(TargetLocationBin, EntryValue::Vector3).c_str());
			SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_SETCURSEL, 0, 0);
			::EnableWindow(GetDlgItem(hwnd, IDC_LOCTO), FALSE);
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
		case IDC_LOCFROM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
					::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			break;
		case IDC_LOCTO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				if (SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
					::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			break;
		case IDC_APPLY:
		{
			// Get height offset
			std::wstring offsetStr = GetEditboxString(GetDlgItem(hwnd, IDC_OFFSET));
			float offset = IsValidFloatStr(offsetStr) ? static_cast<float>(::strtod((WStringToString(*TruncFloatStr(offsetStr))).c_str(), NULL)) : 0;

			// Get binary of teleportation target transform
			uint32_t index = SendMessage(GetDlgItem(hwnd, IDC_LOCFROM), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			uint32_t varindex = vfrom[index];
			std::string binfrom = variables[varindex].value;

			// Get binary of teleportation destination transform
			std::string binto = binfrom;
			if (TargetLocationBin.empty())
			{
				index = SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				bool bIsFileLocation = vto[index].first == 0;
				binto = bIsFileLocation ? locations[vto[index].second].second : variables[vto[index].second].value;
			}
			else
				binto.replace(1, TargetLocationBin.size(), TargetLocationBin);

			// Add height offset if it has any
			if (offset > 0)
				binto.replace(5, 4, FloatToBin(BinToFloat((binto.substr(5, 4))) + offset));

			// Keep original rotation if desired
			int len = IsDlgButtonChecked(hwnd, IDC_TELEC) == BST_CHECKED ? 13 : 41;

			// Overwrite target transform binary with portion of destination transform
			binfrom.replace(0, len, binto.substr(0, len));

			UpdateValue(_T(""), varindex, binfrom);
			// Leak into Discard
		}
		case IDC_DISCARD:
		{
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}
		}
		break;
	}
	case WM_DESTROY:
	{
		vfrom.clear();
		vto.clear();
		break;
	}
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK TransformProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		std::string value = variables[indextable[iItem].second].value.substr(1);

		//location 

		std::wstring str = BinToFloatVector(value, 3);
		SendMessage((GetDlgItem(hwnd, IDC_LOC)), WM_SETTEXT, 0, (LPARAM)str.c_str());

		//rotation

		if (!bEulerAngles)
		{
			str = BinToFloatVector(value, 4, 3);
			SendMessage((GetDlgItem(hwnd, IDC_ROTBOX)), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[27].c_str());
		}
		else
		{
			str.clear();
			QTRN q;
			ANGLES a;

			q.x = BinToFloat(value.substr(12 + 0, 4));
			q.y = BinToFloat(value.substr(12 + 4, 4));
			q.z = BinToFloat(value.substr(12 + 8, 4));
			q.w = BinToFloat(value.substr(12 + 12, 4));
			a = QuatToEuler(&q);

			for (auto& elem : { a.x, a.y, a.z })
			{
				std::wstring tstr = std::to_wstring(elem);
				str += (*TruncFloatStr(tstr) + (L", "));
			}
			str.resize(str.size() - 2);
			SendMessage((GetDlgItem(hwnd, IDC_ROTBOX)), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[28].c_str());
		}
		SendMessage((GetDlgItem(hwnd, IDC_ROT)), WM_SETTEXT, 0, (LPARAM)str.c_str());

		//scale
		str = BinToFloatVector(value, 3, 7);
		SendMessage((GetDlgItem(hwnd, IDC_SCA)), WM_SETTEXT, 0, (LPARAM)str.c_str());

		//MSC dev requested removal of scale property
		if (!bAllowScale)
			::EnableWindow(GetDlgItem(hwnd, IDC_SCA), FALSE);

		//tag
		if (value.size() > 40)
		{
			str = BinStrToWStr(value.substr(40));
			SendMessage((GetDlgItem(hwnd, IDC_TAG)), WM_SETTEXT, 0, (LPARAM)str.c_str());
			SendMessage((GetDlgItem(hwnd, IDC_TAG)), EM_SETLIMITTEXT, 255, 0);
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
				uint32_t error;
				bool valid = TRUE;
				std::wstring value;
				std::string bin(1, char(4));

				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = ErrorTitle.c_str();
				ebt.ttiIcon = TTI_ERROR_LARGE;

				//location
				std::wstring lvalue = GetEditboxString(GetDlgItem(hwnd, IDC_LOC));

				error = VectorStrToBin(lvalue, 3, bin);
				if (error > 0)
				{
					valid = FALSE;
					ebt.pszText = GLOB_STRS[error].c_str();
					SendMessage(GetDlgItem(hwnd, IDC_LOC), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}

				//rotation
				value = GetEditboxString(GetDlgItem(hwnd, IDC_ROT));

				if (!bEulerAngles)
				{
					error = VectorStrToBin(value, 4, bin, TRUE, TRUE);
					if (error > 0)
					{
						valid = FALSE;
						ebt.pszText = GLOB_STRS[error].c_str();
						SendMessage(GetDlgItem(hwnd, IDC_ROT), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}
				else
				{
					std::string oldvar = variables[indextable[iItem].second].value.substr(1);
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
						SendMessage(GetDlgItem(hwnd, IDC_ROT), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}

				//scale
				value = GetEditboxString(GetDlgItem(hwnd, IDC_SCA));

				if (bAllowScale)
				{
					error = VectorStrToBin(value, 3, bin);
					if (error > 0)
					{
						valid = FALSE;
						ebt.pszText = GLOB_STRS[error].c_str();
						SendMessage(GetDlgItem(hwnd, IDC_SCA), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
					}
				}
				else
					bin += variables[indextable[iItem].second].value.substr(29, 12);

				if (valid)
				{
					value = GetEditboxString(GetDlgItem(hwnd, IDC_TAG));
					bin += WStrToBinStr(value);
					UpdateValue(lvalue, indextable[iItem].second, bin);

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

BOOL CALLBACK ColorProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static bool bIsColorProc = TRUE;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		bIsColorProc = (lParam == 1);
		std::wstring str = Variable::ValueBinToStr(variables[indextable[iItem].second].value, bIsColorProc ? EntryValue::Color : EntryValue::Vector3);
		SendMessage((GetDlgItem(hwnd, IDC_COL)), WM_SETTEXT, 0, (LPARAM)str.c_str());
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
				uint32_t error, size;
				std::wstring value;
				std::string bin;
				HWND hEdit;
				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = ErrorTitle.c_str();
				ebt.ttiIcon = TTI_ERROR_LARGE;

				hEdit = GetDlgItem(hwnd, IDC_COL);
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
					SendMessage(GetDlgItem(hwnd, IDC_COL), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}
				else
				{
					UpdateValue(value, indextable[iItem].second, bin);

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

BOOL CALLBACK ContainerEditListChildProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE;
	switch (Message)
	{
	case WM_DESTROY:
	{
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
		plID = nullptr;
		return TRUE;
	}
	}
	return CallWindowProc(plID->hDefProc, hwnd, Message, wParam, lParam);
}

BOOL CALLBACK ContainerEditListProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hListEdit = NULL;
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE;

	switch (Message)
	{
	case WM_LBUTTONDOWN:
	{
		LVHITTESTINFO itemclicked;
		itemclicked.pt.x = GET_X_LPARAM(lParam);
		itemclicked.pt.y = GET_Y_LPARAM(lParam);
		const int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		DestroyWindow(hListEdit);
		SendMessage(hwnd, WM_DESTROYBUTTONS, 0, 0);
		if (lResult != -1)
		{
			RECT rekt;
			
			ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_BOUNDS, &rekt);
			const int pos_left = rekt.left;
			ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_LABEL, &rekt);
			const int height = rekt.bottom - rekt.top;
			

			if (itemclicked.iSubItem > 0)
			{
				const int szbuf = 255;
				const int buttonwidth = 45;
				const int width = rekt.right - pos_left - 2 * buttonwidth;
				
				std::wstring buffer(szbuf, '\0');
				ListView_GetItemText(hwnd, itemclicked.iItem, itemclicked.iSubItem, &buffer[0], szbuf);

				hListEdit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), &buffer[0], WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, pos_left, rekt.top, width, height, hwnd, (HMENU)1337, hInst, NULL);
				AllocWindowUserData(hListEdit, (LONG)ContainerEditListChildProc, itemclicked.iItem, RGB(255, 255, 255), itemclicked.iSubItem);
				SendMessage(hListEdit, WM_SETFONT, (WPARAM)hListFont, TRUE);
				SendMessage(hListEdit, EM_SETLIMITTEXT, szbuf, 0);
				SetFocus(hListEdit);
				SendMessage(hListEdit, EM_SETSEL, 0, -1);
				SendMessage(hListEdit, EM_SETSEL, -1, 0);

				// Create apply button
				HWND hApplyButton = CreateWindowEx(0, WC_BUTTON, _T("Set"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos_left + width, rekt.top, buttonwidth, height, hwnd, (HMENU)IDC_BT1, hInst, NULL);
				SendMessage(hApplyButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
				// Create cancel button
				HWND hCancelButton = CreateWindowEx(0, WC_BUTTON, _T("Cancel"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos_left + width + buttonwidth, rekt.top, buttonwidth, height, hwnd, (HMENU)IDC_BT2, hInst, NULL);
				SendMessage(hCancelButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
			}
			else
			{
				const int width = rekt.right - pos_left;
				const int buttonwidth = static_cast<int>(width * 0.6f);
				const int buttonwidth2 = static_cast<int>((width - buttonwidth) / 2);
				// Create + button
				HWND hAddButton = CreateWindowEx(0, WC_BUTTON, L"+", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos_left, rekt.top, buttonwidth2, height, hwnd, (HMENU)IDC_BT3, hInst, NULL);
				SendMessage(hAddButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
				// Create x button
				HWND hRemoveButton = CreateWindowEx(0, WC_BUTTON, L"-", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos_left + buttonwidth2, rekt.top, buttonwidth2, height, hwnd, (HMENU)IDC_BT4, hInst, NULL);
				SendMessage(hRemoveButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
				// Create cancel button
				HWND hCancelButton = CreateWindowEx(0, WC_BUTTON, L"Cancel", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, pos_left + (width - buttonwidth), rekt.top, buttonwidth, height, hwnd, (HMENU)IDC_BT5, hInst, NULL);
				SendMessage(hCancelButton, WM_SETFONT, (WPARAM)hListFont, TRUE);
				AllocWindowUserData(hCancelButton, (LONG)ContainerEditListChildProc, itemclicked.iItem, RGB(255, 255, 255), itemclicked.iSubItem);
			}
		}
		return FALSE;
	}
	case WM_DESTROYBUTTONS:
	{
		for (auto &id : { IDC_BT1, IDC_BT2, IDC_BT3, IDC_BT4, IDC_BT5 })
			DestroyWindow(GetDlgItem(hwnd, id));
		break;
	}
	case WM_CONTAINERUPDATEINDECES:
	{
		auto NumItems = ListView_GetItemCount(hwnd);
		for (int32_t i = 0; i < NumItems; i++)
		{
			auto str = std::to_wstring(i);
			ListView_SetItemText(hwnd, i, 0, &str[0]);
		}
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_BT1: // Apply
			{
				PLID *plID = (PLID *)GetWindowLong(hListEdit, GWL_USERDATA);
				if (plID != nullptr)
				{
					std::wstring str = GetEditboxString(hListEdit);
					ListView_SetItemText(hwnd, plID->nPID, plID->dat, &str[0]);
				}
				// Leak into Discard
			}
			case IDC_BT2: // Discard
			{
				DestroyWindow(hListEdit);
				DestroyWindow(GetDlgItem(hwnd, IDC_BT1));
				DestroyWindow(GetDlgItem(hwnd, IDC_BT2));
				break;
			}
			case IDC_BT3: // +
			case IDC_BT4: // -
			{
				// The Cancel button contains info about which row and column we're in
				PLID *plID = (PLID *)GetWindowLong(GetDlgItem(hwnd, IDC_BT5), GWL_USERDATA);
				if (plID == nullptr)
					break;
				if (LOWORD(wParam) == IDC_BT4) // -
				{
					ListView_DeleteItem(hwnd, plID->nPID);
					PostMessage(hwnd, WM_CONTAINERUPDATEINDECES, 0, 0);
				}
				else
				{
					LVITEM lvi;
					std::wstring IndexStr = std::to_wstring(plID->nPID + 1);
					lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
					lvi.iItem = plID->nPID + 1; lvi.iSubItem = 0; lvi.pszText = &IndexStr[0], lvi.lParam = 0;
					SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)&lvi);
					PostMessage(hwnd, WM_CONTAINERUPDATEINDECES, 0, 0);
				}
				// Leak into Cancel
			}
			case IDC_BT5 : // Cancel
			{
				SendMessage(hwnd, WM_DESTROYBUTTONS, 0, 0);
				break;
			}
			}
		}
		break;
	}
	case WM_DESTROY:
	{
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
		plID = nullptr;
		return TRUE;
	}
	}
	return CallWindowProc(plID->hDefProc, hwnd, Message, wParam, lParam);
}

BOOL CALLBACK ContainerEditProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static BOOL bTwoColumns;
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		bTwoColumns = variables[indextable[iItem].second].header.GetContainerKeyType() != EntryValue::Null;
		const HWND hContainerList = GetDlgItem(hwnd, IDC_CONTAINERLIST);
		ListView_SetExtendedListViewStyle(hContainerList, LVS_EX_GRIDLINES);

		AllocWindowUserData(hContainerList, (LONG)ContainerEditListProc);

		RECT rekt;
		GetWindowRect(hContainerList, &rekt);
		const int width = rekt.right - rekt.left - 4 - GetSystemMetrics(SM_CXVSCROLL);

		// Column headers
		
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		int indexwidth = static_cast<int>(width * 0.1f);
		int entrywidth = static_cast<int>((width - indexwidth) / (bTwoColumns + 1));
		lvc.iSubItem = 1 + bTwoColumns; lvc.pszText = _T("Value"); lvc.cx = entrywidth; lvc.fmt = LVCFMT_LEFT;
		SendMessage(hContainerList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		if (bTwoColumns)
		{
			lvc.iSubItem = 1; lvc.pszText = _T("Key"); lvc.cx = entrywidth;
			SendMessage(hContainerList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);
		}

		lvc.iSubItem = 0; lvc.pszText = _T("Index"); lvc.cx = indexwidth;
		SendMessage(hContainerList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

		// Entries
		const Variable *var = &variables[indextable[iItem].second];
		const std::string *str = &var->value;
		const uint32_t num = *reinterpret_cast<const int*>(str->data());
		uint32_t offset = 4;
		LVITEM lvi;
		for (uint32_t i = 0; i < num; i++)
		{
			std::wstring IndexStr = std::to_wstring(i);
			lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0;
			lvi.iItem = i; lvi.iSubItem = 0; lvi.pszText = &IndexStr[0], lvi.lParam = 0;
			SendMessage(hContainerList, LVM_INSERTITEM, 0, (LPARAM)&lvi);

			for (auto &index : { 1 , 2 })
			{
				uint32_t len = static_cast<int>((*str)[offset]) + 1;
				std::wstring EntryStr = Variable::ValueBinToStr(str->substr(offset, len), bTwoColumns && index == 1 ? var->header.GetContainerKeyType() : var->header.GetValueType());

				lvi.mask = LVIF_TEXT | LVIF_STATE; lvi.state = 0; lvi.stateMask = 0;
				lvi.iItem = i; lvi.iSubItem = index; lvi.pszText = (LPWSTR)EntryStr.c_str();
				SendMessage(hContainerList, LVM_SETITEM, 0, (LPARAM)&lvi);
				offset += len;
				if (!bTwoColumns)
					break;
			}
		}
		break;
	}
	case WM_NOTIFY:
	{
		if ((((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) && ListView_GetItemCount(GetDlgItem(hwnd, IDC_CONTAINERLIST)) == 0)
		{
			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; lvi.state = 0; lvi.stateMask = 0; lvi.iItem = 0; lvi.iSubItem = 0; lvi.pszText = L"0", lvi.lParam = 0;
			SendMessage(GetDlgItem(hwnd, IDC_CONTAINERLIST), LVM_INSERTITEM, 0, (LPARAM)&lvi);
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
				const HWND hContainerList = GetDlgItem(hwnd, IDC_CONTAINERLIST);
				const auto NumItems = ListView_GetItemCount(hContainerList);
				const int sztext = 255;
				std::string bin = IntToBin(NumItems);
				for (int32_t i = 0; i < NumItems; i++)
				{
					for (int32_t j = 1; j < (bTwoColumns + 2); j++)
					{
						std::wstring buffer(sztext, '\0');
						LVITEM lvi;
						lvi.mask = LVIF_TEXT; lvi.cchTextMax = sztext; lvi.iSubItem = j; lvi.pszText = &buffer[0];
						buffer.resize(SendMessage(hContainerList, LVM_GETITEMTEXT, i, (LPARAM)&lvi));
						bin += WStrToBinStr(buffer);
					}
				}
				UpdateValue(L"", indextable[iItem].second, bin);
				// Leak into Discard
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

BOOL CALLBACK KeyListProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
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

		ScrollWindowEx(hwnd, 0, s_scrollYdelta, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);
		SetScrollPos(hwnd, SB_VERT, scrollY, TRUE);

		UpdateWindow(hwnd);
		break;
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
			int pagesize = static_cast<int>(pow(ScrollRekt.bottom, 2) / wParam);
			SCROLLINFO si = {};
			SecureZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMax = windowsize + pagesize;
			si.nPage = pagesize;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		}
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

BOOL CALLBACK KeyManagerProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	static std::vector<std::wstring> KeyNames = { L"keyferndale", L"keygifu", L"keyhayosiko", L"keyhome", L"keyruscko", L"keysatsuma"};
	static HWND hKeys;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Create child key list. We probably don't need a scrollbar here, ever - but it's better to be on the safe side
		const int BorderMargin = 12;
		RECT rekt;
		GetClientRect(hwnd, &rekt);
		const int ListWidth = rekt.right - rekt.left - 2 * BorderMargin;
		const int ListHeight = rekt.bottom - rekt.top - 37 - 2 * BorderMargin;

		hKeys = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BLANK), hwnd, KeyListProc);
		SetWindowPos(hKeys, NULL, BorderMargin, BorderMargin, ListWidth, ListHeight, SWP_SHOWWINDOW);
		ShowWindow(hKeys, SW_SHOW);

		TEXTMETRIC tm;
		HDC hdc = GetDC(hwnd);
		GetTextMetrics(hdc, &tm);
		int yChar = tm.tmHeight + tm.tmExternalLeading;
		int offset = 6;

		// For every key we add a checkbox, if the coresponding location# was found
		for (uint32_t index = 0; index < KeyNames.size(); index++)
		{
			std::wstring LocationName = L"location" + std::to_wstring(index + 1);
			if ((EntryExists(LocationName) < 0) || EntryExists(KeyNames[index]) < 0 )
				continue;

			HWND hCheckBox = CreateWindowEx(0, WC_BUTTON, KeyNames[index].substr(3).c_str(), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 6, 0 + offset, 150, yChar + 1, hKeys, (HMENU)index, hInst, NULL);
			SendMessage(hCheckBox, WM_SETFONT, (WPARAM)hListFont, TRUE);

			// Get the result
			const int Result = *reinterpret_cast<const int*>(variables[EntryExists(KeyNames[index])].value.substr(0, 4).data());

			// Get the multiplier
			std::wstring MultiplierStr = Variable::ValueBinToStr(variables[EntryExists(std::wstring(L"keycheck"))].value, EntryValue::Integer);

			const int Multiplier = std::stoi(MultiplierStr, NULL);

			// Get the base value
			const std::string BaseValue = variables[EntryExists(LocationName)].value.substr(1);
			const int Base = static_cast<int>(*reinterpret_cast<const float*>(BaseValue.substr(0, 4).data()) + 2.f * *reinterpret_cast<const float*>(BaseValue.substr(4, 4).data()) + *reinterpret_cast<const float*>(BaseValue.substr(8, 4).data()));
			
			// If the result is correct, check the box
			if ((Base * Multiplier) == Result)
				CheckDlgButton(hKeys, index, BST_CHECKED);

			offset += yChar;
		}

		// Apply size
		PostMessage(hKeys, WM_UPDATESIZE, offset + 3, NULL);

		// Invalidate and redraw to avoid transparency issues
		GetClientRect(hKeys, &rekt);
		InvalidateRect(hKeys, &rekt, TRUE);
		RedrawWindow(hKeys, &rekt, NULL, RDW_ERASE | RDW_INVALIDATE);
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
				for (uint32_t index = 0; index < KeyNames.size(); index++)
				{
					std::wstring LocationName = L"location" + std::to_wstring(index + 1);
					if ((EntryExists(LocationName) < 0) || EntryExists(KeyNames[index]) < 0)
						continue;

					const int ResultIndex = EntryExists(KeyNames[index]);
					const int Result = *reinterpret_cast<const int*>(variables[ResultIndex].value.substr(0, 4).data());
					std::wstring MultiplierStr = Variable::ValueBinToStr(variables[EntryExists(L"keycheck")].value, EntryValue::Integer);
					const int Multiplier = std::stoi(MultiplierStr, NULL);
					const std::string BaseValue = variables[EntryExists(LocationName)].value.substr(1);
					const int Base = static_cast<int>(*reinterpret_cast<const float*>(BaseValue.substr(0, 4).data()) + 2.f * *reinterpret_cast<const float*>(BaseValue.substr(4, 4).data()) + *reinterpret_cast<const float*>(BaseValue.substr(8, 4).data()));

					if (IsDlgButtonChecked(hKeys, index) == BST_CHECKED && Base * Multiplier != Result)
					{
						UpdateValue(std::to_wstring(Base * Multiplier), ResultIndex);
					}
					else if (IsDlgButtonChecked(hKeys, index) == BST_UNCHECKED && Result != 0)
					{
						UpdateValue(std::to_wstring(0), ResultIndex);
					}
				}
				// Leak into Discard to close the window
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

BOOL CALLBACK IssueProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	static HWND hList;
	static std::vector<Issue> issues;
	static int IssuesFixed;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		IssuesFixed = 0;
		SetWindowText(hwnd, L"Save-file issues");
		const int BorderMargin = 12;
		RECT rekt;
		GetClientRect(hwnd, &rekt);
		const int ListWidth = rekt.right - rekt.left - 2 * BorderMargin;
		const int ListHeight = rekt.bottom - rekt.top - 37 - 2 * BorderMargin;

		hList = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BLANK), hwnd, KeyListProc);
		SetWindowPos(hList, NULL, BorderMargin, BorderMargin, ListWidth, ListHeight, SWP_SHOWWINDOW);

		TEXTMETRIC tm;
		HDC hdc = GetDC(hwnd);
		GetTextMetrics(hdc, &tm);
		int yChar = tm.tmHeight + tm.tmExternalLeading;
		int offset = 6;
		issues = std::move(*reinterpret_cast<std::vector<Issue>*>(lParam));

		std::wstring NumIssues(128, '\0');
		swprintf(&NumIssues[0], 128, GLOB_STRS[50].c_str(), issues.size(), (issues.size() == 1 ? GLOB_STRS[52] : GLOB_STRS[51]).c_str());

		HWND hDescriptionText = CreateWindowEx(0, WC_STATIC, NumIssues.c_str(), WS_CHILD | WS_VISIBLE, BorderMargin, ListHeight + 28, 120, yChar, hwnd, NULL, hInst, NULL);
		SendMessage(hDescriptionText, WM_SETFONT, (WPARAM)hListFont, TRUE);

		SetWindowText(GetDlgItem(hwnd, IDC_APPLY), L"Fix selected");
		SetWindowText(GetDlgItem(hwnd, IDC_DISCARD), L"Ignore");

		for (uint32_t i = 0; i < issues.size(); i++)
		{
			HWND hCheckBox = CreateWindowEx(0, WC_BUTTON, variables[issues[i].first].key.c_str(), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 6, 0 + offset, 150, yChar + 1, hList, (HMENU)i, hInst, NULL);
			CheckDlgButton(hList, i, BST_CHECKED);
			SendMessage(hCheckBox, WM_SETFONT, (WPARAM)hListFont, TRUE);
			offset += yChar;
		}
		// Apply size
		PostMessage(hList, WM_UPDATESIZE, offset + 3, NULL);

		// Invalidate and redraw to avoid transparency issues
		GetClientRect(hList, &rekt);
		InvalidateRect(hList, &rekt, TRUE);
		RedrawWindow(hList, &rekt, NULL, RDW_ERASE | RDW_INVALIDATE);
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
				for (uint32_t i = 0; i < issues.size(); i++)
					if (IsDlgButtonChecked(hList, i) == BST_CHECKED)
					{
						UpdateValue(L"", issues[i].first, issues[i].second);
						IssuesFixed++;
					}
				// Leak into Discard to close the window
			}
			case IDC_DISCARD:
			{
				HWND hOut = GetDlgItem(hDialog, IDC_OUTPUT4);
				ClearStatic(hOut, hDialog);
				int RemainingIssues = issues.size() - IssuesFixed;
				if (RemainingIssues > 0)
				{
					std::wstring buffer(128, '\0');
					swprintf(&buffer[0], 128, GLOB_STRS[50].c_str(), RemainingIssues, (RemainingIssues == 1 ? GLOB_STRS[52] : GLOB_STRS[51]).c_str());
					SendMessage(hOut, WM_SETTEXT, 0, (LPARAM)&buffer[0]);
				}
				else
					SendMessage(hOut, WM_SETTEXT, 0, (LPARAM)L"");
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

BOOL CALLBACK SpawnItemProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (itemTypes.empty() || itemAttributes.empty())
			{
				MessageBox(hDialog, GLOB_STRS[26].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
				EndDialog(hwnd, 0);
				DestroyWindow(hwnd);

				break;
			}

			for (uint32_t i = 0; i < itemTypes.size(); i++)
			{
				SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (uint32_t)CB_ADDSTRING, 0, (LPARAM)StringToWString(itemTypes[i].GetDisplayName()).c_str());
			}

			for (uint32_t i = 0; i < locations.size(); i++)
			{
				SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_ADDSTRING, 0, (LPARAM)locations[i].first.first.c_str());
			}

			for (uint32_t i = 0; i < variables.size(); i++)
			{
				if (variables[i].header.IsNonContainerOfValueType(EntryValue::Transform))
					SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
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
					uint32_t index = 0;  
					std::string location;
					uint32_t index_what = SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
					uint32_t index_to = 0;
					index = SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

					if (index < locations.size())
						location = WStringToString(locations[index].second);
					else
					{
						for (uint32_t i = 0; i < variables.size(); i++)
						{
							if (variables[i].header.IsNonContainerOfValueType(EntryValue::Transform))
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

					
					index = EntryExists(itemTypes[index_what].GetID());
					const bool bNewIdEntry = (index == UINT_MAX);
					std::wstring SanitizedID = *SanitizeTagStr(itemTypes[index_what].GetID());

					// If the ID entry is missing, we create it
					if (bNewIdEntry)
						Variables_add(Variable(Header(EntryValue::Ids[EntryValue::Integer].first), IntToBin(0), UINT_MAX, itemTypes[index_what].GetID(), SanitizedID));

					for (uint32_t i = 0; i < (uint32_t)amount; i++)
					{
						// We have to refind the ID entry here because it might change after iterations
						index = EntryExists(SanitizedID);

						if (index == UINT_MAX)
						{
							MessageBox(hwnd, GLOB_STRS[47].c_str(), Title.c_str(), MB_OK | MB_ICONERROR);
							EndDialog(hwnd, 0);
							DestroyWindow(hwnd);
							EnableWindow(hDialog, TRUE);
							break;
						}

						std::wstring idStr = variables[index].GetDisplayString();
						int a = static_cast<int>(::strtol(WStringToString(idStr).c_str(), NULL, 10)) + 1;
						UpdateValue(std::to_wstring(a), index);
						std::wstring keyprefix = itemTypes[index_what].GetName() + std::to_wstring(a);

						std::wstring SanitizedTransformKey = keyprefix + itemAttributes[0].GetName();
						Variables_add(Variable(Header(EntryValue::Ids[EntryValue::Transform].first), location, UINT_MAX, keyprefix + itemAttributes[0].GetName(), *SanitizeTagStr(SanitizedTransformKey)));

						std::vector<char> attributes = itemTypes[index_what].GetAttributes(itemAttributes.size());

						for (uint32_t j = 0; j < attributes.size(); j++)
						{
							uint32_t type = itemAttributes[(uint32_t)attributes[j]].GetType();
							std::wstring max = std::to_wstring(itemAttributes[(uint32_t)attributes[j]].GetMax());
							std::string bin = Variable::ValueStrToBin(*TruncFloatStr(max), type);
							
							std::wstring SanitizedAttributeKey = keyprefix + itemAttributes[(uint32_t)attributes[j]].GetName();
							Variables_add(Variable(Header(EntryValue::Ids[type].first), bin, UINT_MAX, keyprefix + itemAttributes[(uint32_t)attributes[j]].GetName(), *SanitizeTagStr(SanitizedAttributeKey)));
						}
					}

					uint32_t size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
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
					{
						if (SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
							::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
						break;
					}

					case IDC_LOCTO:
						if (SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1)
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

BOOL CALLBACK CleanItemProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			for (uint32_t i = 0; i < itemTypes.size(); i++)
			{
				if (EntryExists(itemTypes[i].GetID()) >= 0)
					SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (uint32_t)CB_ADDSTRING, 0, (LPARAM)StringToWString(itemTypes[i].GetName()).c_str());
			}
		}
		case WM_COMMAND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				uint32_t index_what = 0, index = SendMessage(GetDlgItem(hwnd, IDC_SPAWNWHAT), (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				for (uint32_t i = 0; i < itemTypes.size(); i++)
				{
					if (EntryExists(itemTypes[i].GetID()) >= 0)
					{
						if (index_what == index)
							break;
						index_what++;
					}
				}
				for (uint32_t i = 0; i < variables.size(); i++)
				{
					if (StartsWithStr(variables[i].key, itemTypes[index_what].GetName()))
					{
						uint32_t id = ParseItemID(variables[i].key, itemTypes[index_what].GetName().size() + 1);
						if (id != 0)
						{
							std::wstring idStr = std::to_wstring(id);

							std::vector<char> attributes = itemTypes[index_what].GetAttributes(itemAttributes.size());

							for (uint32_t j = 0; j < attributes.size(); j++)
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

LRESULT CALLBACK EditProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	PLID *thisplID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (thisplID == nullptr)
		return FALSE;

	switch (message)
	{
		case WM_KILLFOCUS:
		{
			// If we already executed once, then skip
			if (thisplID->nPID)
				break;

			bool bIgnoreContent = FALSE;
			for (auto& hButton : { GetDlgItem(GetParent(hwnd), IDC_BUTNEGINF),  GetDlgItem(GetParent(hwnd), IDC_BUTPOSINF) })
			{
				if (hButton == (HWND)wParam)
				{
					// The focus got lost when any of the infinity buttons was clicked
					PLID *plID = (PLID *)GetWindowLong(hButton, GWL_USERDATA);
					bIgnoreContent = (plID != nullptr);
					if (bIgnoreContent)
					{
						const float fvalue = plID->nPID ? std::numeric_limits<float>::infinity() : -std::numeric_limits<float>::quiet_NaN();

						if (variables[indextable[iItem].second].header.GetValueType() == EntryValue::Float)
							UpdateValue(_TEXT(""), indextable[iItem].second, FloatToBin(fvalue));
					}
				}
			}
			return CloseEditbox(hwnd, GetDlgItem(hDialog, IDC_OUTPUTBAR2), bIgnoreContent);
		}
		case WM_GETDLGCODE:
		{
			if (wParam == VK_ESCAPE || wParam == VK_RETURN)
			{
				// If we already executed once, then skip
				if (thisplID->nPID)
					break;
				return CloseEditbox(hwnd, GetDlgItem(hDialog, IDC_OUTPUTBAR2));
			}
			break;
		}
		case WM_DESTROY:
		{
			HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, thisplID);
			thisplID = nullptr;
			return TRUE;
		}
	}
	return CallWindowProc(thisplID->hDefProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ComboProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE; 

	switch (message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			return 0;
		}
		case WM_DESTROY:
		{
			hCEdit = NULL;
			HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
			plID = nullptr;
			return TRUE;
		}
	}
	return CallWindowProc(plID->hDefProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ListViewProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
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

// Some controls inside ListCtrl allocated some data on the heap, so we need to clean it up upon destroying
// Namely: inf buttons
LRESULT CALLBACK ListCtrlChildProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	PLID *plID = (PLID *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plID == nullptr)
		return FALSE;

	switch (message)
	{
		case WM_DESTROY:
		{
			HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, plID);
			plID = nullptr;
			return TRUE;
		}
	}
	return CallWindowProc(plID->hDefProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ListCtrlProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_RBUTTONDOWN:
	{
		if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
		if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
		LVHITTESTINFO itemclicked;
		long x, y;
		x = (long)GET_X_LPARAM(lParam);
		y = (long)GET_Y_LPARAM(lParam);
		itemclicked.pt.x = x;
		itemclicked.pt.y = y;
		int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		if (lResult != -1)
		{
			RECT rekt;
			iItem = itemclicked.iItem;
			GetWindowRect(hwnd, &rekt);
			rekt.top += GET_Y_LPARAM(lParam);
			rekt.left += GET_X_LPARAM(lParam);

			PostMessage(hwnd, WM_L2CONTEXTMENU, rekt.top, rekt.left);
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); return 0; };
		if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); return 0; };

		LVHITTESTINFO itemclicked;
		itemclicked.pt.x = GET_X_LPARAM(lParam);
		itemclicked.pt.y = GET_Y_LPARAM(lParam);
		const int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		if (lResult != -1)
		{
			if (itemclicked.iSubItem == 1) { break; }

			RECT rekt;
			std::string value = variables[indextable[itemclicked.iItem].second].value;
			ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_BOUNDS, &rekt);
			const int pos_left = rekt.left;

			ListView_GetSubItemRect(hwnd, itemclicked.iItem, itemclicked.iSubItem, LVIR_LABEL, &rekt);
			const int height = rekt.bottom - rekt.top;
			const int width = rekt.right - pos_left;

			const auto type = variables[indextable[itemclicked.iItem].second].header.GetNonContainerValueType();
			iItem = itemclicked.iItem;

			switch (type)
			{
			case EntryValue::String:
			case EntryValue::Integer:
			case EntryValue::Float:
			{
				const int szButton = 24;
				const int szButtonPadding = 2;
				const int szbuf = 128;

				hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), (LPWSTR)_T(""), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, pos_left, rekt.top, type == EntryValue::Float ? width - (2 * szButton) : width , height, hwnd, 0, hInst, NULL);

				SendMessage(hEdit, WM_SETFONT, (WPARAM)hListFont, TRUE);

				SendMessage(hEdit, EM_SETLIMITTEXT, szbuf, 0);
				std::wstring buffer(szbuf, '\0');

				ListView_GetItemText(hwnd, iItem, 0, &buffer[0], szbuf);
				SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buffer.c_str());

				SetFocus(hEdit);
				SendMessage(hEdit, EM_SETSEL, 0, -1);
				SendMessage(hEdit, EM_SETSEL, -1, 0);

				// Alloc memory on heap to store EditBox's default window process and data
				AllocWindowUserData(hEdit, (LONG)EditProc, 0);

				if (type != EntryValue::Float)
					break;

				// For float, we add two buttons for positve and negative infinity
				const HWND hNegInfButton = CreateWindowEx(0, WC_BUTTON, negInfinity.c_str(), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, (pos_left + width) - (2 * szButton), rekt.top, szButton, height, hwnd, (HMENU)(IDC_BUTNEGINF), hInst, NULL);
				SendMessage(hNegInfButton, WM_SETFONT, (WPARAM)hListFont, TRUE);

				// Alloc memory on heap to store NegInfButton's default window process
				AllocWindowUserData(hNegInfButton, (LONG)ListCtrlChildProc, 0);

				const HWND hPosInfButton = CreateWindowEx(0, WC_BUTTON, posInfinity.c_str(), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, (pos_left + width) - (szButton), rekt.top, szButton, height, hwnd, (HMENU)(IDC_BUTPOSINF), hInst, NULL);
				SendMessage(hPosInfButton, WM_SETFONT, (WPARAM)hListFont, TRUE);

				// Alloc memory on heap to store PosInfButton's default window process
				AllocWindowUserData(hPosInfButton, (LONG)ListCtrlChildProc, 1);

				break;
			}
			case EntryValue::Bool:
			{
				int boolindex = value.c_str()[0];
				hCEdit = CreateWindow(WC_COMBOBOX, (LPWSTR)_T(""), CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, pos_left + 2, rekt.top - 2, 100, height, hwnd, NULL, hInst, NULL);
				AllocWindowUserData(hCEdit, (LONG)ComboProc, 0);

				SendMessage(hCEdit, WM_SETFONT, (WPARAM)hListFont, TRUE);
				SendMessage(hCEdit, (uint32_t)CB_ADDSTRING, (WPARAM)0, (LPARAM)bools[boolindex].c_str());
				SendMessage(hCEdit, (uint32_t)CB_ADDSTRING, (WPARAM)0, (LPARAM)(boolindex == 0 ? bools[1].c_str() : bools[0].c_str()));
				SendMessage(hCEdit, CB_SETCURSEL, 0, 0);
				break;
			}
			case EntryValue::Transform:
			{
				EnableWindow(hDialog, FALSE);
				HWND hTransform = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TRANSFORM), hDialog, TransformProc);
				ShowWindow(hTransform, SW_SHOW);
				break;
			}
			case EntryValue::Color:
			case EntryValue::Vector3:
			{
				EnableWindow(hDialog, FALSE);
				HWND hVector = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_COLOR), hDialog, ColorProc, type == EntryValue::Color);
				ShowWindow(hVector, SW_SHOW);
				break;
			}
			default:
			{
				Header *hdr = &(variables[indextable[itemclicked.iItem].second].header);
				if (!hdr->IsContainer())
					break;

				if (hdr->GetValueType() != EntryValue::String || (hdr->GetContainerType() != EntryContainer::List && hdr->GetContainerType() != EntryContainer::Dictionary))
					break;

				if (hdr->GetContainerKeyType() != EntryValue::Null && hdr->GetContainerKeyType() != EntryValue::String)
					break;

				EnableWindow(hDialog, FALSE);
				HWND hContainer = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONTAINER), hDialog, ContainerEditProc);
				ShowWindow(hContainer, SW_SHOW);
				break;
			}
			}
		}
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		static std::wstring prevTypeStr;
		std::wstring TypeStr = L"";
		LVHITTESTINFO htnfo;
		htnfo.pt.x = (long)GET_X_LPARAM(lParam);
		htnfo.pt.y = (long)GET_Y_LPARAM(lParam);
		const int lResult = ListView_SubItemHitTest(hwnd, &htnfo);
		if (lResult >= 0)
			TypeStr = variables[indextable[lResult].second].GetTypeDisplayString();
		if (TypeStr != prevTypeStr)
		{
			ClearStatic(GetDlgItem(hDialog, IDC_OUTPUT2), hDialog);
			SendMessage(GetDlgItem(hDialog, IDC_OUTPUT2), WM_SETTEXT, 0, (LPARAM)TypeStr.c_str());
			prevTypeStr = TypeStr;
		}
		return TRUE;
	}
	case WM_L2CONTEXTMENU:
	{
		HMENU hPopupMenu = CreatePopupMenu();
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING | ((!variables[indextable[iItem].second].IsModified() && !variables[indextable[iItem].second].IsRemoved()) ? MF_DISABLED : 0), ID_RESET, _T("Reset"));
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING | (variables[indextable[iItem].second].IsRemoved() ? MF_DISABLED : 0), ID_DELETE, _T("Delete"));
#ifdef _MAP
		if (variables[indextable[iItem].second].header.IsNonContainerOfValueType(EntryValue::Transform))
			InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_SHOW_ON_MAP, _T("Show on map"));
#endif
		TrackPopupMenuEx(hPopupMenu, TPM_LEFTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_VERPOSANIMATION, lParam, wParam, hwnd, NULL);
		DestroyMenu(hPopupMenu);

		// Clear selection / focus of listitem (blue highlight)
		RECT rekt;
		GetWindowRect(hwnd, &rekt);
		
		LVHITTESTINFO itemclicked;
		itemclicked.pt.x = lParam - rekt.left;
		itemclicked.pt.y = wParam - rekt.top;
		const int lResult = ListView_SubItemHitTest(hwnd, &itemclicked);
		if (lResult != -1)
		{
			LVITEM lvitem;
			lvitem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
			lvitem.state = (uint32_t)0;
			SendMessage(hwnd, LVM_SETITEMSTATE, iItem, (WPARAM)&lvitem);
		}
		break;
	}
	case WM_SIZE:
	{
		ListView_SetColumnWidth(hwnd, 1, LOWORD(lParam) - SendMessage(hwnd, LVM_GETCOLUMNWIDTH, 0, 0) - GetSystemMetrics(SM_CXVSCROLL));
		return FALSE;
	}
	case WM_COMMAND:
	{
		// handle hcedits messages in here because I'm messy like that 
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			const int ItemIndex = SendMessage((HWND)lParam, (uint32_t)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			const int oSize = SendMessage((HWND)lParam, (uint32_t)CB_GETLBTEXTLEN, (WPARAM)ItemIndex, 0);
			std::wstring value(oSize, '\0');
			SendMessage((HWND)lParam, (uint32_t)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)value.data());
			SendMessage(hCEdit, CB_RESETCONTENT, 0, 0);
			ShowWindow(hCEdit, SW_HIDE);
			SendMessage(hCEdit, WM_CLOSE, 0, 0);
			UpdateValue(value, indextable[iItem].second);
			break;
		}
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_BUTNEGINF || LOWORD(wParam) == IDC_BUTPOSINF)
			{
				if (hEdit != NULL)
				{
					int fuck = 1;
				}
				break;
			}
		}
		switch (LOWORD(wParam))
		{
		case ID_RESET:
		{
			const uint32_t index = indextable[iItem].second;

			variables[index].SetRemoved(FALSE);
			UpdateValue(_T(""), index, variables[index].static_value);
			break;
		}
		case ID_DELETE:
		{
			const uint32_t index = indextable[iItem].second;
			Variables_remove(indextable[iItem].second);
			break;
		}
#ifdef _MAP
		case ID_SHOW_ON_MAP:
		{
			ShowObjectOnMap(&variables[indextable[iItem].second]);
			break;
		}
#endif
		}
		break;
	}
	}
	return CallWindowProc(DefaultListCtrlProc, hwnd, message, wParam, lParam);
}

BOOL CALLBACK ReportProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		const DWORD dwDlgBase = GetDialogBaseUnits();
		const int cxMargin = GET_X_LPARAM(dwDlgBase) / 4;
		const int cyMargin = GET_Y_LPARAM(dwDlgBase) / 8;

		TCITEM tie;
		RECT rcTab;
		HWND hwndButton;
		RECT rcButton;
		RECT rcButtonText;
		int i;

		// Allocate memory for the DLGHDR structure. 
		DLGHDR *pHdr = (DLGHDR *)LocalAlloc(LPTR, sizeof(DLGHDR));

		// Save a pointer to the DLGHDR structure in the window data of the dialog box. 
		SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)pHdr);

		// Create the tab control.
		pHdr->hwndTab = CreateWindowEx(0, WC_TABCONTROL, 0,
			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
			0, 0, 100, 100,
			hwnd, NULL, hInst, NULL
		);
		SendMessage(pHdr->hwndTab, WM_SETFONT, (WPARAM)hListFont, TRUE);

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
			const int iSel = TabCtrl_GetCurSel(pHdr->hwndTab);

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