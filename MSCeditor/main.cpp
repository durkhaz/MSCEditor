#include "main.h"

#undef max //undef windows max() macro to use numeric_limits<streamsize>::max()

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	ResizeDialogInitialize(hInst);
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);
} 

BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	ResizeDialogProc(hwnd, Message, wParam, lParam, &pResizeState);
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			//init common controls
			INITCOMMONCONTROLSEX iccex;
			iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			iccex.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;

			if (!InitCommonControlsEx(&iccex))
			{
				MessageBox(hwnd, _T("Could not initialize common controls!"), _T("Error"), MB_OK | MB_ICONERROR);
				EndDialog(hwnd, 0);
			}

			hDialog = hwnd;

			ListView_SetBkColor(GetDlgItem(hwnd, IDC_List), (COLORREF)GetSysColor(COLOR_MENU));
			ListView_SetBkColor(GetDlgItem(hwnd, IDC_List2), (COLORREF)GetSysColor(COLOR_MENU));

			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_List), LVS_EX_FULLROWSELECT);
			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_List), LVS_EX_DOUBLEBUFFER);
			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_List), LVS_EX_BORDERSELECT);
			
			DefaultListCtrlProc = (WNDPROC)SetWindowLong(GetDlgItem(hwnd, IDC_List2), GWL_WNDPROC, (LONG)ListCtrlProc);
			DefaultListViewProc = (WNDPROC)SetWindowLong(GetDlgItem(hwnd, IDC_List), GWL_WNDPROC, (LONG)ListViewProc);

			HICON hicon = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);

			SetWindowText(hDialog, (LPCWSTR)Title);

			//load data.txt
			using namespace std;

			ifstream inf("data.txt", ifstream::in);
			if (inf.is_open())
			{
				string strInput(2, 0);
				while (!ContainsStr(StringToWString(strInput), _T("Locations")) && !inf.eof())
				{
					inf.ignore(numeric_limits<streamsize>::max(), '#');
					getline(inf, strInput);
				}
				while (inf)
				{
					getline(inf, strInput);
					if (inf.fail()) break;
					if (strInput[0] == '#') break;
					if (strInput.substr(0, 2) != "//")
					{
						string::size_type startpos = 0, endpos = 0;
						if (FetchDataFileParameters(strInput, startpos, endpos))
						{
							wstring name = StringToWString(strInput.substr(startpos, endpos));
							unsigned int _i = startpos + endpos + 1;

							if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos))
							{
								wstring value = StringToWString(strInput.substr(_i + startpos, endpos));
								string bin;
								unsigned int error = 0;
								error += VectorStrToBin(value, 3, bin);
								error += VectorStrToBin(wstring(_T("0,0,0,1")), 4, bin);
								error += VectorStrToBin(wstring(_T("1,1,1")), 3, bin);
								locations.push_back(TextLookup(name, StringToWString(bin)));
							}
						}
					}
					if (inf.eof()) break;
				}
				strInput.clear();
				inf.clear();
				inf.seekg(0, inf.beg);

				while (!ContainsStr(StringToWString(strInput),_T("Report_Identifiers")) && !inf.eof())
				{
					inf.ignore(numeric_limits<streamsize>::max(), '#');
					getline(inf, strInput);
				}
				while (inf)
				{
					getline(inf, strInput);
					if (inf.fail()) break;
					if (strInput[0] == '#') break;
					if (strInput.substr(0, 2) != "//")
					{
						string::size_type startpos = 0, endpos = 0;
						if (FetchDataFileParameters(strInput, startpos, endpos))
						{
							wstring name = StringToWString(strInput.substr(startpos, endpos));
							partIdentifiers.push_back(name);
						}
					}
					if (inf.eof()) break;
				}
				strInput.clear();
				inf.clear();
				inf.seekg(0, inf.beg);

				while (!ContainsStr(StringToWString(strInput), _T("Report_Special")) && !inf.eof())
				{
					inf.ignore(numeric_limits<streamsize>::max(), '#');
					getline(inf, strInput);
				}
				while (inf)
				{
					getline(inf, strInput);
					if (inf.fail()) break;
					if (strInput[0] == '#') break;
					if (strInput.substr(0, 2) != "//")
					{
						string::size_type startpos = 0, endpos = 0;
						if (FetchDataFileParameters(strInput, startpos, endpos))
						{
							wstring str = StringToWString(strInput.substr(startpos, endpos));
							unsigned int _i = startpos + endpos + 1;

							if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos))
							{
								unsigned int id = static_cast<int>(::strtol(strInput.substr(_i + startpos, endpos).c_str(), NULL, 10));
								_i += startpos + endpos + 1;
								wstring param = _T("");
								if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos))
								{
									param = StringToWString(strInput.substr(_i + startpos, endpos));
								}
								partSCs.push_back(SC(str, id, param));
							}
						}
					}
					if (inf.eof()) break;
				}
				inf.clear();
				inf.seekg(0, inf.beg);
				inf.close();
			}
			
			break;
		}
		case WM_NOTIFY:
		{
			// color modified values on List2
			if (((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwnd, IDC_List2) && ((LPNMHDR)lParam)->code == NM_CUSTOMDRAW) 
			{
				int result = CDRF_DODEFAULT;
				LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
				switch (lplvcd->nmcd.dwDrawStage) 
				{
					case CDDS_PREPAINT:
						result = CDRF_NOTIFYITEMDRAW;
						break;
					case CDDS_ITEMPREPAINT:
						if (variables[indextable[lplvcd->nmcd.dwItemSpec].index2].modified) lplvcd->clrText = RGB(255, 0, 0);
						result = CDRF_NEWFONT;
						break;
				}
				SetWindowLongPtr(hDialog, DWLP_MSGRESULT, result);
				return TRUE;
			}

			// color modified entries on List1
			if (((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwnd, IDC_List) && ((LPNMHDR)lParam)->code == NM_CUSTOMDRAW)
			{
				int result = CDRF_DODEFAULT;
				LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
				switch (lplvcd->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT:
						result = CDRF_NOTIFYITEMDRAW;
						break;
					case CDDS_ITEMPREPAINT:

						LVITEM lvi;
						lvi.mask = LVIF_PARAM;
						lvi.iItem = lplvcd->nmcd.dwItemSpec;
						lvi.iSubItem = 0;
						ListView_GetItem(GetDlgItem(hwnd, IDC_List), (LPARAM)&lvi);
						if (lvi.lParam >= 10000) lplvcd->clrText = RGB(255, 0, 0);

						result = CDRF_NEWFONT;
						break;
				}
				SetWindowLongPtr(hDialog, DWLP_MSGRESULT, result);
				return TRUE;
			}

			//update List2 depending on List1 selection
			if (((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwnd, IDC_List) && ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED)
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				HWND hList1 = GetDlgItem(hwnd, IDC_List);
				HWND hList2 = GetDlgItem(hwnd, IDC_List2);

				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iItem = pnmv->iItem;
				lvi.iSubItem = 0;
				ListView_GetItem(hList1, (LPARAM)&lvi);

				int group = lvi.lParam;
				if (group >= 10000) group += -10000;
				SendMessage(hList2, LVM_DELETEALLITEMS, 0, 0);
				indextable.clear();
				int item = 0;
				for (unsigned int i = 0; i < variables.size(); i++)
				{
					if (variables[i].group == group)
					{
						LVITEM lvi;
						std::wstring value;
						FormatString(value, variables[i].value, variables[i].type);

						lvi.mask = LVIF_TEXT | LVIF_STATE; lvi.state = 0; lvi.stateMask = 0;
						lvi.iItem = item; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)value.c_str();
						int uindex = SendMessage(hList2, LVM_INSERTITEM, 0, (LPARAM)&lvi);

						indextable.push_back(IndexLookup(uindex, i));

						lvi.iItem = item; lvi.iSubItem = 1; lvi.pszText = (LPWSTR)variables[i].key.c_str();
						SendMessage(hList2, LVM_SETITEM, 0, (LPARAM)&lvi);
						item++;
					}
				}
				return 0;
			}
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case ID_FILE_OPEN:
					if (CanClose()) OpenFileDialog();
					break;
				case ID_FILE_EXIT:
					if (CanClose()) EndDialog(hDialog, 0);
					break;
				case ID_FILE_CLOSE:
					if (CanClose()) UnloadFile();
					break;
				case ID_FILE_EXPLORER:
				{
					size_t found = filepath.find_last_of(_T('\\'));
					if (found != std::string::npos)
					{
						ShellExecute(NULL, _T("open"), filepath.substr(0, found).c_str(), NULL, NULL, SW_SHOWDEFAULT);
					}
					break;
				}
				case ID_FILE_GAME:
				{
					std::wstring steampath = ReadRegistry(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"SteamPath");
		
					if (steampath.empty()) break;
					std::wstring gamepath = L"\\steamapps\\common\\My Summer Car\\mysummercar.exe";
					//try default path
					if ((int)ShellExecute(NULL, _T("open"), (steampath + gamepath).c_str(), NULL, NULL, SW_SHOWDEFAULT) > 32) break;

					//user is a fancypants and stores library on a different drive. Read configfile
					steampath += L"\\config\\config.vdf";

					using namespace std;

					vector<wstring> basefolders;
					wifstream inf(steampath, wifstream::in);

					if (!inf.is_open()) break;
					while (inf)
					{
						wstring strInput;
						getline(inf, strInput);
						string::size_type startpos = 0, endpos = 0;
						if (FetchDataFileParameters(strInput, startpos, endpos))
						{
							wstring key = StringToWString(strInput.substr(startpos, endpos));
							if (ContainsStr(key, L"BaseInstallFolder"))
							{
								unsigned int _i = startpos + endpos + 1;

								if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos))
								{
									wstring value = StringToWString(strInput.substr(_i + startpos, endpos));
									basefolders.push_back(value);
								}
							}
						}
					}
						
					for (unsigned int i = 0; i < basefolders.size(); ++i)
					{
						if ((int)ShellExecute(NULL, _T("open"), (basefolders[i] + gamepath).c_str(), NULL, NULL, SW_SHOWDEFAULT) > 32) break;
					}
					break;
				}

				case ID_OPTIONS_MAKEBACKUP:
				{
					MakeBackup = (MF_CHECKED == (MakeBackup ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_CHECKED)) ? FALSE : TRUE);
					break;	
				}
				case ID_OPTIONS_USEEULERANGLES:
				{
					EulerAngles = (MF_CHECKED == (EulerAngles ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_CHECKED)) ? FALSE : TRUE);
					break;
				}
				case ID_FILE_SAVE:
				{
					int result = SaveFile();
					if (result > 0)
					{
						MessageBox(hDialog, (GLOB_STRS[12] + GLOB_STRS[12 + result]).c_str(), _T("Error"), MB_OK | MB_ICONERROR);
					}
					else
					{
						InitMainDialog(hDialog);
					}
					break;
				}
				case ID__HELP:
				{
					MessageBox(hDialog, HelpStr, Title, MB_OK | MB_ICONINFORMATION);
					break;
				}
				case ID__ABOUT:
				{
					HWND hAbout = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ABOUT), hDialog, AboutProc);
					ShowWindow(hAbout, SW_SHOW);
					break;
				}
				case ID_TOOLS_TELEPORTENTITY:
				{
					if (!IsWindow(hTeleport))
					{
						EnableWindow(hDialog, FALSE);
						hTeleport = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TELEPORT), hDialog, TeleportProc);
						ShowWindow(hTeleport, SW_SHOW);
					}
					break;
				}
				case ID_TOOLS_REPORT:
				{
					if (!IsWindow(hReport))
					{
						EnableWindow(hDialog, FALSE);
						hReport = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BOLTS), hDialog, ReportProc);
						ShowWindow(hReport, SW_SHOW);
					}
					break;
				}
			}
			
			// FILTER
			if ((HIWORD(wParam) == EN_UPDATE) && (LOWORD(wParam) == IDC_FILTER))
			{
				unsigned int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
				std::wstring str(size, '\0');
				GetWindowText(GetDlgItem(hwnd, IDC_FILTER), (LPWSTR)str.data(), size);
				str.resize(size - 1);

				UpdateList(str);

				break;
			}
		} 

		case WM_SIZE:
			ClearStatic(GetDlgItem(hwnd, IDC_STXT1), hDialog);
			ClearStatic(GetDlgItem(hwnd, IDC_STXT2), hDialog);
			break;

		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == GetDlgItem(hDialog, IDC_OUTPUT3))
			{
				SetBkMode((HDC)wParam, COLOR_WINDOW);
				WasModified() ? SetTextColor((HDC)wParam, RGB(255, 0, 0)): SetTextColor((HDC)wParam, RGB(0, 0, 0));
				return (BOOL)GetSysColorBrush(COLOR_WINDOW);
			}
			else
			{
				SetBkMode((HDC)wParam, COLOR_WINDOW);
				return (BOOL)GetSysColorBrush(COLOR_WINDOW);
			}
			break;

		case WM_LBUTTONDOWN:
		{
			if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
			if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
			break;
		}

		case WM_CLOSE:
			if (CanClose()) EndDialog(hDialog, 0);

			break;

		default:
			return FALSE;
	}
	return TRUE;
}