#include "dlgprocs.h"

#include <CommCtrl.h>


BOOL CALLBACK AboutProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwnd, IDC_VERSION), WM_SETTEXT, 0, (LPARAM)Title);
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

		unsigned int item = 0;
		Overview ov;

		SendMessage(hList3, WM_SETREDRAW, 0, 0);

		if (!carparts.empty())
		{
			for (unsigned int i = 0; i < carparts.size(); i++)
			{
				PopulateBList(hwnd, &carparts[i], item, &ov);
			}
		}
		else
		{
			for (unsigned int i = 0; i < variables.size(); i++)
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
					for (unsigned int j = 0; j < variables.size(); j++)
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
						for (unsigned int j = 0; j < partSCs.size(); j++)
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

						for (unsigned int j = 0; j < variables.size(); j++)
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
		for (unsigned int i = 0; i < locations.size(); i++)
		{
			SendMessage(GetDlgItem(hwnd, IDC_TELETO), (UINT)CB_ADDSTRING, 0, (LPARAM)locations[i].badstring.c_str());
		}

		for (unsigned int i = 0; i < variables.size(); i++)
		{
			if (variables[i].type == ID_TRANSFORM)
			{
				SendMessage(GetDlgItem(hwnd, IDC_TELEFROM), (UINT)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
				SendMessage(GetDlgItem(hwnd, IDC_TELETO), (UINT)CB_ADDSTRING, 0, (LPARAM)variables[i].key.c_str());
			}
		}

		CheckDlgButton(hwnd, IDC_TELEC, BST_CHECKED);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_APPLY:
		{
			unsigned int index = SendMessage(GetDlgItem(hwnd, IDC_TELEFROM), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			unsigned int index_from = 0;
			for (unsigned int i = 0; i < variables.size(); i++)
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
			index = SendMessage(GetDlgItem(hwnd, IDC_TELETO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			unsigned int index_to = 0;

			if (index < locations.size())
			{
				std::string _tmp = WStringToString(locations[index].newstring).substr(0, 12) + variables[index_from].value.substr(12);
				IsDlgButtonChecked(hwnd, IDC_TELEC) == BST_CHECKED ? UpdateValue(_T(""), index_from, _tmp) : UpdateValue(_T(""), index_from, WStringToString(locations[index].newstring));
			}
			else
			{
				for (unsigned int i = 0; i < variables.size(); i++)
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
				std::string _tmp = variables[index_to].value.substr(0, 12) + variables[index_from].value.substr(12);
				IsDlgButtonChecked(hwnd, IDC_TELEC) == BST_CHECKED ? UpdateValue(_T(""), index_from, _tmp) : UpdateValue(_T(""), index_from, variables[index_to].value);
			}

			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);
			break;
		}

		case IDC_DISCARD:
			EndDialog(hwnd, 0);
			DestroyWindow(hwnd);

			break;

		case IDC_TELEFROM:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (SendMessage(GetDlgItem(hwnd, IDC_TELETO), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1) ::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			}
			break;

		case IDC_TELETO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if (SendMessage(GetDlgItem(hwnd, IDC_TELEFROM), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0) != -1) ::EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
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

		unsigned int type = variables[indextable[iItem].index2].type;
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
		for (unsigned int i = 0; i < 3; i++)
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
			for (unsigned int i = 0; i < 4; i++)
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
			for (unsigned int i = 0; i < 3; i++)
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
		for (unsigned int i = 0; i < 3; i++)
		{
			tstr = BinToFloatStr(value.substr(28 + (4 * i), 4));
			TruncFloatStr(tstr);
			str += (tstr + (", "));
		}
		str.resize(str.size() - 2);
		SendMessage((GetDlgItem(hTransform, IDC_SCA)), WM_SETTEXT, 0, (LPARAM)StringToWString(str).c_str());

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
				unsigned int error, size;
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

		unsigned int type = variables[indextable[iItem].index2].type;
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
				unsigned int error, size;
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

		unsigned int i = 4;
		while (TRUE)
		{
			if (i + 1 > wstr.size()) break;
			unsigned int len = static_cast<int>(wstr[i]);
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
				unsigned int max = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_GETLINECOUNT, 0, 0);

				int len = GetWindowTextLength(GetDlgItem(hwnd, IDC_STRINGEDIT)) + 1;
				std::wstring ctrlText(len, '\0');
				//TCHAR* ctrlText = new TCHAR[len];
				//memset(ctrlText, 0, len);
				GetWindowText(GetDlgItem(hwnd, IDC_STRINGEDIT), (LPTSTR)ctrlText.data(), len);

				unsigned int lines = 0;
				std::string value;
				for (unsigned int i = 0; i < max; i++)
				{
					unsigned int charindex = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_LINEINDEX, i, 0);
					unsigned int linelen = SendMessage(GetDlgItem(hwnd, IDC_STRINGEDIT), EM_LINELENGTH, charindex, 0);
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

LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KILLFOCUS:
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

				DestroyWindow(hwnd);
				return TRUE;
			}
			TruncFloatStr(value);
			break;
		}
		case ID_STRING:
		{
			break;
		}
		}
		UpdateValue(value, indextable[iItem].index2);

		DestroyWindow(hwnd);
		break;
	}
	}

	return CallWindowProc(DefaultEditProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ComboProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
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
			UpdateValue(_T(""), indextable[itemclicked.iItem].index2, variables[indextable[itemclicked.iItem].index2].static_value);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	{
		if (hEdit != NULL) { SendMessage(hEdit, WM_KILLFOCUS, 0, 0); };
		if (hCEdit != NULL) { SendMessage(hCEdit, WM_CLOSE, 0, 0); };
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
			SendMessage(GetDlgItem(hDialog, IDC_OUTPUT2), WM_SETTEXT, 0, (LPARAM)TypeStrs[type].c_str());

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
					WS_CHILD | WS_VISIBLE | ES_WANTRETURN | ES_AUTOHSCROLL,
					rekt.left, rekt.top, width, static_cast<int>(1.5*height), hwnd, 0, GetModuleHandle(NULL), NULL);

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
					rekt.left, rekt.top, 100, 400, hwnd, NULL, GetModuleHandle(NULL), NULL);

				SendMessage(hCEdit, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)bools[boolindex]);
				SendMessage(hCEdit, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(boolindex == 0 ? bools[1] : bools[0]));
				SendMessage(hCEdit, CB_SETCURSEL, 0, 0);
				DefaultComboProc = (WNDPROC)SetWindowLong(hCEdit, GWL_WNDPROC, (LONG)ComboProc);
				//SetFocus(hCEdit);
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

	case WM_COMMAND:
	{
		// handle hcedits messages in here because I'm messy like that 
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			std::wstring value(6, '\0');
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)value.data());

			SendMessage(hCEdit, CB_RESETCONTENT, 0, 0);
			ShowWindow(hCEdit, SW_HIDE);
			UpdateValue(value, indextable[iItem].index2);
		}
		break;
	}
	}
	return CallWindowProc(DefaultListCtrlProc, hwnd, message, wParam, lParam);
}

