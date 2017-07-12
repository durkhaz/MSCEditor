#include "dlgprocs.h"

#include <CommCtrl.h>
#define        WM_L2CONTEXTMENU     (WM_USER + 10)
#define        WM_DRAWINFOICON     (WM_USER + 11)

// ======================
// dlg utils
// ======================

bool CloseEditbox(HWND hwnd)
{
	int size = GetWindowTextLength(hwnd) + 1;
	std::wstring value(size - 1, '\0');
	GetWindowText(hwnd, (LPWSTR)value.data(), size);

	switch (variables[indextable[iItem].index2].type)
	{
	case ID_FLOAT:
	{
		if (!IsValidFloatStr(value))
		{
			EDITBALLOONTIP ebt;
			ebt.cbStruct = sizeof(EDITBALLOONTIP);
			ebt.pszTitle = _T("Oops!");
			ebt.ttiIcon = TTI_ERROR_LARGE;
			ebt.pszText = GLOB_STRS[8].c_str();
			SendMessage(GetDlgItem(hDialog, IDC_OUTPUTBAR2), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
			break;
		}
		TruncFloatStr(value);
		break;
	}
	}
	UpdateValue(value, indextable[iItem].index2);
	DestroyWindow(hwnd);
	hEdit = NULL;
	return TRUE;
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
			if (first_startup)
				SendMessage(GetDlgItem(hwnd, IDC_HELPNEW), WM_SETTEXT, 0, (LPARAM)GLOB_STRS[42].c_str());
			
			PostMessage(hwnd, WM_DRAWINFOICON, 0, 0);
			MessageBeep(MB_ICONASTERISK);
			first_startup = FALSE;
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

BOOL CALLBACK ReportProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		if (partIdentifiers.size() < 5)
		{
			MessageBox(hDialog, GLOB_STRS[26].c_str(), _T("Error"), MB_OK | MB_ICONERROR);
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}

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
				if (ContainsStr(variables[i].key, partIdentifiers[3])) //contains installed?
				{
					prefix = variables[i].key;
					prefix.resize(prefix.size() - partIdentifiers[3].size());
				}
				else if (ContainsStr(variables[i].key, partIdentifiers[1]))
				{
					bool valid = TRUE;
					prefix = variables[i].key;
					prefix.resize(prefix.size() - partIdentifiers[1].size());

					std::wstring _str = prefix + partIdentifiers[3];
					for (UINT j = 0; j < variables.size(); j++)
					{
						if (variables[j].key == _str)
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
						}
						part.name = prefix;

						carparts.push_back(part);

						PopulateBList(hwnd, &part, item, &ov);
					}
				}
			}
		}
		//draw list
		SendMessage(hList3, WM_SETREDRAW, 1, 0);

		//draw overview
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
			//fix loose bolts
			if (!carparts.empty())
			{
				BatchProcessBolts(TRUE);
				UpdateBDialog();
			}
			break;
		case IDC_BBUT2:
			//loosen all bolts
			if (!carparts.empty())
			{
				BatchProcessBolts(FALSE);
				UpdateBDialog();
			}
			break;
		case IDC_BBUT6:
			//repair parts
			if (!carparts.empty())
			{
				BatchProcessDamage(FALSE);
				UpdateBDialog();
			}
			break;
		case IDC_BBUT4:
			//repair all parts
			if (!carparts.empty())
			{
				BatchProcessDamage(TRUE);
				UpdateBDialog();
			}
			break;
		case IDC_BBUT3:
			//fix stuck parts
			if (!carparts.empty())
			{
				BatchProcessStuck();
				UpdateBDialog();
			}
			break;
		case IDC_BBUT5:
			//uninstall all
			if (!carparts.empty())
			{
				BatchProcessUninstall();
				UpdateBDialog();
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
			//something seriously went wrong here
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

		if (!EulerAngles)
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
		if (!allow_scale)
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
				UINT error, size;
				bool valid = TRUE;
				std::wstring value;
				std::string bin;
				HWND hEdit;
				EDITBALLOONTIP ebt;
				ebt.cbStruct = sizeof(EDITBALLOONTIP);
				ebt.pszTitle = _T("Error!");
				ebt.ttiIcon = TTI_ERROR_LARGE;

				//location
				hEdit = GetDlgItem(hTransform, IDC_LOC);
				size = GetWindowTextLength(hEdit) + 1;
				TCHAR *LocEditText = new TCHAR[size];
				memset(LocEditText, 0, size);
				GetWindowText(hEdit, (LPWSTR)LocEditText, size);
				std::wstring lvalue = LocEditText;
				delete[] LocEditText;

				error = VectorStrToBin(lvalue, 3, bin);
				if (error > 0)
				{
					valid = FALSE;
					ebt.pszText = GLOB_STRS[error].c_str();
					SendMessage(GetDlgItem(hTransform, IDC_LOC), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}

				//rotation
				hEdit = GetDlgItem(hTransform, IDC_ROT);
				size = GetWindowTextLength(hEdit) + 1;
				TCHAR *RotEditText = new TCHAR[size];
				memset(RotEditText, 0, size);
				GetWindowText(hEdit, (LPWSTR)RotEditText, size);
				value = RotEditText;
				delete[] RotEditText;

				if (!EulerAngles)
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
				hEdit = GetDlgItem(hTransform, IDC_SCA);
				size = GetWindowTextLength(hEdit) + 1;
				TCHAR *ScaEditText = new TCHAR[size];
				memset(ScaEditText, 0, size);
				GetWindowText(hEdit, (LPWSTR)ScaEditText, size);
				value = ScaEditText;
				delete[] ScaEditText;

				error = VectorStrToBin(value, 3, bin);
				if (error > 0)
				{
					valid = FALSE;
					ebt.pszText = GLOB_STRS[error].c_str();
					SendMessage(GetDlgItem(hTransform, IDC_SCA), EM_SHOWBALLOONTIP, 0, (LPARAM)&ebt);
				}

				if (valid)
				{
					UINT size = GetWindowTextLength(GetDlgItem(hwnd, IDC_TAG)) + 1;
					std::wstring str(size, '\0');
					GetWindowText(GetDlgItem(hwnd, IDC_TAG), (LPWSTR)str.data(), size);
					str.resize(size - 1);
					bin += char(str.size()) + WStringToString(str);

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
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		hColor = hwnd;

		UINT type = variables[indextable[iItem].index2].type;
		if (type != ID_COLOR)
		{
			//something seriously went wrong here
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			EnableWindow(hDialog, TRUE);
			break;
		}

		std::wstring str;
		FormatString(str, variables[indextable[iItem].index2].value, ID_COLOR);

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

				error = VectorStrToBin(value, 4, bin, FALSE, TRUE);
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
					std::string itemtag;
					UINT index_to = 0;
					UINT index_what = 0;
					UINT index = SendMessage(GetDlgItem(hwnd, IDC_LOCTO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
					std::string location;

					for (UINT i = 0; i < variables.size(); i++)
					{
						if (variables[i].type == ID_TRANSFORM)
						{
							if (variables[i].value.size() > 40)
								itemtag = variables[i].value.substr(40);
						}
					}

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
					location += itemtag;

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
			return CloseEditbox(hwnd);
		}
		case WM_GETDLGCODE:
			switch (wParam)
			{
				case VK_ESCAPE: 
				case VK_RETURN: 
					return CloseEditbox(hwnd);
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
					hColor = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLOR), hDialog, ColorProc);
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

