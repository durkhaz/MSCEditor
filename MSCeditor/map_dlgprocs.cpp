#ifdef _MAP
#include <windows.h>
#include "map.h"
#include "externs.h"
#include "resource.h"
#include "utils.h"

#define		WM_MAPCONTEXTMENU			(WM_USER + 10)

using Microsoft::WRL::ComPtr;

INT_PTR MapDialog::OnSidebarMouseWheel(WPARAM wParam, LPARAM lParam)
{
	m_SidebarListCurrentScrollpos -= GET_WHEEL_DELTA_WPARAM(wParam);
	m_SidebarListCurrentScrollpos = max(0, min(m_SidebarListCurrentScrollpos, static_cast<INT>(m_SidebarListScrollRange) - static_cast<INT>(m_SidebarListRenderTarget->GetSize().height)));

	SCROLLINFO si = { 0 };
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	INT_PTR ret = GetScrollInfo(m_hDialogSidebarList, SB_VERT, &si);

	if (m_SidebarListCurrentScrollpos != si.nPos)
	{
		si.nPos = m_SidebarListCurrentScrollpos;
		SetScrollInfo(m_hDialogSidebarList, SB_VERT, &si, TRUE);
		OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
	}
	return FALSE;
}

INT_PTR MapDialog::OnSidebarVScroll(WPARAM wParam, LPARAM lParam)
{
	INT newScrollPos = m_SidebarListCurrentScrollpos;

	switch (LOWORD(wParam))
	{
	case SB_LINEUP:
		newScrollPos -= 1;
		break;

	case SB_LINEDOWN:
		newScrollPos += 1;
		break;

	case SB_PAGEUP:
		newScrollPos -= static_cast<UINT>(m_SidebarListRenderTarget->GetSize().height);
		break;

	case SB_PAGEDOWN:
		newScrollPos += static_cast<UINT>(m_SidebarListRenderTarget->GetSize().height);
		break;

	case SB_THUMBTRACK:
	{
		SCROLLINFO si = { 0 };
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
		INT_PTR ret = GetScrollInfo(m_hDialogSidebarList, SB_VERT, &si);
		newScrollPos = si.nTrackPos;
		break;
	}
	default:
		break;
	}

	newScrollPos = max(0, min(newScrollPos, m_SidebarListScrollRange - static_cast<INT>(m_SidebarListRenderTarget->GetSize().height)));

	m_SidebarListCurrentScrollpos = newScrollPos;

	SCROLLINFO si = { 0 };
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	INT_PTR ret = GetScrollInfo(m_hDialogSidebarList, SB_VERT, &si);
	if (!ret)
		return TRUE;

	if (m_SidebarListCurrentScrollpos != si.nPos)
	{
		si.nPos = m_SidebarListCurrentScrollpos;
		SetScrollInfo(m_hDialogSidebarList, SB_VERT, &si, TRUE);
		OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
	}
	return FALSE;
}

void MapDialog::SidebarToggleCategory(uint32_t type)
{
	uint32_t NumDrawnItems = 0;
	for (auto& item : m_SidebarItems)
	{
		if (item.m_Type == type && item.m_Index != UINT_MAX)
			item.m_ScrollPos = item.m_ScrollPos != UINT_MAX  ? UINT_MAX : NumDrawnItems * s_MapSidebarListEntryHeight;
		else if (item.m_Type >= type && item.m_ScrollPos != UINT_MAX)
			item.m_ScrollPos = NumDrawnItems * s_MapSidebarListEntryHeight;

		if (item.m_ScrollPos != UINT_MAX)
			NumDrawnItems++;
	}
	m_SidebarListScrollRange = NumDrawnItems * s_MapSidebarListEntryHeight;
	m_SidebarListCurrentScrollpos = max(0, min(m_SidebarListCurrentScrollpos, m_SidebarListScrollRange - static_cast<INT>(m_SidebarListRenderTarget->GetSize().height)));
}

INT_PTR MapDialog::OnSidebarLButtondown(WPARAM wParam, LPARAM lParam)
{
	D2D1_POINT_2F p;
	p.x = static_cast<FLOAT>(GET_X_LPARAM(lParam));
	p.y = m_SidebarListCurrentScrollpos + static_cast<FLOAT>(GET_Y_LPARAM(lParam));

	for (auto& item : m_SidebarItems)
	{
		if (p.y >= item.m_ScrollPos && p.y <= item.m_ScrollPos + s_MapSidebarListEntryHeight)
		{
			// Toggle active state
			item.m_bActive = !item.m_bActive;

			// Is category?
			if (item.m_Index == UINT_MAX)
			{
				SidebarToggleCategory(item.m_Type);
				SidebarUpdateScrollbar();
			}
			else 
			{
				if (item.m_bActive)
				{
					m_MapMarkers.push_front(&item);
					if (item.m_Location.x == -1.f)
						item.m_Location = GetSidebarItemLocation(&item);
				}
				else
					m_MapMarkers.remove(&item);
				OnMapRender(m_MapRenderTarget->GetHwnd());
			}
			OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
			break;
		}
	}
	return FALSE;
}

D2D1_POINT_2F MapCursorToClient(HWND hwnd, D2D1_RECT_F *cRekt, LPARAM lParam)
{
	RECT wRekt, lRekt;
	GetWindowRect(hwnd, &wRekt);
	GetClientRect(hwnd, &lRekt);
	int szBorder = ((wRekt.right - wRekt.left) - lRekt.right) / 2;
	D2D1_POINT_2F p;
	p.x = static_cast<FLOAT>(GET_X_LPARAM(lParam) - wRekt.left - szBorder);
	p.y = static_cast<FLOAT>(GET_Y_LPARAM(lParam) - wRekt.top - (szBorder + GetSystemMetrics(SM_CYCAPTION)));
	cRekt->top = static_cast<FLOAT>(lRekt.top);
	cRekt->left = static_cast<FLOAT>(lRekt.left);
	cRekt->bottom = static_cast<FLOAT>(lRekt.bottom);
	cRekt->right = static_cast<FLOAT>(lRekt.right);
	return p;
}

bool MapDialog::OnSidebarPaint(HWND hwnd, bool bDrawSidebarCoordsOnly)
{
	static const INT right = ((s_MapSidebarWidth - s_MapSidebarListWidth) / 2) + s_MapSidebarListWidth;
	bool bSucc = SUCCEEDED(OnSidebarRender(hwnd)) && SUCCEEDED(OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd()));
	InvalidateRect(GetDlgItem(hwnd, 1), NULL, TRUE);
	InvalidateRect(GetDlgItem(hwnd, 2), NULL, TRUE);
	InvalidateRect(hwnd, &RECT{ right - GetSystemMetrics(SM_CXVSCROLL), s_MapSidebarListYOffset, right, s_MapSidebarListYOffset + static_cast<INT>(m_SidebarListRenderTarget->GetSize().height) }, TRUE);
	return !bSucc;
}

INT_PTR CALLBACK MapDialog::SidebarInfoProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static MapDialog* map = nullptr;
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		map = reinterpret_cast<MapDialog*>(lParam);
		break;
	}
	case WM_PAINT:
	case WM_DISPLAYCHANGE:
		if (map->m_D2DFactory)
			return map->OnSidebarInfoRender(hwnd);
	case WM_DESTROY:
		map = nullptr;
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK MapDialog::SidebarListProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static MapDialog* map = nullptr;
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		map = reinterpret_cast<MapDialog*>(lParam);
		break;
	}
	case WM_VSCROLL:
		return map->OnSidebarVScroll(wParam, lParam);
	case WM_MOUSEWHEEL:
		return map->OnSidebarMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return map->OnSidebarLButtondown(wParam, lParam);
	case WM_DESTROY:
		map = nullptr;
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK MapDialog::SidebarProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH backgroundBrush;
	static MapDialog* map = nullptr;
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		map = reinterpret_cast<MapDialog*>(lParam);
		backgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
		break;
	}
	case WM_PAINT:
	case WM_DISPLAYCHANGE: 
		if (map->m_D2DFactory)
			return map->OnSidebarPaint(hwnd, map);
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
		return (LRESULT)backgroundBrush;
	case WM_COMMAND:
	{
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
		{
			int CtrlID = GetDlgCtrlID((HWND)lParam);
			switch (CtrlID)
			{
			case 1:
			case 2:
				map->m_MapMarkers.clear();
				for (auto& item : map->m_SidebarItems)
					if (item.m_Index != UINT_MAX)
					{
						item.m_bActive = CtrlID == 1 ? TRUE : FALSE;
						if (CtrlID == 1)
						{
							map->m_MapMarkers.push_front(&item);
							if (item.m_Location.x == -1.f)
								item.m_Location = map->GetSidebarItemLocation(&item);
						}
					}
				map->OnSidebarListRender(map->m_SidebarListRenderTarget->GetHwnd());
				map->OnMapRender(map->m_MapRenderTarget->GetHwnd());
				SendMessage(map->m_hwnd, WM_NEXTDLGCTL, (WPARAM)map->m_MapRenderTarget->GetHwnd(), TRUE);
				break;
			}
		}
		}
		break;
	}
	case WM_DESTROY:
	{
		DeleteObject(backgroundBrush);
		map = nullptr;
		return FALSE;
	}
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK MapDialog::MapProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam)
{
	static MapDialog* map = nullptr;
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		map = reinterpret_cast<MapDialog*>(lParam);
		break;
	}
	case WM_PAINT:
	case WM_DISPLAYCHANGE:
		if (map->m_D2DFactory)
			return SUCCEEDED(map->OnMapRender(hwnd)) ? FALSE : TRUE;
	case WM_RBUTTONDOWN:
	{
		RECT rekt;
		GetWindowRect(hwnd, &rekt);
		rekt.left += GET_X_LPARAM(lParam);
		rekt.top += GET_Y_LPARAM(lParam);
		PostMessage(hwnd, WM_MAPCONTEXTMENU, rekt.top, rekt.left);
		return FALSE;
	}
	case WM_LBUTTONDOWN:
	{
		if (map->m_MapZoom == 1)
			return FALSE;
		map->m_bMapDragging = TRUE;
		RECT wRekt, cRekt;
		GetWindowRect(hwnd, &wRekt);
		GetClientRect(hwnd, &cRekt);

		int szBorder = ((wRekt.right - wRekt.left) - cRekt.right) / 2;
		wRekt.left += szBorder;
		wRekt.top += szBorder + GetSystemMetrics(SM_CYCAPTION);
		wRekt.right -= szBorder;
		wRekt.bottom -= szBorder;
		ClipCursor(&wRekt);

		D2D1_POINT_2F p;
		p.x = static_cast<FLOAT>(GET_X_LPARAM(lParam));
		p.y = static_cast<FLOAT>(GET_Y_LPARAM(lParam));
		map->m_MapDragPrevPt = p;
		return FALSE;
	}
	case WM_LBUTTONUP:
	{
		map->m_bMapDragging = FALSE;
		ClipCursor(NULL);
		return FALSE;
	}
	case WM_MOUSEMOVE:
	{
		D2D1_POINT_2F WndMousePos;
		WndMousePos.x = static_cast<FLOAT>(GET_X_LPARAM(lParam));
		WndMousePos.y = static_cast<FLOAT>(GET_Y_LPARAM(lParam));
		D2D1_SIZE_F rtSize = map->m_MapRenderTarget->GetSize();
		map->m_MapWindowMousePos = WndMousePos;
		if (map->m_bMapDragging)
		{
			D2D1_POINT_2F DesiredCenter = ToMapPos(map->m_MapViewCenterPos, rtSize);
			DesiredCenter.x -= (WndMousePos.x - map->m_MapDragPrevPt.x) / (map->m_MapZoom);
			DesiredCenter.y -= (WndMousePos.y - map->m_MapDragPrevPt.y) / (map->m_MapZoom);

			D2D1_RECT_F MapRekt = map->GetRenderedMapDimensions();
			D2D1_SIZE_F MapDimensions = D2D1::SizeF((MapRekt.right - MapRekt.left) / 2.f, (MapRekt.bottom - MapRekt.top) / 2.f);
			D2D1_SIZE_F WindowRekt = map->m_MapRenderTarget->GetSize();
			D2D1_RECT_F Border = D2D1::RectF(MapDimensions.width, MapDimensions.height, WindowRekt.width - MapDimensions.width, WindowRekt.height - MapDimensions.height);

			{
				if (DesiredCenter.x > Border.right)
					DesiredCenter.x = Border.right;
				else if (DesiredCenter.x < Border.left)
					DesiredCenter.x = Border.left;
				else
					map->m_MapDragPrevPt.x = WndMousePos.x;
				map->m_MapViewCenterPos.x = DesiredCenter.x / rtSize.width;
			}
			{
				if (DesiredCenter.y > Border.bottom)
					DesiredCenter.y = Border.bottom;
				else if (DesiredCenter.y < Border.top)
					DesiredCenter.y = Border.top;
				else
					map->m_MapDragPrevPt.y = WndMousePos.y;
				map->m_MapViewCenterPos.y = DesiredCenter.y / rtSize.height;
			}
			map->OnMapRender(hwnd);
		}
		else
		{
			D2D1_SIZE_F rtSize = map->m_MapRenderTarget->GetSize();
			bool bStartSelectionEmpty = map->m_MapMarkersSelected.empty();
			map->m_MapMarkersSelected.clear();
			FLOAT MouseAreaSizeX = map->m_MarkerRadius / rtSize.width / map->m_MapZoom;
			D2D1_RECT_F MouseAreaSize = D2D1::RectF(-MouseAreaSizeX, 0.f, MouseAreaSizeX, (map->m_MarkerRadius * 2.55f) / rtSize.height / map->m_MapZoom);
			D2D1_POINT_2F MousePos = ToRelativePos(map->RemapRenderedMapPosToMapPos(&WndMousePos), map->m_MapRenderTarget->GetSize());
			D2D1_RECT_F MouseArea = D2D1::RectF(MousePos.x + MouseAreaSize.left, MousePos.y, MousePos.x + MouseAreaSize.right, MousePos.y + MouseAreaSize.bottom);

			for (auto& Marker : map->m_MapMarkers)
				if (Marker->m_Location.x >= MouseArea.left && Marker->m_Location.y >= MouseArea.top && Marker->m_Location.x <= MouseArea.right && Marker->m_Location.y <= MouseArea.bottom)
					map->m_MapMarkersSelected.push_back(Marker);

			bool bEndSeletionEmpty = map->m_MapMarkersSelected.empty();
			if (!bEndSeletionEmpty || (!bStartSelectionEmpty && bEndSeletionEmpty))
				map->OnMapRender(hwnd);
			map->OnSidebarInfoRender(map->m_SidebarInfoRenderTarget->GetHwnd());
		}
		return FALSE;
	}
	case WM_MOUSEWHEEL:
	{
		FLOAT delta = map->m_MapZoom * GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		bool bZoomingOut = delta < 0;

		if (bZoomingOut)
			delta /= 2.f;

		D2D1_SIZE_F cRekt = map->m_MapRenderTarget->GetSize();
		RECT wRekt;
		GetWindowRect(hwnd, &wRekt);
		int szBorder = ((wRekt.right - wRekt.left) - static_cast<int>(cRekt.width)) / 2;
		D2D1_POINT_2F p = D2D1::Point2F(static_cast<FLOAT>(GET_X_LPARAM(lParam) - wRekt.left - szBorder), static_cast<FLOAT>(GET_Y_LPARAM(lParam) - wRekt.top - (szBorder + GetSystemMetrics(SM_CYCAPTION))));

		if (p.x < 0.f || p.y < 0.f || p.x > cRekt.width || p.y > cRekt.height || map->m_MapZoom + delta > s_MapMaxZoom)
			break;

		if (map->m_MapZoom + delta <= 1)
		{
			map->m_MapZoom = 1.f;
			delta = 0.f;
			map->m_MapViewCenterPos = D2D1::Point2F(0.5f, 0.5f);
		}
		else
		{
			if (!bZoomingOut)
			{
				D2D1_RECT_F CurrentMapRekt = map->GetRenderedMapDimensions();
				FLOAT DesiredX = CurrentMapRekt.left + ((p.x / cRekt.width) * (CurrentMapRekt.right - CurrentMapRekt.left));
				FLOAT DesiredY = CurrentMapRekt.top + ((p.y / cRekt.height) * (CurrentMapRekt.bottom - CurrentMapRekt.top));
				FLOAT Alpha = (map->m_MapZoom - 1) / (s_MapMaxZoom - 1);

				map->m_MapViewCenterPos.x = (((1 - Alpha) * DesiredX) + (Alpha * (map->m_MapViewCenterPos.x * cRekt.width))) / cRekt.width;
				map->m_MapViewCenterPos.y = (((1 - Alpha) * DesiredY) + (Alpha * (map->m_MapViewCenterPos.y * cRekt.height))) / cRekt.height;
			}

			D2D1_RECT_F MapRekt = map->GetRenderedMapDimensions(map->m_MapZoom + delta);
			if (MapRekt.left > 0)
			{
				if (MapRekt.right > cRekt.width)
					map->m_MapViewCenterPos.x += (cRekt.width - MapRekt.right) / cRekt.width;
			}
			else
				map->m_MapViewCenterPos.x -= MapRekt.left / cRekt.width;

			if (MapRekt.top > 0)
			{
				if (MapRekt.bottom > cRekt.height)
					map->m_MapViewCenterPos.y += (cRekt.height - MapRekt.bottom) / cRekt.height;
			}
			else
				map->m_MapViewCenterPos.y -= MapRekt.top / cRekt.height;
		}

		map->m_MapZoom += delta;
		map->OnMapRender(hwnd);
		return FALSE;
	}
	case WM_MAPCONTEXTMENU:
	{
		HMENU hPopupMenu = CreatePopupMenu();
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_MAP_CLIPBOARD, GLOB_STRS[56].c_str());
		InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_MAP_TELEPORT, GLOB_STRS[57].c_str());
		if (map->m_MapDistanceNodes.empty())
			InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_MAP_MEASURE_DISTANCE, GLOB_STRS[58].c_str());
		else
		{
			InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_MAP_MEASURE_DISTANCE, GLOB_STRS[59].c_str());
			InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, ID_MAP_CLEAR_MEASURE, GLOB_STRS[60].c_str());
		}

		TrackPopupMenuEx(hPopupMenu, TPM_LEFTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_VERPOSANIMATION, static_cast<int32_t>(lParam), static_cast<int32_t>(wParam), hwnd, NULL);
		DestroyMenu(hPopupMenu);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_MAP_CLIPBOARD:
		{
			if (!OpenClipboard(map->m_hwnd))
				break;

			std::wstring Str(64, '\0');
			const D2D_VECTOR_3F WorldMousePos = map->RemapMapCursorPosToWorldPos();
			Str.resize(swprintf(&Str[0], 64, L"%.3f, %.3f, %.3f", WorldMousePos.x, WorldMousePos.y, WorldMousePos.z));
			std::size_t SizeInWords = Str.size() + 1;
			std::size_t SizeInBytes = SizeInWords * sizeof(wchar_t);

			HANDLE hRc = NULL;
			HGLOBAL hGlob = GlobalAlloc(GHND | GMEM_SHARE, SizeInBytes);
			if (hGlob) 
			{
				wchar_t* pvGlob = static_cast<wchar_t*>(GlobalLock(hGlob));
				if (pvGlob)
				{
					wcscpy_s(pvGlob, SizeInWords, Str.c_str());
					GlobalUnlock(hGlob);
					EmptyClipboard();
					hRc = SetClipboardData(CF_UNICODETEXT, hGlob);
				}
				if (!hRc)
					GlobalFree(hGlob);
			}
			CloseClipboard();
			break;
		}
		case ID_MAP_TELEPORT:
		{
			const D2D_VECTOR_3F WorldMousePos = map->RemapMapCursorPosToWorldPos();
			std::wstring Str(64, '\0');
			Str.resize(swprintf(&Str[0], 64, L"%.3f, %.3f, %.3f", WorldMousePos.x, WorldMousePos.y, WorldMousePos.z));
			std::string Bin = Variable::ValueStrToBin(Str, EntryValue::Vector3);
			SendMessage(hDialog, WM_COMMAND, MAKEWPARAM(ID_TOOLS_TELEPORTENTITY, 0), reinterpret_cast<LPARAM>(&Bin));
			break;
		}
		case ID_MAP_MEASURE_DISTANCE:
		{
			map->m_MapDistanceNodes.push_back(ToRelativePos(map->RemapRenderedMapPosToMapPos(&map->m_MapWindowMousePos), map->m_MapRenderTarget->GetSize()));
			map->OnMapRender(map->m_MapRenderTarget->GetHwnd());
			break;
		}
		case ID_MAP_CLEAR_MEASURE:
		{
			map->m_MapDistanceNodes.clear();
			map->OnMapRender(map->m_MapRenderTarget->GetHwnd());
			break;
		}
		}
		break;
	}
	case WM_DESTROY:
		map = nullptr;
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK MapDialog::MainProc(HWND hwnd, unsigned int Message, WPARAM wParam, LPARAM lParam)
{
	static MapDialog* map = nullptr;

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		if (appfolderpath.empty())
		{
			MessageBox(hDialog, GLOB_STRS[55].c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
			DestroyWindow(hwnd);
			break;
		}
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(hr))
		{
			map = reinterpret_cast<MapDialog*>(lParam);
			LOG(L"Map : Instantiating\n");
			map->m_hwnd = hwnd;

			// Set Icon
			HICON hicon = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);

			RECT rekt;
			GetClientRect(hwnd, &rekt);

			// Create Sidebar window
			map->m_hDialogSidebar = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_BLANKMAP), hwnd, SidebarProc, reinterpret_cast<LPARAM>(map));
			SetWindowPos(map->m_hDialogSidebar, NULL, rekt.right - s_MapSidebarWidth, 0, s_MapSidebarWidth, rekt.bottom - s_MapSidebarInfoHeight, SWP_SHOWWINDOW);

			int Padding = (s_MapSidebarWidth - s_MapSidebarListWidth) / 2;
			map->m_hSelectAll = CreateWindowEx(0, WC_BUTTON, L"All", WS_CHILD | WS_VISIBLE, 60 + Padding, 170, 50, 20, map->m_hDialogSidebar, (HMENU)1, hInst, NULL);
			map->m_hUnselectAll = CreateWindowEx(0, WC_BUTTON, L"None", WS_CHILD | WS_VISIBLE, 110 + Padding, 170, 50, 20, map->m_hDialogSidebar, (HMENU)2, hInst, NULL);
			SendMessage(map->m_hSelectAll, WM_SETFONT, (WPARAM)hListFont, TRUE);
			SendMessage(map->m_hUnselectAll, WM_SETFONT, (WPARAM)hListFont, TRUE);

			// Create Sidebar list window
			map->m_hDialogSidebarList = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_BLANKMAP), map->m_hDialogSidebar, SidebarListProc, reinterpret_cast<LPARAM>(map));
			SetWindowLongPtr(map->m_hDialogSidebarList, GWL_STYLE, WS_VSCROLL);
			SetWindowPos(map->m_hDialogSidebarList, NULL, Padding, s_MapSidebarListYOffset, s_MapSidebarListWidth, rekt.bottom - s_MapSidebarInfoHeight - s_MapSidebarListYOffset - Padding, SWP_SHOWWINDOW);

			// Create Sidebar info window
			map->m_hDialogSidebarInfo = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_BLANKMAP), hwnd, SidebarInfoProc, reinterpret_cast<LPARAM>(map));
			SetWindowPos(map->m_hDialogSidebarInfo, NULL, rekt.right - s_MapSidebarWidth, rekt.bottom - s_MapSidebarInfoHeight, s_MapSidebarWidth, s_MapSidebarInfoHeight, SWP_SHOWWINDOW);

			// Create map window
			map->m_hDialogMap = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_BLANKMAP), hwnd, MapProc, reinterpret_cast<LPARAM>(map));
			SetWindowPos(map->m_hDialogMap, NULL, 0, 0, rekt.right - s_MapSidebarWidth, rekt.bottom, SWP_SHOWWINDOW);

			HERROR hr = map->CreateDeviceIndependentResources();
			if (SUCCEEDED(hr.first))
			{
				InvalidateRect(hwnd, NULL, TRUE);
				LOG(L"Map : Instantiation successful!\n");
				break;
			}
			LOG(L"Map : Instantiation failed!\n");
			std::wstring hrstr(64, '\0');
			HRToStr(hr.first, hrstr);
			std::wstring bla = StringToWString(hr.second);
			hrstr += bla;
			throw hrstr;
		}
	}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMinTrackSize.x= s_WindowMinSize.width;
		lpmmi->ptMinTrackSize.y = s_WindowMinSize.height;
		return FALSE;
	}
	case WM_MOVING:
	{
		return FALSE;
	}
	case WM_SIZE:
	{
		D2D1_SIZE_U NewSize = D2D1::SizeU(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		int MapWidth = NewSize.width - s_MapSidebarWidth;
		int Padding = (s_MapSidebarWidth - s_MapSidebarListWidth) / 2;
		int SidebarListHeight = NewSize.height - s_MapSidebarInfoHeight - s_MapSidebarListYOffset - Padding;
		// Note: These methods can fail, but it's okay to ignore the error here -- it will be repeated on the next call to EndDraw.

		map->m_MapRenderTarget->Resize(D2D1::SizeU(MapWidth, NewSize.height));
		map->m_SidebarRenderTarget->Resize(D2D1::SizeU(static_cast<INT>(map->m_SidebarRenderTarget->GetSize().width), NewSize.height - s_MapSidebarInfoHeight));
		map->m_SidebarListRenderTarget->Resize(D2D1::SizeU(static_cast<INT>(map->m_SidebarListRenderTarget->GetSize().width), SidebarListHeight));
		map->OnSidebarInfoRender(map->m_SidebarListRenderTarget->GetHwnd());

		map->SidebarUpdateScrollbar(NewSize.height - s_MapSidebarListYOffset - Padding);
		map->OnSidebarVScroll(MAKEWPARAM(SB_THUMBTRACK, 0), 0);

		MoveWindow(map->m_hDialogMap, 0, 0, MapWidth, NewSize.height, FALSE);
		MoveWindow(map->m_hDialogSidebar, MapWidth, 0, s_MapSidebarWidth, NewSize.height - s_MapSidebarInfoHeight, FALSE);
		MoveWindow(map->m_hDialogSidebarList, Padding, s_MapSidebarListYOffset, s_MapSidebarListWidth, SidebarListHeight, FALSE);
		MoveWindow(map->m_hDialogSidebarInfo, MapWidth, NewSize.height - s_MapSidebarInfoHeight, s_MapSidebarWidth, s_MapSidebarInfoHeight, FALSE);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return FALSE;
	}
	case WM_CLOSE:
		if (EditorMap)
			DestroyWindow(map->m_hwnd);
		return FALSE;
	case WM_DESTROY:
	{
		ClipCursor(NULL);
		if (EditorMap)
		{
			delete map;
			map = nullptr;
		}
		return FALSE;
	}
	default:
		return FALSE;
	}
	return TRUE;
}
#endif