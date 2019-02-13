#pragma once
#ifdef _MAP
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Shlwapi.lib")

#include <d2d1helper.h>
#include <vector>
#include <forward_list>
#include <wrl\client.h>
#include "heightmap.h"
#define LEN(arr) (sizeof (arr) / sizeof (arr)[0])

// Make sure we link against the intended symbol on Windows 7
#if defined(CLSID_WICImagingFactory)
#undef CLSID_WICImagingFactory
#endif

struct ID2D1Factory;
struct IWICImagingFactory;
struct IDWriteFactory;
struct IWICFormatConverter;
class Variable;
typedef std::pair<HRESULT, std::string> HERROR;

// DDS MD5 checksum
static const BYTE s_DDSMD5[] = { 0x71, 0xDE, 0xA1, 0x77, 0x3D, 0xDC, 0x1F, 0xE3, 0xCD, 0xD0, 0x0B, 0x7F, 0x57, 0x07, 0x55, 0x18 };

// Map is taken from a slight angle in orthographic view (10°) so sprites such as trees can be seen.
// That equates to the y axis having  1.015 GU/Px (game units / map pixel) more than the x axis.
static const FLOAT s_WorldYAlpha = 1.015f;

// The dimensions of the map in game-world units
static const D2D1_RECT_F s_WorldDimensions = D2D1::RectF(-2377.7777777777777f, -1802.407407f * s_WorldYAlpha, 2377.7777777777777f, 1802.407407f * s_WorldYAlpha);

// The dimensions of the map in device independent pixels (doesn't include DDS padding)
static const D2D1_SIZE_F s_MapSize = D2D1::SizeF(12840.f, 9733.f);

// Number of rows in the height map array
static const INT s_HeightMapHeight = LEN(s_HeightMap);

// Number of columns in the height map array
static const INT s_HeightMapWidth = s_HeightMapHeight == 0 ? 0 : LEN(s_HeightMap[0]);

// Min dimensions of the window. Can't resize to dimensions lower than this
static const D2D1_SIZE_U s_WindowMinSize = D2D1::SizeU(500, 350);

// The width of the map sidebar in dialog window pixels
static const INT s_MapSidebarWidth = 300;

// The y offset of the sidebar list
static const INT s_MapSidebarListYOffset = 200;

// The width of the map sidebar list in dialog window pixels
static const INT s_MapSidebarListWidth = 250;

// The height of an entry in the sidebar list in dialog window pixels
static const INT s_MapSidebarListEntryHeight = 15;

// The height of the sidebar info window
static const INT s_MapSidebarInfoHeight = 20;

// Max zoom level of the map (in steps of 2^n)
static const FLOAT s_MapMaxZoom = 16.f;

// Name of map DDS
static const wchar_t s_MapDDSName[] = L"msce110.map";

// Color for the highlights (a neon yellow)
static const D2D1::ColorF s_ColorHighlights = D2D1::ColorF(1.f, 1.f, 0.f, 1.f);

// Color for the shadows (a dark yellow)
static const D2D1::ColorF s_ColorShadows = D2D1::ColorF(0.5f, 0.5f, 0.f, 1.f);

// Color for the background (a retro brown)
static const D2D1::ColorF s_ColorBackground = D2D1::ColorF(0.39f, 0.1333f, 0.07f, 1.f);

// Remaps a relative position (0 - 1) to map position
D2D1_POINT_2F ToMapPos(const D2D1_POINT_2F& RelativePos, const D2D1_SIZE_F& WndSize);

// Remaps a map position to a relative position
D2D1_POINT_2F ToRelativePos(const D2D1_POINT_2F& MapPos, const D2D1_SIZE_F& WndSize);

class SidebarListEntry
{
public:
	SidebarListEntry(uint32_t type, uint32_t index, uint32_t scrollpos, bool bActive)
		: m_Type(std::move(type)), m_Index(std::move(index)), m_ScrollPos(std::move(scrollpos)), m_bActive(std::move(bActive)), m_Location(D2D1::Point2F(-1.f, -1.f))
	{	}

	enum EntryType { Landmark, Carpart, Misc };
	uint32_t m_Type, m_Index, m_ScrollPos;
	bool m_bActive;
	D2D1_POINT_2F m_Location;
};

class MapDialog
{
public:
	MapDialog();
	~MapDialog();

	void ClearAllMapObjects(bool bForceReRender = TRUE);
	void UpdateAllMapObjects(bool bForceReRender = TRUE);
	void UpdateMapObject(Variable* var,  bool bForceReRender = TRUE);
	void ShowObjectOnMap(Variable* var);

private:
	static INT_PTR CALLBACK SidebarInfoProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SidebarListProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SidebarProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK MapProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK MainProc(HWND hwnd, uint32_t Message, WPARAM wParam, LPARAM lParam);

	HERROR CreateDeviceResources();
	HERROR CreateDeviceIndependentResources();
	SidebarListEntry* FindVariableInSidebarList(Variable* var);
	void RenderTextWithShadow(const Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget>& RenderTarget, const D2D1_POINT_2F Pos, const Microsoft::WRL::ComPtr<IDWriteTextLayout>& TextLayout, const Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& Brush, const D2D1_COLOR_F Color, const FLOAT offset = 1.f);
	HRESULT OnMapRender(HWND hwnd);
	bool OnSidebarPaint(HWND hwnd, bool bDrawSidebarCoordsOnly = FALSE);
	HRESULT OnSidebarRender(HWND hwnd);
	HRESULT OnSidebarListRender(HWND hwnd);
	HRESULT OnSidebarInfoRender(HWND hwnd);
	INT_PTR OnSidebarVScroll(WPARAM wParam, LPARAM lParam);
	INT_PTR OnSidebarMouseWheel(WPARAM wParam, LPARAM lParam);
	INT_PTR OnSidebarLButtondown(WPARAM wParam, LPARAM lParam);
	void SidebarToggleCategory(uint32_t type);
	void SidebarUpdateScrollbar(uint32_t nPage = UINT_MAX);

	// Convert JPG to raw bitmap
	HERROR PrepareRawImage(BYTE** pDeobfuscatedResource, Microsoft::WRL::ComPtr<IWICFormatConverter>& pConverter, const DWORD imageFileSize, const bool bIsDDS = TRUE);

	// Gets the JPG from resource, converts it to bitmap, then to DDS and then saves it to file. Is working on a seperate thread. Only executed for Windows 8 and greater
	void PrepareMapDDS();

	// Load the JPG from resource
	HERROR LoadJPG();

	// Loads DDS from file. Can be called from seperate thread
	HERROR LoadDDS(const bool bCalledFromThread = FALSE);

	// Checks wether the DDS file is valid
	bool IsMapDDSValid();

	// Remaps a point of the rendered map to that of the absolute map
	D2D1_POINT_2F RemapRenderedMapPosToMapPos(const D2D1_POINT_2F* RenderedMapPos) const;

	// Remaps a point of the absolute map tp that of the rendered one
	D2D1_POINT_2F RemapMapPosToRenderedMapPos(const D2D1_POINT_2F* MapPos) const;

	// Remaps a point on the map to game world postion
	D2D1_POINT_2F RemapMapPosToWorldPos(const D2D1_POINT_2F* WndPos) const;

	// Remaps a game world position to map position
	D2D1_POINT_2F RemapWorldPosToMapPos(const D2D1_POINT_2F* WorldPos) const;

	// Returns portion of map window that is currently being rendered in device dependent pixels
	D2D1_RECT_F GetRenderedMapDimensions(FLOAT LocalMapZoom = -1.f) const;

	// Converts binary into point
	static D2D1_POINT_2F GetItemLocation(const std::string& bin);

	// Gets the display name of the sidebar item
	std::wstring GetSidebarItemName(const SidebarListEntry* entry) const;

	// Gets the AGUbƒö7g6 bp794a4BHP()NAB$NSFAASD JASD AW AK" ="=  PD=GI" SSSSSsssssss  s    s s
	D2D1_POINT_2F GetSidebarItemLocation(const SidebarListEntry* entry) const;

	// Gets interpolated height at given map location (relative pos)
	FLOAT GetHeightAtRelMapPos(const D2D1_POINT_2F* MapPos) const;

	// Remaps the current position of the mouse cursor to game world position
	D2D_VECTOR_3F RemapMapCursorPosToWorldPos() const;

	// Gets the distance in game world units between two points on the map
	FLOAT GetWorldDistanceBetweenTwoMapPoints(const D2D1_POINT_2F* MapPos1, const D2D1_POINT_2F* MapPos2) const;

public:
	HWND m_hwnd;

private:
	Microsoft::WRL::ComPtr<	ID2D1Factory>				m_D2DFactory;
	Microsoft::WRL::ComPtr<	IWICImagingFactory>			m_IWICFactory;
	Microsoft::WRL::ComPtr<	IDWriteFactory>				m_DWriteFactory;
	Microsoft::WRL::ComPtr<	ID2D1Bitmap>				m_MapBitmap;

	Microsoft::WRL::ComPtr<	ID2D1HwndRenderTarget>		m_MapRenderTarget;
	Microsoft::WRL::ComPtr<	ID2D1HwndRenderTarget>		m_SidebarListRenderTarget;
	Microsoft::WRL::ComPtr<	ID2D1HwndRenderTarget>		m_SidebarRenderTarget;
	Microsoft::WRL::ComPtr<	ID2D1HwndRenderTarget>		m_SidebarInfoRenderTarget;

	Microsoft::WRL::ComPtr<	ID2D1SolidColorBrush>		m_MapSceneBrush;
	Microsoft::WRL::ComPtr<	ID2D1SolidColorBrush>		m_SidebarSceneBrush;
	Microsoft::WRL::ComPtr<	ID2D1SolidColorBrush>		m_SidebarListSceneBrush;
	Microsoft::WRL::ComPtr<	ID2D1SolidColorBrush>		m_SidebarInfoSceneBrush;

	Microsoft::WRL::ComPtr<	ID2D1PathGeometry>			m_MapMarkerGeometry;
	Microsoft::WRL::ComPtr<	ID2D1PathGeometry>			m_MapArrowMarkerGeometry;
	Microsoft::WRL::ComPtr<	IDWriteTextFormat>			m_MonospaceTextFormat;
	Microsoft::WRL::ComPtr<	IDWriteTextFormat>			m_ListTextFormat;

	bool												m_bMapDragging;
	FLOAT												m_MarkerRadius;
	FLOAT												m_MapZoom;
	INT													m_SidebarListCurrentScrollpos;
	INT													m_SidebarListScrollRange;
	UINT												m_SatsumaTransformIndex;

	D2D1_POINT_2F										m_MapWindowMousePos;
	D2D1_POINT_2F										m_MapDragPrevPt;
	D2D1_POINT_2F										m_MapViewCenterPos;
										
	std::forward_list<SidebarListEntry*>				m_MapMarkers;
	std::vector<SidebarListEntry>						m_SidebarItems;
	std::vector<Variable*>								m_SidebarAssortedItems; 
	std::vector<SidebarListEntry*>						m_MapMarkersSelected;
	std::vector<D2D1_POINT_2F>							m_MapDistanceNodes;

	std::wstring										m_MapDDSPath;
	D2D1_SIZE_U											m_DialogMapSize;
	HWND												m_hDialogSidebar;
	HWND												m_hDialogSidebarList;
	HWND												m_hDialogSidebarInfo;
	HWND												m_hDialogMap;
	HWND												m_hSelectAll;
	HWND												m_hUnselectAll;
};

#endif