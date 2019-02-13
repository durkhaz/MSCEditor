#include "main.h"
#ifdef _MAP
#include "map.h"
#endif /*_MAP*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	hInst = hInstance;
	ResizeDialogInitialize(hInst);
	return static_cast<int>(DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc, (LPARAM)pCmdLine));
} 

INT_PTR CALLBACK DlgProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	ResizeDialogProc(hwnd, Message, wParam, lParam, &pResizeState);
	switch (Message)
	{
		case WM_INITDIALOG:
		{
#ifdef _DEBUG
			// Alloc debug console
			AllocConsole();
			std::wstring cTitle = Title + _T(" Debug Console");
			SetConsoleTitle(cTitle.c_str());

			// Get STDOUT handle
			HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
			FILE *COutputHandle = _fdopen(SystemOutput, "w");

			// Get STDERR handle
			HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
			int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
			FILE *CErrorHandle = _fdopen(SystemError, "w");

			// Get STDIN handle
			HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
			int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
			FILE *CInputHandle = _fdopen(SystemInput, "r");

			//make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
			std::ios::sync_with_stdio(true);

			// Redirect the CRT standard input, output, and error handles to the console
			freopen_s(&CInputHandle, "CONIN$", "r", stdin);
			freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
			freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

			// Move console over to other screen and maximize, if possible
			HWND hConsole = GetConsoleWindow();
			SetWindowPos(hConsole, NULL, 3000, 0, 200, 200, SWP_NOSIZE);
			ShowWindow(hConsole, SW_MAXIMIZE);
			
			std::wcout << cTitle << _T(" successfully allocated.") << std::endl;

			// Log
			dbglog = new DebugOutput(L"log.txt");
			dbglog->LogNoConsole(L"~~~\n\nNEW LOG INSTANCE\n\n~~~\n");
#endif

			//init common controls
			INITCOMMONCONTROLSEX iccex;
			iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			iccex.dwICC = ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;

			if (!InitCommonControlsEx(&iccex))
			{
				MessageBox(hwnd, _T("Could not initialize common controls!"), Title.c_str(), MB_OK | MB_ICONERROR);
				EndDialog(hwnd, 0);
			}

			hListFont = (HFONT)SendMessage(GetDlgItem(hwnd, IDC_List), WM_GETFONT, 0, 0);
		 
			hDialog = hwnd;

			ListView_SetBkColor(GetDlgItem(hwnd, IDC_List), (COLORREF)GetSysColor(COLOR_MENU));
			ListView_SetBkColor(GetDlgItem(hwnd, IDC_List2), (COLORREF)GetSysColor(COLOR_MENU));

			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_List), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_BORDERSELECT);
			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_List2), LVS_EX_GRIDLINES);
			
			DefaultListCtrlProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_List2), GWLP_WNDPROC, (LONG_PTR)ListCtrlProc);
			DefaultListViewProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_List), GWLP_WNDPROC, (LONG_PTR)ListViewProc);

			// Set Icon and title
			HICON hicon = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);
			SetWindowText(hDialog, (LPCWSTR)Title.c_str());

			// Prepare and set app folder
			if (FAILED(FindAndCreateAppFolder()))
			{
				std::wstring buffer(128, '\0');
				swprintf(&buffer[0], 128, GLOB_STRS[37].c_str(), appfolderpath.c_str());
				MessageBox(hwnd, buffer.c_str(), Title.c_str(), MB_OK);
			}

			//load inifile
			if (LoadDataFile(IniFile))
				MessageBox(hwnd, GLOB_STRS[35].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
			else
				if (bFirstStartup)
				{
					HWND hHelp = CreateDialog(hInst, MAKEINTRESOURCE(IDD_HELP), hDialog, HelpProc);
					ShowWindow(hHelp, SW_SHOW);
				}

			//check updates
			if (bCheckForUpdate && !appfolderpath.empty())
			{
				std::wstring path; 
#ifdef _DEBUG
				if (!DownloadUpdatefile(L"", path))
#else
				if (!DownloadUpdatefile(L"http://mscedit.superskalar.org/current", path))
#endif
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
		
			// Apply settings
			if (!bMakeBackup)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_UNCHECKED);
			if (!bCheckForUpdate)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_UNCHECKED);
			if (!bEulerAngles)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 2), MF_UNCHECKED);
			if (!bDisplayRawNames)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 3), MF_UNCHECKED);
			if (!bCheckIssues)
				CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 4), MF_UNCHECKED);

#ifdef _MAP
			HMENU menu = GetSubMenu(GetMenu(hDialog), 1);
			EnableMenuItem(menu, GetMenuItemID(menu, 8), MF_ENABLED);
#endif /*_MAP*/
			// If we got a path from the command line, try to open
			// Make sure this is at the end of InitDialog because it contains breaks
			std::wstring InitFile = std::wstring((LPWSTR)lParam);
			if (!InitFile.empty())
			{
				size_t found = InitFile.find_last_of('\\');
				if (found != std::string::npos)
				{
					// Check if we have an actual file at our hands
					HANDLE hTest = CreateFile(InitFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
					if (hTest == INVALID_HANDLE_VALUE)
						break;

					LARGE_INTEGER fSize;
					if (!GetFileSizeEx(hTest, &fSize))
						break;

					CloseHandle(hTest);
					filename = InitFile.substr(found + 1);
					filepath = InitFile;
					InitMainDialog(hDialog);
				}
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
					{
						size_t index = lplvcd->nmcd.dwItemSpec > indextable.size() - 1 ? indextable.size() - 1 : lplvcd->nmcd.dwItemSpec;
						if (variables[indextable[index].second].IsRemoved())
						lplvcd->clrText = RGB(128, 128, 128);
						else if (variables[indextable[index].second].IsModified() || variables[indextable[index].second].IsAdded())
						lplvcd->clrText = RGB(255, 0, 0);
						result = CDRF_NEWFONT;
						break;
					}
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
					{
						LVITEM lvi;
						lvi.mask = LVIF_PARAM;
						lvi.iItem = static_cast<int>(lplvcd->nmcd.dwItemSpec);
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
				uint32_t group = (uint32_t)listparam->GetIndex();
				SendMessage(hList2, LVM_DELETEALLITEMS, 0, 0);
				indextable.clear();
				int item = 0;
				for (uint32_t i = 0; i < variables.size(); i++)
				{
					if (variables[i].group == group)
					{
						LVITEM lvi;
						std::wstring value = variables[i].GetDisplayString();

						lvi.mask = LVIF_TEXT | LVIF_STATE; lvi.state = 0; lvi.stateMask = 0;
						lvi.iItem = item; lvi.iSubItem = 0; lvi.pszText = (LPWSTR)value.c_str();
						auto uindex = static_cast<int32_t>(SendMessage(hList2, LVM_INSERTITEM, 0, (LPARAM)&lvi));

						indextable.push_back(std::pair<uint32_t, uint32_t>(uindex, i));

						lvi.iItem = item; lvi.iSubItem = 1; lvi.pszText = (LPWSTR)(bDisplayRawNames ? variables[i].raw_key : variables[i].key).c_str();
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
					if (reinterpret_cast<INT_PTR>(ShellExecute(NULL, _T("open"), (steampath + gamepath).c_str(), NULL, NULL, SW_SHOWDEFAULT)) > 32) break;

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
								size_t _i = endpos + 1;

								if (FetchDataFileParameters(strInput.substr(_i), startpos, endpos) == 0)
								{
									wstring value = strInput.substr(_i + startpos, endpos - startpos);
									basefolders.push_back(value);
								}
							}
						}
					}
						
					for (uint32_t i = 0; i < basefolders.size(); ++i)
					{
						if (reinterpret_cast<INT_PTR>(ShellExecute(NULL, _T("open"), (basefolders[i] + gamepath).c_str(), NULL, NULL, SW_SHOWDEFAULT)) > 32) break;
					}
					break;
				}
				case ID_OPTIONS_MAKEBACKUP:
				{
					if (bMakeBackup)
					{
						CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_UNCHECKED);
						if (!bBackupChangeNotified)
						{
							MessageBox(hDialog, GLOB_STRS[41].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
							bBackupChangeNotified = TRUE;
						}
					}
					else
						CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 0), MF_CHECKED);
					bMakeBackup = !bMakeBackup;
					break;	
				}
				case ID_OPTIONS_CHECKFORUPDATES:
				{
					bCheckForUpdate = MF_CHECKED != (bCheckForUpdate ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 1), MF_CHECKED));
					break;
				}
				case ID_OPTIONS_USEEULERANGLES:
				{
					bEulerAngles = MF_CHECKED != (bEulerAngles ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 2), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 2), MF_CHECKED));
					break;
				}
				case ID_OPTIONS_DISPLAYRAWNAMES:
				{
					bDisplayRawNames = MF_CHECKED != (bDisplayRawNames ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 3), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 3), MF_CHECKED));
					auto index = static_cast<int32_t>(SendMessage(GetDlgItem(hwnd, IDC_List), LVM_GETSELECTIONMARK, 0, 0));
					if (index < 0)
						break;
					NMHDR nmhdr = { GetDlgItem(hwnd, IDC_List), IDC_List, LVN_ITEMCHANGED };
					NMLISTVIEW nmlv = { nmhdr , index , 0 , 0 , 0 , 0 , 0 , 0 };
					SendMessage(hwnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmlv);
					break;
				}
				case ID_OPTIONS_CHECKISSUESWHENSAVING:
				{
					bCheckIssues = MF_CHECKED != (bCheckIssues ? CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 4), MF_UNCHECKED) : CheckMenuItem(GetSubMenu(GetMenu(hDialog), 2), GetMenuItemID(GetSubMenu(GetMenu(hDialog), 2), 4), MF_CHECKED));
					break;
				}
				case ID_FILE_SAVE:
				{
					std::vector<Issue> issues;
					PopulateCarparts();
					if (bCheckIssues && SaveHasIssues(issues))
					{
						std::wstring buffer(256, '\0');
						swprintf(&buffer[0], 256, GLOB_STRS[38].c_str(), issues.size());
						if (MessageBox(hDialog, buffer.c_str(), ErrorTitle.c_str(), MB_YESNO | MB_ICONWARNING) == IDYES)
						{
							SendMessage(hDialog, WM_COMMAND, MAKEWPARAM(ID_TOOLS_ISSUEDIAGNOSTICS, 0), reinterpret_cast<LPARAM>(&issues));
							break;
						}
					}
					int result = SaveFile();
					if (result > 0)
						MessageBox(hDialog, (GLOB_STRS[12] + GLOB_STRS[12 + result]).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
					else
						InitMainDialog(hDialog);
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
					EnableWindow(hDialog, FALSE);
					HWND hTeleport = lParam == 0 ? CreateDialog(hInst, MAKEINTRESOURCE(IDD_TELEPORT), hDialog, TeleportProc) : CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_TELEPORT), hwnd, TeleportProc, lParam);
					ShowWindow(hTeleport, SW_SHOW);
					break;
				}
				case ID_TOOLS_REPORT:
				{
					if (!IsWindow(hReport))
					{
						EnableWindow(hDialog, FALSE);
						hReport = CreateDialog(hInst, MAKEINTRESOURCE(IDD_REPORT), hDialog, ReportProc);
						ShowWindow(hReport, SW_SHOW);
					}
					break;
				}
				case ID_TOOLS_ISSUEDIAGNOSTICS:
				{
					bool bHasIssues = TRUE;
					std::vector<Issue> issues;
					if (lParam == 0)
					{
						PopulateCarparts();
						bHasIssues = SaveHasIssues(issues);
					}
					if (bHasIssues)
					{
						EnableWindow(hwnd, FALSE);
						HWND hIssues = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_KEYS), hDialog, IssueProc, lParam == 0 ? reinterpret_cast<LPARAM>(&issues) : lParam);
						ShowWindow(hIssues, SW_SHOW);
					}
					else
					{
						std::wstring buffer(128, '\0');
						swprintf(&buffer[0], 128, GLOB_STRS[50].c_str(), 0, GLOB_STRS[51].c_str());
						MessageBox(hwnd, &buffer[0], Title.c_str(), MB_OK);
					}
					break;
				}
				case ID_TOOLS_DUMP:
				{
					if (variables.size() == 0) break;
					std::wofstream dump(L"entry_dump.txt", std::wofstream::out, std::wofstream::trunc);
					if (!dump.is_open()) MessageBox(hwnd, GLOB_STRS[30].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);


					for (uint32_t i = 0; i < variables.size(); i++)
					{
						std::wstring str = variables[i].GetDisplayString();
						dump << (L"\"" + StringToWString(variables[i].key) + L"\" \"" + str + L"\"\n");
					}

					if (dump.bad() || dump.fail())
					{
						MessageBox(hwnd, GLOB_STRS[30].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
					}
					else
					{
						//DebugFetchVariablesFromAssets();
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
				case ID_TOOLS_MANAGEKEYS:
				{
					EnableWindow(hDialog, FALSE);
					HWND hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_KEYS), hDialog, KeyManagerProc);
					ShowWindow(hWnd, SW_SHOW);
					break;
				}
				case ID_TOOLS_TIMEANDWEATHER:
				{
					EnableWindow(hDialog, FALSE);
					HWND hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TIMEWEATHER), hDialog, TimeWeatherProc);
					ShowWindow(hWnd, SW_SHOW);
					break;
				}
#ifdef _MAP
				case ID_TOOLS_WORLDMAP:
				{
					OpenMap();
					break;
				}
#endif /*_MAP*/
			}
			
			// FILTER
			if ((HIWORD(wParam) == EN_UPDATE) && (LOWORD(wParam) == IDC_FILTER))
			{
				uint32_t size = GetWindowTextLength(GetDlgItem(hwnd, IDC_FILTER)) + 1;
				std::wstring str(size, '\0');
				GetWindowText(GetDlgItem(hwnd, IDC_FILTER), (LPWSTR)str.data(), size);
				str.resize(size - 1);

				UpdateList(str);

				break;
			}
		} 
		case WM_SIZE:
		{
			ClearStatic(GetDlgItem(hwnd, IDC_STXT1), hDialog);
			ClearStatic(GetDlgItem(hwnd, IDC_STXT2), hDialog);
			break;
		}
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == GetDlgItem(hDialog, IDC_OUTPUT3))
			{
				SetBkMode((HDC)wParam, COLOR_WINDOW);
				WasModified() ? SetTextColor((HDC)wParam, RGB(255, 0, 0)): SetTextColor((HDC)wParam, RGB(0, 0, 0));
				return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
			}
			else if ((HWND)lParam == GetDlgItem(hDialog, IDC_OUTPUT4))
			{
				SetBkMode((HDC)wParam, COLOR_WINDOW);
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
				return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
			}
			else
			{
				SetBkMode((HDC)wParam, COLOR_WINDOW);
				return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
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
				DeleteObject(hListFont);
#ifdef _DEBUG
				delete dbglog;
#endif /*_DEBUG*/
#ifdef _MAP
				if (EditorMap)
					DestroyWindow(EditorMap->m_hwnd);
#endif /*_MAP*/

				EndDialog(hDialog, 0);
				return FALSE;
			}
			

		default:
			return FALSE;
	}
	return TRUE;
}