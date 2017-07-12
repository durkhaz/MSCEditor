#include "main.h"

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

			LOGFONT lf;
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
			hFont = CreateFont(lf.lfHeight, lf.lfWidth,
				lf.lfEscapement, lf.lfOrientation, lf.lfWeight,
				lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut, lf.lfCharSet,
				lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality,
				lf.lfPitchAndFamily, lf.lfFaceName);

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

			SetWindowText(hDialog, (LPCWSTR)Title.c_str());

			//load inifile
			if (LoadDataFile(IniFile))
				MessageBox(hwnd, GLOB_STRS[35].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
			else
				if (first_startup)
				{
					HWND hHelp = CreateDialog(hInst, MAKEINTRESOURCE(IDD_HELP), hDialog, HelpProc);
					ShowWindow(hHelp, SW_SHOW);
				}

			//check updates
			if (CheckForUpdate)
			{
				std::wstring path; 
				if (!DownloadUpdatefile(L"http://mscedit.superskalar.org/current", path))
				{
					std::wstring link, changelog;
					if (CheckUpdate(path, link, changelog))
					{
						switch (MessageBox(NULL, (GLOB_STRS[36] + changelog).c_str(), Title.c_str(), MB_YESNO | MB_ICONINFORMATION))
						{
						case IDYES:
							ShellExecute(NULL, _T("open"), link.c_str(), NULL, NULL, SW_SHOWDEFAULT);
							EndDialog(hDialog, 0);
							break;
						default:
							break;
						}
					}
				}
			}
		
			//apply settings
			if (!MakeBackup)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_UNCHECKED);
			if (!CheckForUpdate)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_UNCHECKED);

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
						if (variables[indextable[lplvcd->nmcd.dwItemSpec].index2].IsRemoved())
							lplvcd->clrText = RGB(128, 128, 128);
						else if (variables[indextable[lplvcd->nmcd.dwItemSpec].index2].IsModified() || variables[indextable[lplvcd->nmcd.dwItemSpec].index2].IsAdded())
							lplvcd->clrText = RGB(255, 0, 0);

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

						ListParam* listparam = (ListParam*)lvi.lParam;
						if (listparam->GetFlag(VAR_REMOVED))
							lplvcd->clrText = RGB(128, 128, 128);
						else if (listparam->GetFlag(VAR_MODIFIED)) 
							lplvcd->clrText = RGB(255, 0, 0);

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

				ListParam* listparam = (ListParam*)lvi.lParam;
				int group = listparam->GetIndex();
				SendMessage(hList2, LVM_DELETEALLITEMS, 0, 0);
				indextable.clear();
				int item = 0;
				for (UINT i = 0; i < variables.size(); i++)
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

		case WM_ACTIVATE:
		{
			static bool busy = false;
			if (!busy)
			{
				busy = TRUE;
				busy = FileChanged();
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
					if (CanClose())
					{
						SaveSettings(IniFile);
						EndDialog(hDialog, 0);
					}
					break;
				case ID_FILE_CLOSE:
					if (CanClose()) UnloadFile();
					break;
				case ID_FILE_EXPLORER:
				{
					std::wstring folderpath = filepath;
					folderpath.resize(filepath.size() - filename.size());
					ShellExecute(NULL, _T("open"), folderpath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					break;
				}
				case ID_FILE_GAMESTEAM:
				{
					ShellExecute(NULL, NULL, L"steam://run/516750", NULL, NULL, SW_SHOW);
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
						if (FetchDataFileParameters(strInput, startpos, endpos) == 0)
						{
							wstring key = strInput.substr(startpos, endpos - startpos);
							if (ContainsStr(key, L"BaseInstallFolder"))
							{
								UINT _i = endpos + 1;

								if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos) == 0)
								{
									wstring value = strInput.substr(_i + startpos, endpos - startpos);
									basefolders.push_back(value);
								}
							}
						}
					}
						
					for (UINT i = 0; i < basefolders.size(); ++i)
					{
						if ((int)ShellExecute(NULL, _T("open"), (basefolders[i] + gamepath).c_str(), NULL, NULL, SW_SHOWDEFAULT) > 32) break;
					}
					break;
				}

				case ID_OPTIONS_MAKEBACKUP:
				{
					if (MakeBackup)
					{
						CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_UNCHECKED);
						if (!backup_change_notified)
						{
							MessageBox(hDialog, GLOB_STRS[41].c_str(), Title.c_str(), MB_OK | MB_ICONERROR);
							backup_change_notified = TRUE;
						}
					}
					else
						CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_CHECKED);
					MakeBackup = !MakeBackup;
					break;	
				}
				case ID_OPTIONS_CHECKFORUPDATES:
				{
					CheckForUpdate = (MF_CHECKED == (CheckForUpdate ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_CHECKED)) ? FALSE : TRUE);
					break;
				}
				case ID_OPTIONS_USEEULERANGLES:
				{
					EulerAngles = (MF_CHECKED == (EulerAngles ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 2), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 2), MF_CHECKED)) ? FALSE : TRUE);
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
					HWND hHelp = CreateDialog(hInst, MAKEINTRESOURCE(IDD_HELP), hDialog, HelpProc);
					ShowWindow(hHelp, SW_SHOW);
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
				case ID_TOOLS_DUMP:
				{
					if (variables.size() == 0) break;
					std::wofstream dump(L"entry_dump.txt", std::wofstream::out, std::wofstream::trunc);
					if (!dump.is_open()) MessageBox(hwnd, GLOB_STRS[30].c_str(), _T("Error"), MB_OK | MB_ICONERROR);

					for (UINT i = 0; i < variables.size(); i++)
					{
						std::wstring str;
						FormatString(str, variables[i].value, variables[i].type);
						dump << (L"\"" + variables[i].key + L"\" \"" + str + L"\"\n");
					}

					if (dump.bad() || dump.fail())
					{
						MessageBox(hwnd, GLOB_STRS[30].c_str(), Title.c_str(), MB_OK | MB_ICONERROR);
					}
					else
					{
						MessageBox(hwnd, GLOB_STRS[40].c_str(), Title.c_str(), MB_OK );
					}
					dump.close();
					break;
				}
				case ID_ITEMS_SPAWNITEMS:
				{
					EnableWindow(hDialog, FALSE);
					HWND hSpawnitems = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SPAWNITEM), hDialog, SpawnItemProc);
					ShowWindow(hSpawnitems, SW_SHOW);
					break;
				}
			}
			
			// FILTER
			if ((HIWORD(wParam) == EN_UPDATE) && (LOWORD(wParam) == IDC_FILTER))
			{
				UINT size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
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

		case WM_INITMENUPOPUP:
		case WM_LBUTTONDOWN:
		{
			if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
			if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
			break;
		}

		case WM_CLOSE:
			if (CanClose())
			{
				SaveSettings(IniFile);
				EndDialog(hDialog, 0);
			}
			break;

		default:
			return FALSE;
	}
	return TRUE;
}