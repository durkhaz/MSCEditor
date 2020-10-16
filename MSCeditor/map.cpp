#ifdef _MAP
#include <windows.h>
#include <wincodec.h>
#include <shlwapi.h> 
#include <thread>
#include <commdlg.h>
#include <d2d1.h>
#include <dwrite.h>
#include "map.h"
#include "externs.h"
#include "resource.h"
#include "utils.h"
#include "bcrypt.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif // NT_SUCCESS
#define STATUS_SUCCESS 0x00000000

#define MAKE_HR(hr) \
if (FAILED(hr.first) && hr.second[0] != s_eStr[0]) \
sprintf_s(&(hr.second)[0], s_eSz, s_eStr, __FUNCTION__, __LINE__ - 3)

using Microsoft::WRL::ComPtr;

/******************************************************************
*                                                                 *
*  Helper functions					                              *
*                                                                 *
******************************************************************/


D2D1_POINT_2F ToMapPos(const D2D1_POINT_2F& RelativePos, const D2D1_SIZE_F& WndSize)
{
	return D2D1::Point2F(RelativePos.x * WndSize.width, RelativePos.y * WndSize.height);
}

D2D1_POINT_2F ToRelativePos(const D2D1_POINT_2F& MapPos, const D2D1_SIZE_F& WndSize)
{
	return D2D1::Point2F(MapPos.x / WndSize.width, MapPos.y / WndSize.height);
}

FLOAT GetDistanceBetweenTwoPoints(const D2D1_POINT_2F* p1, const D2D1_POINT_2F* p2)
{
	return sqrt(((p2->x - p1->x) * (p2->x - p1->x)) + ((p2->y - p1->y) * (p2->y - p1->y)));
}

FLOAT lerp(FLOAT s, FLOAT e, FLOAT t)
{
	return s + (e - s)*t;
}

FLOAT blerp(FLOAT c00, FLOAT c10, FLOAT c01, FLOAT c11, FLOAT tx, FLOAT ty)
{
	return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}

D2D1_SIZE_F GetCursorSize()
{
	// We essentially just look for the first transparent pixel on a cursor bitmap to determine its size
	D2D1_SIZE_F size;
	int i, j, n, width, height;

	ICONINFO ii;
	BITMAP bitmap;
	GetIconInfo((HICON)GetCursor(), &ii);
	GetObject(ii.hbmMask, sizeof(BITMAP), &bitmap);
	width = bitmap.bmWidth;
	if (ii.hbmColor == NULL)
		height = bitmap.bmHeight / 2;
	else
		height = bitmap.bmHeight;

	HDC dc = CreateCompatibleDC(GetDC(hDialog));
	SelectObject(dc, ii.hbmMask);
	for (i = 0, n = 0; i < width; i++)
		for (j = 0; j < height; j++)
			if (GetPixel(dc, i, j) != RGB(255, 255, 255))
				if (n < j)
					n = j;

	DeleteDC(dc);
	if (ii.hbmColor != NULL)
		DeleteObject(ii.hbmColor);
	DeleteObject(ii.hbmMask);
	size.height = static_cast<FLOAT>(n - ii.yHotspot);
	size.width = size.height;
	return size;
}

// MAKE SURE THESE AREN'T INCLUDED IN THE SOURCE
namespace Obfuscate
{
	HRESULT ObfuscateIStream(IStream* pOutStream)
	{
		/* Omitted */
		return S_OK;
	}

	HRESULT DeobfuscateResource(BYTE* ResourceBytes, const DWORD imageFileSize, BYTE* OutputBytes)
	{
		/* Omitted */
		return S_OK;
	}

	HRESULT IsValidJPG(BYTE* Bytes, const DWORD NumBytes)
	{
		/* Omitted */
		return S_OK;
	}
}

HRESULT PrepareResource(BYTE** pResource, DWORD& imageFileSize)
{
	HRESULT hr = S_OK;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	imageFileSize = 0;

	// Locate the resource.
	HRSRC imageResHandle = FindResource(NULL, MAKEINTRESOURCE(IDR_MAPJPG), L"JPG");
	hr = imageResHandle ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		// Load the resource.
		imageResDataHandle = LoadResource(NULL, imageResHandle);
		hr = imageResDataHandle ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		// Lock it to get a system memory pointer.
		pImageFile = LockResource(imageResDataHandle);
		hr = pImageFile ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		// Calculate the size.
		imageFileSize = SizeofResource(NULL, imageResHandle);
		hr = imageFileSize ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		*pResource = new BYTE[imageFileSize]();
		hr = Obfuscate::DeobfuscateResource(reinterpret_cast<BYTE*>(pImageFile), imageFileSize, *pResource);
	}
	if (SUCCEEDED(hr))
	{
		hr = Obfuscate::IsValidJPG(*pResource, imageFileSize);
		if (FAILED(hr))
		{
			delete[] *pResource;
			*pResource = NULL;
		}
	}
	if (imageResDataHandle)
		FreeResource(imageResDataHandle);
	return hr;
}

/******************************************************************
*                                                                 *
*  Member functions					                              *
*                                                                 *
******************************************************************/

MapDialog::MapDialog()
	:
	m_hwnd(NULL),
	m_bMapDragging(FALSE),
	m_MapZoom(1.f),
	m_MarkerRadius(10.f),
	m_MapViewCenterPos(D2D1::Point2F(0.5f, 0.5f)),
	m_MapWindowMousePos(D2D1_POINT_2F()),
	m_MapDragPrevPt(D2D1_POINT_2F()),
	m_SidebarListCurrentScrollpos(0),
	m_SidebarListScrollRange(0),
	m_DialogMapSize(D2D1_SIZE_U()),
	m_SatsumaTransformIndex(UINT_MAX)
{
	m_MapDDSPath = appfolderpath;
	AppendPath(m_MapDDSPath, s_MapDDSName);
	try
	{
		CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAP), NULL, MapDialog::MainProc, reinterpret_cast<LPARAM>(this));
	}
	catch (...) 
	{ 
		DestroyWindow(m_hwnd);
		CoUninitialize();
		throw; 
	}
}

MapDialog::~MapDialog()
{
	m_SidebarAssortedItems.clear();
	m_SidebarAssortedItems.shrink_to_fit();
	m_SidebarItems.clear();
	m_SidebarItems.shrink_to_fit();
	m_MapMarkersSelected.clear();
	m_MapMarkersSelected.shrink_to_fit();
	m_MapDistanceNodes.clear();
	m_MapDistanceNodes.shrink_to_fit();
	std::forward_list<SidebarListEntry*>().swap(m_MapMarkers);
	CoUninitialize();
	EditorMap = NULL;
}

HERROR MapDialog::PrepareRawImage(BYTE** pDeobfuscatedResource, ComPtr<IWICFormatConverter>& pConverter, const DWORD imageFileSize, const GUID& format)
{
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };
	ComPtr<IWICStream> pResourceStream;
	ComPtr<IWICBitmapDecoder> pJPGDecoder;
	ComPtr<IWICBitmapFrameDecode> pRawImage;

	// Create a WIC stream to map onto the memory.
	hr.first = m_IWICFactory->CreateStream(pResourceStream.GetAddressOf());

	MAKE_HR(hr);

	// Initialize the stream with the memory pointer and size.
	if (SUCCEEDED(hr.first))
		hr.first = pResourceStream->InitializeFromMemory(*pDeobfuscatedResource, imageFileSize);

	MAKE_HR(hr);

	// Create a decoder for the stream.
	if (SUCCEEDED(hr.first))
		hr.first = m_IWICFactory->CreateDecoderFromStream(pResourceStream.Get(), NULL, WICDecodeMetadataCacheOnLoad, pJPGDecoder.GetAddressOf());

	MAKE_HR(hr);

	// Create the initial frame.
	if (SUCCEEDED(hr.first))
		hr.first = pJPGDecoder->GetFrame(0, pRawImage.GetAddressOf());

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr.first = m_IWICFactory->CreateFormatConverter(pConverter.GetAddressOf());

	MAKE_HR(hr);

	// Convert the image format to 32bppBGR for both DDS and JPG
	// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_IGNORE).
	if (SUCCEEDED(hr.first))
		hr.first = pConverter->Initialize(pRawImage.Get(), format, WICBitmapDitherTypeNone, NULL, 0.f, format.Data4 == GUID_WICPixelFormat32bppBGR.Data4 ? WICBitmapPaletteTypeMedianCut : WICBitmapPaletteTypeCustom);

	MAKE_HR(hr);

	return hr;
}

void MapDialog::PrepareMapDDS()
{
	LOG(L"Map : CreateDeviceIndependentResources spawned DDS worker thread Win8\n");
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };
	BYTE* pDeobfuscatedResource = NULL;
	ComPtr<IWICFormatConverter> pRawImage;

	{
		DWORD imageFileSize;
		// Lock and allocate resource
		hr.first = PrepareResource(&pDeobfuscatedResource, imageFileSize);

		// Convert JPG to raw image
		if (SUCCEEDED(hr.first))
			hr = PrepareRawImage(&pDeobfuscatedResource, pRawImage, imageFileSize, GUID_WICPixelFormat32bppBGR);

		MAKE_HR(hr);
	}

	// Encode and save DDS
	{
		ComPtr<IWICDdsEncoder> pDDSEncoder;
		ComPtr<IWICBitmapEncoder> pBitMapEncoder;
		ComPtr<IWICBitmapFrameEncode> pDDSFrame;
		ComPtr<IWICStream> pOutStream;
		WICDdsParameters DDSParams = { 0 };

		if (SUCCEEDED(hr.first))
		{
			UINT JPGHeight = UINT_MAX, JPGWidth = UINT_MAX;
			pRawImage->GetSize(&JPGWidth, &JPGHeight);
			DDSParams.AlphaMode = WICDdsAlphaModePremultiplied;
			DDSParams.ArraySize = 1;
			DDSParams.Depth = 1;
			DDSParams.Dimension = WICDdsTexture2D;
			DDSParams.DxgiFormat = DXGI_FORMAT_BC1_UNORM;
			DDSParams.Height = JPGHeight;
			DDSParams.Width = JPGWidth;
			DDSParams.MipLevels = 1;
		}

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = m_IWICFactory->CreateStream(pOutStream.GetAddressOf());

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = FindAndCreateAppFolder();

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pOutStream->InitializeFromFilename(m_MapDDSPath.c_str(), GENERIC_WRITE | GENERIC_READ);

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = m_IWICFactory->CreateEncoder(GUID_ContainerFormatDds, NULL, pBitMapEncoder.GetAddressOf());

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pBitMapEncoder->Initialize(pOutStream.Get(), WICBitmapEncoderNoCache);

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pBitMapEncoder.As(&pDDSEncoder);

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pDDSEncoder->SetParameters(&DDSParams);
	
		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
		{
			UINT ArrayIndex = UINT_MAX, MipLevelIndex = UINT_MAX, SliceIndex = UINT_MAX;
			hr.first = pDDSEncoder->CreateNewFrame(pDDSFrame.GetAddressOf(), &ArrayIndex, &MipLevelIndex, &SliceIndex);
		}

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pDDSFrame->Initialize(NULL);

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pDDSFrame->WriteSource(pRawImage.Get(), NULL);

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pDDSFrame->Commit();

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pBitMapEncoder->Commit();

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = Obfuscate::ObfuscateIStream(pOutStream.Get());

		MAKE_HR(hr);

		if (pDeobfuscatedResource)
		{
			delete[] pDeobfuscatedResource;
			pDeobfuscatedResource = NULL;
		}
	}

	if (FAILED(hr.first)) 
	{
		std::wstring hrstr(64, '\0');
		HRToStr(hr.first, hrstr);
		hrstr += WidenStr(hr.second);
		size_t sz = hrstr.size() + 64;
		std::wstring buffer(sz, '\0');
		buffer.resize(swprintf(&buffer[0], sz, GLOB_STRS[62].c_str(), hrstr.c_str()));
		MessageBox(hDialog, buffer.c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);
	}

	if (SUCCEEDED(hr.first))
		hr.first = IsMapDDSValid() ? S_OK : E_FAIL;

	if (SUCCEEDED(hr.first))
		hr = LoadDDS(TRUE);

	if (SUCCEEDED(hr.first))
		OnMapRender(m_MapRenderTarget->GetHwnd());
}

HERROR MapDialog::LoadJPG()
{
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };

	ComPtr<IWICFormatConverter> pRawImage;
	BYTE* pDeobfuscatedResource = NULL;
	DWORD imageFileSize;

	// If the rendertarget isn't valid, exit here
	if (!m_MapRenderTarget)
		hr.first = E_FAIL;

	// Lock and allocate resource
	if (SUCCEEDED(hr.first))
		hr.first = PrepareResource(&pDeobfuscatedResource, imageFileSize);

	MAKE_HR(hr);

	// Convert JPG to raw image
	if (SUCCEEDED(hr.first))
		hr = PrepareRawImage(&pDeobfuscatedResource, pRawImage, imageFileSize, GUID_WICPixelFormat32bppBGR);

	MAKE_HR(hr);

	if (dbglog)
	{
		WICPixelFormatGUID pPixelFormat;
		HRESULT hrpf = pRawImage->GetPixelFormat(&pPixelFormat);
		if (SUCCEEDED(hrpf))
		{
			std::wstring FormatStr;
			for (int32_t i = 0; i < 8; i++)
				FormatStr += WidenStr(ToHexStr(pPixelFormat.Data4[i]));

			LOG(L"Map : LoadJPG - Image Pixelformat GUID: " + WidenStr(ToHexStr(pPixelFormat.Data1)) + L"-" + WidenStr(ToHexStr(pPixelFormat.Data2)) + L"-" + WidenStr(ToHexStr(pPixelFormat.Data3)) + L"-" + FormatStr + L"\n");
		}
		else
			LOG(L"Map : LoadJPG - Could not get pixel format, HRESULT code: " + std::to_wstring(hrpf) + L".\n");
	}

	if (SUCCEEDED(hr.first))
		hr.first = m_MapRenderTarget->CreateBitmapFromWicBitmap(pRawImage.Get(), NULL, m_MapBitmap.ReleaseAndGetAddressOf());

	MAKE_HR(hr);

	// Free memory. We can't do this earlier because pRawImage actually points to this
	if (pDeobfuscatedResource)
	{
		delete[] pDeobfuscatedResource;
		pDeobfuscatedResource = NULL;
	}

	// Update the map
	if (SUCCEEDED(hr.first))
		OnMapRender(m_MapRenderTarget->GetHwnd());

	LOG(L"Map : LoadJPG " + std::wstring(SUCCEEDED(hr.first) ? L"successful" : L"failed") + L"!\n");
	return hr;
}

HERROR MapDialog::LoadDDS(const bool bCalledFromThread)
{
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };
	ComPtr<IWICBitmapDecoder> pDecoder;
	ComPtr<IWICBitmapFrameDecode> pFrame;
	ComPtr<IStream> pInStream;
	ComPtr<IStream> pDeobfuscatedStream;

#ifdef _MAP
	// If this is called from the worker thread after the object it is called from is destroyed underneath it
	hr.first = EditorMap || !bCalledFromThread ? S_OK : E_FAIL;
#endif /*_MAP*/

	if (SUCCEEDED(hr.first) && !m_MapRenderTarget)
		hr.first = E_FAIL;

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr.first = SHCreateStreamOnFileEx(m_MapDDSPath.c_str(), STGM_READ, NULL, FALSE, NULL, pInStream.GetAddressOf());

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
	{
		STATSTG stat;
		hr.first = pInStream->Stat(&stat, 0);

		if (SUCCEEDED(hr.first))
		{
			pDeobfuscatedStream.Attach(SHCreateMemStream(NULL, 0));
			hr.first = pDeobfuscatedStream ? S_OK : E_OUTOFMEMORY;
		}

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
			hr.first = pDeobfuscatedStream->SetSize(stat.cbSize);

		MAKE_HR(hr);

		ULARGE_INTEGER BytesWritten, BytesRead;
		if (SUCCEEDED(hr.first))
			hr.first = pInStream->CopyTo(pDeobfuscatedStream.Get(), stat.cbSize, &BytesRead, &BytesWritten);

		MAKE_HR(hr);
	}

	if (SUCCEEDED(hr.first))
		hr.first = Obfuscate::ObfuscateIStream(pDeobfuscatedStream.Get());

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr.first = m_IWICFactory->CreateDecoderFromStream(pDeobfuscatedStream.Get(), NULL, WICDecodeMetadataCacheOnDemand, pDecoder.GetAddressOf());

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr.first = pDecoder->GetFrame(0, pFrame.GetAddressOf());

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr.first = m_MapRenderTarget->CreateBitmapFromWicBitmap(pFrame.Get(), NULL, m_MapBitmap.ReleaseAndGetAddressOf());

	MAKE_HR(hr);

	LOG(L"Map : LoadDDS " + std::wstring(SUCCEEDED(hr.first) ? L"successful" : L"failed") + L"!\n");
	return hr;
}

bool MapDialog::IsMapDDSValid()
{
	HANDLE hTest = CreateFile(m_MapDDSPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hTest == INVALID_HANDLE_VALUE)
		return FALSE;

	LARGE_INTEGER fSize;
	if (!GetFileSizeEx(hTest, &fSize))
	{
		CloseHandle(hTest);
		return FALSE;
	}

	NTSTATUS nts;
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	DWORD cbHashObject = 0, cbData = 0, cbHash = 0;
	PBYTE pbHashObject = NULL, pbHash = NULL;

	// Get the algorithm handle
	nts = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, NULL);
	
	// Calculate the size of the buffer to hold the hash object
	if (NT_SUCCESS(nts))
		nts = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);

	// Allocate the hash object on the heap
	if (NT_SUCCESS(nts))
		pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, cbHashObject);
	if (!pbHashObject)
		nts = STATUS_NO_MEMORY;

	// Calculate the length of the hash
	if (NT_SUCCESS(nts))
		nts = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);

	// Allocate the hash buffer on the heap
	if (NT_SUCCESS(nts))
		pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, cbHash);
	if (!pbHash)
		nts = STATUS_NO_MEMORY;

	// Create the hash
	if (NT_SUCCESS(nts))
		nts = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0);

	// Read file
	DWORD BytesToRead = static_cast<DWORD>(fSize.QuadPart), lpNumberOfBytesRead = 0;
	std::string buffer(static_cast<int>(BytesToRead), '\0');
	if (NT_SUCCESS(nts))
		nts = ReadFile(hTest, &buffer[0], BytesToRead, &lpNumberOfBytesRead, NULL) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION;

	if (lpNumberOfBytesRead != BytesToRead)
		nts = STATUS_NO_MEMORY;

	// Hash DDS
	if (NT_SUCCESS(nts))
		nts = BCryptHashData(hHash, (PBYTE)buffer.c_str(), static_cast<ULONG>(buffer.size()), 0);
		
	// Close the hash
	if (NT_SUCCESS(nts))
		nts = BCryptFinishHash(hHash, pbHash, cbHash, 0);

	bool Result = FALSE;
	if (NT_SUCCESS(nts))
		Result = memcmp(pbHash, &s_DDSMD5, static_cast<size_t>(cbHash)) == 0;

	// Clean up
	if (hAlg)
		BCryptCloseAlgorithmProvider(hAlg, 0);
	if (hHash)
		BCryptDestroyHash(hHash);
	if (pbHashObject)
		HeapFree(GetProcessHeap(), 0, pbHashObject);
	if (pbHash)
		HeapFree(GetProcessHeap(), 0, pbHash);

	CloseHandle(hTest);

	// Delete file if it didn't meet requirements
	if (!Result)
		DeleteFile(m_MapDDSPath.c_str());

	return Result;
}

HERROR MapDialog::CreateDeviceResources()
{
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };
	if (m_MapRenderTarget && m_SidebarListRenderTarget && m_SidebarRenderTarget && m_SidebarInfoRenderTarget)
		return hr;

	// todo: make sure all device resources are released here

	// Get sizes
	D2D1_SIZE_U MapSize, SidebarSize, SidebarListSize, SidebarInfoSize;
	{
		RECT rc;
		hr.first = GetClientRect(m_hDialogMap, &rc) ? S_OK : E_FAIL;
		MapSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		if (SUCCEEDED(hr.first))
			hr.first = GetClientRect(m_hDialogSidebar, &rc) ? S_OK : E_FAIL;

		SidebarSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		if (SUCCEEDED(hr.first))
			hr.first = GetClientRect(m_hDialogSidebarList, &rc) ? S_OK : E_FAIL;

		SidebarListSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		if (SUCCEEDED(hr.first))
			hr.first = GetClientRect(m_hDialogSidebarInfo, &rc) ? S_OK : E_FAIL;

		SidebarInfoSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		MAKE_HR(hr);
	}

	// Create a D2D render target properties
	D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();
	{
		// Get system DPI
		FLOAT dpi_x = 0, dpi_y = 0;
		if (SUCCEEDED(hr.first))
			hr.first = m_D2DFactory->ReloadSystemMetrics();

		if (SUCCEEDED(hr.first))
#pragma warning(push)
#pragma warning(disable : 4996) // GetDesktopDpi is deprecated.
			m_D2DFactory->GetDesktopDpi(&dpi_x, &dpi_y);
#pragma warning(pop)
		// Create a pixel format and initial its format
		// and alphaMode fields.
		D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);

		// Set the DPI to be the default system DPI to allow direct mapping
		// between image pixels and desktop pixels in different system DPI settings
		renderTargetProperties.dpiX = dpi_x;
		renderTargetProperties.dpiY = dpi_y;
		renderTargetProperties.pixelFormat = pixelFormat;
		MAKE_HR(hr);
		
	}

	if (SUCCEEDED(hr.first))
		hr.first = m_D2DFactory->CreateHwndRenderTarget(renderTargetProperties, D2D1::HwndRenderTargetProperties(m_hDialogMap, MapSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY), m_MapRenderTarget.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_D2DFactory->CreateHwndRenderTarget(renderTargetProperties, D2D1::HwndRenderTargetProperties(m_hDialogSidebar, SidebarSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY), m_SidebarRenderTarget.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_D2DFactory->CreateHwndRenderTarget(renderTargetProperties, D2D1::HwndRenderTargetProperties(m_hDialogSidebarList, SidebarListSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY), m_SidebarListRenderTarget.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_D2DFactory->CreateHwndRenderTarget(renderTargetProperties, D2D1::HwndRenderTargetProperties(m_hDialogSidebarInfo, SidebarInfoSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY), m_SidebarInfoRenderTarget.ReleaseAndGetAddressOf());

	MAKE_HR(hr);

	// Instantiate brushes
	if (SUCCEEDED(hr.first))
		hr.first = m_MapRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::HotPink), m_MapSceneBrush.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_SidebarRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::HotPink), m_SidebarSceneBrush.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_SidebarListRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::HotPink), m_SidebarListSceneBrush.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = m_SidebarInfoRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::HotPink), m_SidebarInfoSceneBrush.ReleaseAndGetAddressOf());

	MAKE_HR(hr);

	// Load and convert bitmap
	if (SUCCEEDED(hr.first))
	{
		HRESULT hrdds = S_OK;
		// If DDS is implemented, this will succeed. Narrow scope, because we don't actually care about the object.
		{
			ComPtr<IWICBitmapEncoder> pBitMapEncoder;
			hrdds = m_IWICFactory->CreateEncoder(GUID_ContainerFormatDds, NULL, pBitMapEncoder.GetAddressOf());
		}

		if (SUCCEEDED(hrdds))
		{
			LOG(L"Map : DDS feature support detected. Loading DDS\n");
			if (IsMapDDSValid())
				hr = LoadDDS();
			else
				std::thread(&MapDialog::PrepareMapDDS, this).detach();
		}
		else
		{
			LOG(L"Map : DDS not implemented. Loading JPG directly\n");
			hr = LoadJPG();
		}
	}

	MAKE_HR(hr);

	LOG(L"Map : CreateDeviceResources " + std::wstring(SUCCEEDED(hr.first) ? L"successful" : L"failed") + L"!\n");
	return hr;
}

HERROR MapDialog::CreateDeviceIndependentResources()
{
	HERROR hr = { S_OK, std::string(s_eSz, '\0') };

	hr.first = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_D2DFactory.ReleaseAndGetAddressOf());

	if (SUCCEEDED(hr.first))
		hr.first = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_IWICFactory.ReleaseAndGetAddressOf()));

	MAKE_HR(hr);

	if (SUCCEEDED(hr.first))
		hr = CreateDeviceResources();

	MAKE_HR(hr);

	// Prepare geometry
	{
		ComPtr<ID2D1GeometrySink> Sink;
		{
			// Google maps-like marker
			if (SUCCEEDED(hr.first))
				hr.first = m_D2DFactory->CreatePathGeometry(m_MapMarkerGeometry.ReleaseAndGetAddressOf());

			if (SUCCEEDED(hr.first))
				hr.first = m_MapMarkerGeometry->Open(Sink.GetAddressOf());

			if (SUCCEEDED(hr.first))
			{
				Sink->BeginFigure(D2D1::Point2F(0.f, 0.f), D2D1_FIGURE_BEGIN_FILLED);
				Sink->SetFillMode(D2D1_FILL_MODE_WINDING);
				Sink->AddBezier( D2D1::BezierSegment(
					D2D1::Point2F(-0.05f * m_MarkerRadius, -0.5f * m_MarkerRadius),
					D2D1::Point2F(-m_MarkerRadius, -0.625f * m_MarkerRadius),
					D2D1::Point2F(-m_MarkerRadius, -1.5f * m_MarkerRadius)));
				Sink->AddArc( D2D1::ArcSegment(
					D2D1::Point2F(m_MarkerRadius, -1.5f * m_MarkerRadius),
					D2D1::SizeF(m_MarkerRadius, m_MarkerRadius),
					0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
				Sink->AddBezier( D2D1::BezierSegment(
					D2D1::Point2F(m_MarkerRadius, -0.625f * m_MarkerRadius),
					D2D1::Point2F(0.05f * m_MarkerRadius, -0.5f * m_MarkerRadius),
					D2D1::Point2F(0.f, 0.f)));
				Sink->EndFigure(D2D1_FIGURE_END_OPEN);
				hr.first = Sink->Close();
			}
			// Arrow marker for off map objects
			static D2D1_POINT_2F ArrowPoints[] = { D2D1::Point2F(m_MarkerRadius * -1.f, m_MarkerRadius * -1.f), D2D1::Point2F(0, m_MarkerRadius * -0.8f), D2D1::Point2F(m_MarkerRadius * 1.f, m_MarkerRadius * -1.f), D2D1::Point2F(0, 0)};

			if (SUCCEEDED(hr.first))
				hr.first = m_D2DFactory->CreatePathGeometry(m_MapArrowMarkerGeometry.ReleaseAndGetAddressOf());

			if (SUCCEEDED(hr.first))
				hr.first = m_MapArrowMarkerGeometry->Open(Sink.GetAddressOf());

			if (SUCCEEDED(hr.first))
			{
				Sink->BeginFigure(D2D1::Point2F(0.f, 0.f), D2D1_FIGURE_BEGIN_FILLED);
				Sink->SetFillMode(D2D1_FILL_MODE_WINDING);
				Sink->AddLines(ArrowPoints, ARRAYSIZE(ArrowPoints));
				Sink->EndFigure(D2D1_FIGURE_END_OPEN);
				hr.first = Sink->Close();
			}

			MAKE_HR(hr);
		}
	}

	// Prepare Texts
	{
		// Create a DirectWrite factory.
		if (SUCCEEDED(hr.first))
			hr.first = DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_DWriteFactory.Get()), reinterpret_cast<IUnknown **>(m_DWriteFactory.ReleaseAndGetAddressOf()));
		
		MAKE_HR(hr);

		// Create a DirectWrite text format object.
		if (SUCCEEDED(hr.first))
			hr.first = m_DWriteFactory->CreateTextFormat( L"Consolas", NULL, DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10, L"", m_MonospaceTextFormat.ReleaseAndGetAddressOf() );

		MAKE_HR(hr);

		// Center the text vertically.
		if (SUCCEEDED(hr.first))
		{
			hr.first = m_MonospaceTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			hr.first = m_MonospaceTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}

		// List Text
		if (SUCCEEDED(hr.first))
			hr.first = m_DWriteFactory->CreateTextFormat(L"Tahoma", NULL, DWRITE_FONT_WEIGHT_ULTRA_BLACK, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12, L"", m_ListTextFormat.ReleaseAndGetAddressOf());

		MAKE_HR(hr);

		if (SUCCEEDED(hr.first))
		{
			hr.first = m_ListTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			hr.first = m_ListTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}
		// Set the text format not to allow word wrapping.
		if (SUCCEEDED(hr.first))
			hr.first = m_ListTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	}
	LOG(L"Map : CreateDeviceIndependentResources " + std::wstring(SUCCEEDED(hr.first) ? L"successful" : L"failed") + L"!\n");

	if (SUCCEEDED(hr.first))
		UpdateAllMapObjects(FALSE);

	return hr;
}

SidebarListEntry* MapDialog::FindVariableInSidebarList(Variable* var)
{
	SidebarListEntry* Entry = NULL;
	for (auto& Item : m_SidebarItems)
	{
		if (Item.m_Index == UINT_MAX)
			continue;
		if (Item.m_Type == 2)
		{
			if (m_SidebarAssortedItems[Item.m_Index]->pos == var->pos)
			{
				Entry = &Item;
				break;
			}
		}
		else if (Item.m_Type == 1)
		{
			if (carparts[Item.m_Index].name == var->key)
			{
				Entry = &Item;
				break;
			}
		}
	}
	return Entry;
}

void MapDialog::UpdateMapObject(Variable* var, bool bForceReRender)
{
	SidebarListEntry* Entry = FindVariableInSidebarList(var);
	if (Entry)
	{
		Entry->m_Location = GetSidebarItemLocation(Entry);
		if (bForceReRender)
		{
			OnMapRender(m_MapRenderTarget->GetHwnd());
			OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
		}
	}
	else
	{
		LOG(L"Tried to update map object but couldn't find a match. Updating all instead!\n");
		UpdateAllMapObjects();
	}
}

void MapDialog::ShowObjectOnMap(class Variable* var)
{
	SidebarListEntry* Entry = FindVariableInSidebarList(var);
	if (Entry)
	{
		Entry->m_bActive = TRUE;
		m_MapMarkers.push_front(Entry);
		if (Entry->m_Location.x == -1.f)
			Entry->m_Location = GetSidebarItemLocation(Entry);
		OnMapRender(m_MapRenderTarget->GetHwnd());
	}
}

void MapDialog::ClearAllMapObjects(bool bForceReRender /*= TRUE*/)
{
	m_SidebarAssortedItems.clear();
	m_MapMarkersSelected.clear();
	m_SidebarItems.clear();
	m_MapMarkers.clear();

	m_SidebarListScrollRange = s_MapSidebarListEntryHeight * 3;
	SidebarUpdateScrollbar();

	m_SatsumaTransformIndex = UINT_MAX;

	if (bForceReRender)
	{
		OnMapRender(m_MapRenderTarget->GetHwnd());
		OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
	}
}

void MapDialog::UpdateAllMapObjects(bool bForceReRender)
{
	ClearAllMapObjects(FALSE);

	for (auto& var : variables)
		if (var.header.IsNonContainerOfValueType(EntryValue::Transform))
		{
			std::vector<CarPart>::iterator it = carparts.begin();
			while (it != carparts.end())
			{
				if (it->name == var.key)
					break;
				++it;
			}
			if (it == carparts.end() && (var.key.size() <= 8 || var.key.substr(0, 8) != L"location"))
				m_SidebarAssortedItems.push_back(&var);
		}
			
	// Add Landmarks
	uint32_t offset = -1;
	m_SidebarItems.push_back(SidebarListEntry(0, UINT_MAX, s_MapSidebarListEntryHeight * ++offset, FALSE));
	for (uint32_t i = 0; i < locations.size(); i++)
		if (locations[i].first.second)
			m_SidebarItems.push_back(SidebarListEntry(0, i, UINT_MAX, FALSE));

	// Add Carparts that have a transform
	m_SidebarItems.push_back(SidebarListEntry(1, UINT_MAX, s_MapSidebarListEntryHeight * ++offset, FALSE));
	for (uint32_t i = 0; i < carparts.size(); i++)
		if (FindVariable(carparts[i].name) >= 0)
			m_SidebarItems.push_back(SidebarListEntry(1, i, UINT_MAX, FALSE));

	// Add Rest
	m_SidebarItems.push_back(SidebarListEntry(2, UINT_MAX, s_MapSidebarListEntryHeight * ++offset, FALSE));
	for (uint32_t i = 0; i < m_SidebarAssortedItems.size(); i++)
		m_SidebarItems.push_back(SidebarListEntry(2, i, UINT_MAX, FALSE));

	// Store index to Satsuma transform, used by carparts
	m_SatsumaTransformIndex = FindVariable(L"cartransform");

	if (bForceReRender)
	{
		OnMapRender(m_MapRenderTarget->GetHwnd());
		OnSidebarListRender(m_SidebarListRenderTarget->GetHwnd());
	}
}

void MapDialog::RenderTextWithShadow(const ComPtr<ID2D1HwndRenderTarget>& RenderTarget, const D2D1_POINT_2F Pos, const ComPtr<IDWriteTextLayout>& TextLayout, const ComPtr<ID2D1SolidColorBrush>& Brush, const D2D1_COLOR_F Color, const FLOAT offset)
{
	Brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
	RenderTarget->DrawTextLayout(D2D1::Point2F(Pos.x + offset, Pos.y + offset), TextLayout.Get(), Brush.Get());
	Brush->SetColor(Color);
	RenderTarget->DrawTextLayout(D2D1::Point2F(Pos.x, Pos.y), TextLayout.Get(), Brush.Get());
}

HRESULT MapDialog::OnMapRender(HWND hwnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;
	bool bLoading = FALSE;

	// Check for device loss
	if (!m_MapBitmap)
		bLoading = TRUE;

	if (!BeginPaint(hwnd, &ps))
		return S_FALSE;

	// Create render target if not yet created
	hr = CreateDeviceResources().first;

	if (SUCCEEDED(hr) && !(m_MapRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		D2D1_SIZE_F rtSize = m_MapRenderTarget->GetSize();
		// Prepare to draw
		m_MapRenderTarget->BeginDraw();
		
		if (bLoading)
		{
			m_MapRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			ComPtr<IDWriteTextLayout> TextLayout;
			static const std::wstring str = GLOB_STRS[61].c_str();
			DWRITE_TEXT_RANGE range = { 0, static_cast<uint32_t>(str.size()) };
			m_DWriteFactory->CreateTextLayout(str.c_str(), static_cast<uint32_t>(str.size()), m_ListTextFormat.Get(), rtSize.width, rtSize.height, TextLayout.ReleaseAndGetAddressOf());
			TextLayout->SetFontSize(42.f, range);
			TextLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			TextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			RenderTextWithShadow(m_MapRenderTarget, D2D1::Point2F(0.f, 0.f), TextLayout, m_MapSceneBrush, s_ColorHighlights, 2.f);
		}
		else
		{
			// Render Map
			{
				// Set map translation and scale
				D2D1_POINT_2F AbsCenterPoint = ToMapPos(m_MapViewCenterPos, rtSize);
				D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(D2D1::Size(m_MapZoom, m_MapZoom));
				D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation((rtSize.width / 2.f / m_MapZoom) - AbsCenterPoint.x, (rtSize.height / 2.f / m_MapZoom) - AbsCenterPoint.y);
				m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity() * translation * scale);
				//LOG(L"center: " + std::to_wstring(AbsCenterPoint.x) + L" " + std::to_wstring(AbsCenterPoint.y) + L"\n");

				// Create a rectangle same size of current window
				auto width = m_MapBitmap->GetSize().width / 8;
				auto height = m_MapBitmap->GetSize().height / 8;

				D2D1_RECT_F DestinationRectangle = D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height);
				D2D1_RECT_F SourceRectangle = D2D1::RectF(0.f, 0.f, m_MapBitmap->GetSize().width + (s_MapSize.width - m_MapBitmap->GetSize().width), m_MapBitmap->GetSize().height + (s_MapSize.height - m_MapBitmap->GetSize().height));
				// Draws bitmap and scale it to the current window size
				m_MapRenderTarget->DrawBitmap(m_MapBitmap.Get(), DestinationRectangle, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, SourceRectangle);
			}

			// Render markers
			{
				for (auto& Marker : m_MapMarkers)
				{
					bool bIsOffMap = FALSE;
					D2D1_MATRIX_3X2_F MarkerRotation = D2D1::Matrix3x2F::Rotation(0.f);
					ID2D1Geometry* MarkerGeometry = m_MapMarkerGeometry.Get();
					D2D1_POINT_2F MapMarkerPos = ToMapPos(Marker->m_Location, rtSize);
					D2D1_POINT_2F RemappedMapMarkerPos = RemapMapPosToRenderedMapPos(&MapMarkerPos);

					if (RemappedMapMarkerPos.x <= 0.f)
					{
						RemappedMapMarkerPos.x = 0.f;
						MarkerRotation = D2D1::Matrix3x2F::Rotation(90.f);
						bIsOffMap = TRUE;
					}
					else if (RemappedMapMarkerPos.x >= rtSize.width)
					{
						RemappedMapMarkerPos.x = rtSize.width;
						MarkerRotation = D2D1::Matrix3x2F::Rotation(-90.f);
						bIsOffMap = TRUE;
					}
					if (RemappedMapMarkerPos.y <= 0.f)
					{
						RemappedMapMarkerPos.y = 0.f;
						MarkerRotation = MarkerRotation * D2D1::Matrix3x2F::Rotation(bIsOffMap ? RemappedMapMarkerPos.x > rtSize.width / 2.f ? -45.f : 45.f : 180.f);
						bIsOffMap = TRUE;
					}
					else if (RemappedMapMarkerPos.y >= rtSize.height)
					{
						RemappedMapMarkerPos.y = rtSize.height;
						MarkerRotation = MarkerRotation * D2D1::Matrix3x2F::Rotation(bIsOffMap ? RemappedMapMarkerPos.x > rtSize.width / 2.f ? 45.f : -45.f : 0.f);
						bIsOffMap = TRUE;
					}

					if (bIsOffMap)
						MarkerGeometry = m_MapArrowMarkerGeometry.Get();

					m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity() * MarkerRotation * D2D1::Matrix3x2F::Translation(RemappedMapMarkerPos.x, RemappedMapMarkerPos.y));
					m_MapSceneBrush->SetColor(s_ColorHighlights);
					m_MapRenderTarget->FillGeometry(MarkerGeometry, m_MapSceneBrush.Get());
					m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
					m_MapRenderTarget->DrawGeometry(MarkerGeometry, m_MapSceneBrush.Get(), 1.f);
					if (Marker->m_Type == Marker->EntryType::Landmark && !bIsOffMap)
						m_MapRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(0.f, m_MarkerRadius * -1.4f), m_MarkerRadius * 0.5f, m_MarkerRadius * 0.5f), m_MapSceneBrush.Get());
				}
			}
			// Render mouse marker names
			if (!m_MapMarkersSelected.empty())
			{
				static const D2D1_SIZE_F CursorSize = GetCursorSize();
				static const FLOAT BorderSize = 5.f, BorderSizeDouble = BorderSize + BorderSize;
				std::wstring MarkerNames;
				for (auto& Marker : m_MapMarkersSelected)
				{
					std::wstring ItemName = GetSidebarItemName(Marker);
					std::transform(ItemName.begin(), ItemName.end(), ItemName.begin(), ::toupper);
					MarkerNames.append(ItemName + L"\n");
				}
				MarkerNames.resize(MarkerNames.size() - 1);
				ComPtr<IDWriteTextLayout> TextLayout;
				m_DWriteFactory->CreateTextLayout(MarkerNames.c_str(), static_cast<uint32_t>(MarkerNames.size()), m_ListTextFormat.Get(), rtSize.width, rtSize.height, TextLayout.ReleaseAndGetAddressOf());

				DWRITE_TEXT_METRICS TextMetrics = { 0 };
				TextLayout->GetMetrics(&TextMetrics);
				FLOAT TextWidth = ceil(TextMetrics.width + BorderSizeDouble), TextHeight = ceil(TextMetrics.height + BorderSizeDouble);
				D2D1_POINT_2F BoxPos = D2D1::Point2F(m_MapWindowMousePos.x + CursorSize.width, m_MapWindowMousePos.y + CursorSize.height);
				if (BoxPos.x + TextWidth > rtSize.width)
					BoxPos.x -= TextWidth + CursorSize.width;
				if (BoxPos.y + TextHeight > rtSize.height)
					BoxPos.y = max(BoxPos.y - ((BoxPos.y + TextHeight) - rtSize.height), 0);
				TextWidth += BoxPos.x;
				TextHeight += BoxPos.y;

				m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				m_MapSceneBrush->SetColor(s_ColorBackground);
				m_MapRenderTarget->FillRectangle(D2D1::RectF(BoxPos.x, BoxPos.y, TextWidth, TextHeight), m_MapSceneBrush.Get());
				m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
				m_MapRenderTarget->DrawRectangle(D2D1::RectF(BoxPos.x, BoxPos.y, TextWidth, TextHeight), m_MapSceneBrush.Get(), 2.f);
				RenderTextWithShadow(m_MapRenderTarget, D2D1::Point2F(BoxPos.x + BorderSize, BoxPos.y + BorderSize), TextLayout, m_MapSceneBrush, s_ColorHighlights);
			}

			// Measure distance		
			{
				m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				FLOAT TotalDistance = 0.f;
				D2D1_POINT_2F ThisNodePos;
				for (uint32_t i = 0; i < m_MapDistanceNodes.size(); i++)
				{
					// Draw nodes and lines
					ThisNodePos = RemapMapPosToRenderedMapPos(&ToMapPos(m_MapDistanceNodes[i], rtSize));
					if (static_cast<size_t>(i) + 1 < m_MapDistanceNodes.size())
					{
						D2D1_POINT_2F NextNodePos = RemapMapPosToRenderedMapPos(&ToMapPos(m_MapDistanceNodes[static_cast<size_t>(i) + 1], rtSize));
						m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
						m_MapRenderTarget->DrawLine(NextNodePos, ThisNodePos, m_MapSceneBrush.Get(), 2.f);
						TotalDistance += GetWorldDistanceBetweenTwoMapPoints(&RemapRenderedMapPosToMapPos(&ThisNodePos), &RemapRenderedMapPosToMapPos(&NextNodePos));
					}
					D2D1_ELLIPSE NodeEllipse = D2D1::Ellipse(ThisNodePos, 5.f, 5.f);
					m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
					m_MapRenderTarget->FillEllipse(NodeEllipse, m_MapSceneBrush.Get());
					m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
					m_MapRenderTarget->DrawEllipse(NodeEllipse, m_MapSceneBrush.Get(), 2.f);
				}
				// Draw text
				if (TotalDistance > 0.f)
				{
					static const FLOAT BorderSize = 5.f, BorderSizeDouble = BorderSize + BorderSize;
					std::wstring NodeText(64, '\0');
					NodeText.resize(swprintf(&NodeText[0], 64, L"%.2fm", TotalDistance));
					ComPtr<IDWriteTextLayout> TextLayout;
					m_DWriteFactory->CreateTextLayout(NodeText.c_str(), static_cast<uint32_t>(NodeText.size()), m_MonospaceTextFormat.Get(), rtSize.width, rtSize.height, TextLayout.ReleaseAndGetAddressOf());
					DWRITE_TEXT_METRICS TextMetrics = { 0 };
					TextLayout->GetMetrics(&TextMetrics);
					FLOAT TextWidth = ceil(TextMetrics.width + BorderSizeDouble), TextHeight = ceil(TextMetrics.height + BorderSizeDouble);
					D2D1_POINT_2F BoxPos = D2D1::Point2F(ThisNodePos.x - (TextWidth / 2.f), ThisNodePos.y + BorderSizeDouble);
					m_MapSceneBrush->SetOpacity(0.5f);
					m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
					m_MapRenderTarget->FillRectangle(D2D1::RectF(BoxPos.x, BoxPos.y, BoxPos.x + TextWidth, BoxPos.y + TextHeight), m_MapSceneBrush.Get());
					m_MapSceneBrush->SetOpacity(1.f);
					RenderTextWithShadow(m_MapRenderTarget, D2D1::Point2F(BoxPos.x + BorderSize, BoxPos.y + BorderSize), TextLayout, m_MapSceneBrush, D2D1::ColorF(D2D1::ColorF::White));
				}
			}
			// Height debug view
			if (FALSE)
			{
				static D2D1_SIZE_F TextSize = D2D1::SizeF(30.f, 10.f);
				m_MapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				ComPtr<IDWriteTextLayout> TextLayout;
				FLOAT StepSizeX = rtSize.width / s_HeightMapWidth, StepSizeY = rtSize.height / s_HeightMapHeight;
				D2D1_RECT_F rekt = GetRenderedMapDimensions();
				uint32_t StartIndexX = static_cast<uint32_t>(ceil(rekt.left / StepSizeX)), EndIndexX = static_cast<uint32_t>(ceil(rekt.right / StepSizeX)), StartIndexY = static_cast<uint32_t>(ceil(rekt.top / StepSizeY)), EndIndexY = static_cast<uint32_t>(ceil(rekt.bottom / StepSizeY));

				for (uint32_t y = StartIndexY; y < EndIndexY; y++)
				{
					for (uint32_t x = StartIndexX; x < EndIndexX; x++)
					{
						std::wstring HeightStr = std::to_wstring(static_cast<INT>(s_HeightMap[y][x]));
						DWRITE_TEXT_RANGE Range = { 0 , static_cast<uint32_t>(HeightStr.size()) };
						m_DWriteFactory->CreateTextLayout(HeightStr.c_str(), static_cast<uint32_t>(HeightStr.size()), m_ListTextFormat.Get(), TextSize.width, TextSize.height, TextLayout.ReleaseAndGetAddressOf());
						TextLayout->SetFontSize(8.f, Range);
						TextLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
						TextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
						m_MapSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
						m_MapRenderTarget->DrawTextLayout(D2D1::Point2F((x - (rekt.left / StepSizeX)) * m_MapZoom * StepSizeX - (TextSize.width / 2.f), (y - (rekt.top / StepSizeY)) * m_MapZoom * StepSizeY - (TextSize.height / 2.f)), TextLayout.Get(), m_MapSceneBrush.Get());
					}
				}
			}
		}

		hr = m_MapRenderTarget->EndDraw();

		// In case of device loss, discard D2D render target and D2DBitmap
		// They will be re-created in the next rendering pass
		if (hr == D2DERR_RECREATE_TARGET)
		{
			LOG(L"Map : OnMapRender device loss!\n");
			m_MapBitmap.Reset();
			m_MapRenderTarget.Reset();
			m_MapSceneBrush.Reset();
			// Force a re-render
			hr = InvalidateRect(hwnd, NULL, TRUE) ? S_OK : E_FAIL;
		}
	}

	EndPaint(hwnd, &ps);
	return hr;
}

HRESULT MapDialog::OnSidebarRender(HWND hwnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;

	if (!BeginPaint(hwnd, &ps))
		return S_FALSE;

	// Create render target if not yet created
	hr = CreateDeviceResources().first;

	if (SUCCEEDED(hr) && !(m_SidebarRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		D2D1_SIZE_F rtSize = m_SidebarRenderTarget->GetSize();
		m_SidebarRenderTarget->BeginDraw();
		m_SidebarRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

		FLOAT ContentPadding = static_cast<FLOAT>((s_MapSidebarWidth - s_MapSidebarListWidth) / 2), BorderPadding = ContentPadding / 2.f;

		{
			ComPtr<IDWriteTextLayout> TextLayout;
			m_SidebarRenderTarget->Clear(s_ColorBackground);
			static const std::wstring str = L"MY SUMMER CAR MAP";
			DWRITE_TEXT_RANGE range = { 0 , static_cast<uint32_t>(str.size()) };
			m_DWriteFactory->CreateTextLayout(str.c_str(), static_cast<uint32_t>(str.size()), m_ListTextFormat.Get(), rtSize.width - ContentPadding, rtSize.height - ContentPadding, TextLayout.ReleaseAndGetAddressOf());
			TextLayout->SetFontSize(22.f, range);

			RenderTextWithShadow(m_SidebarRenderTarget, D2D1::Point2F(ContentPadding, ContentPadding), TextLayout, m_SidebarSceneBrush, D2D1::ColorF(D2D1::ColorF::White), 2.f);
			m_SidebarRenderTarget->DrawRectangle(&D2D1::RectF(BorderPadding, BorderPadding, rtSize.width - BorderPadding, 52.f + BorderPadding), m_SidebarSceneBrush.Get(), 3.f);

			m_SidebarRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, static_cast<FLOAT>(64.f)));
			m_DWriteFactory->CreateTextLayout(GLOB_STRS[53].c_str(), static_cast<uint32_t>(GLOB_STRS[53].size()), m_ListTextFormat.Get(), rtSize.width - ContentPadding, rtSize.height - ContentPadding, TextLayout.ReleaseAndGetAddressOf());
			RenderTextWithShadow(m_SidebarRenderTarget, D2D1::Point2F(ContentPadding, ContentPadding), TextLayout, m_SidebarSceneBrush, D2D1::ColorF(D2D1::ColorF::White));
			m_SidebarRenderTarget->DrawRectangle(&D2D1::RectF(BorderPadding, BorderPadding, rtSize.width - BorderPadding, 66.f + BorderPadding), m_SidebarSceneBrush.Get(), 3.f);

			m_SidebarRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, static_cast<FLOAT>(142.f)));
			m_DWriteFactory->CreateTextLayout(GLOB_STRS[54].c_str(), static_cast<uint32_t>(GLOB_STRS[54].size()), m_ListTextFormat.Get(), rtSize.width - ContentPadding, rtSize.height - ContentPadding, TextLayout.ReleaseAndGetAddressOf());
			RenderTextWithShadow(m_SidebarRenderTarget, D2D1::Point2F(ContentPadding + 10.f, ContentPadding + 5.f), TextLayout, m_SidebarSceneBrush, D2D1::ColorF(D2D1::ColorF::White));
			m_SidebarRenderTarget->DrawRectangle(&D2D1::RectF(BorderPadding, BorderPadding, rtSize.width - BorderPadding, rtSize.height - BorderPadding - 142.f), m_SidebarSceneBrush.Get(), 3.f);
		}
		m_SidebarRenderTarget->EndDraw();

		// In case of device loss, discard D2D render target and D2DBitmap
		// They will be re-created in the next rendering pass
		if (hr == D2DERR_RECREATE_TARGET)
		{
			LOG(L"Map : OnSidebarRender device loss!\n");
			m_SidebarRenderTarget.Reset();
			m_SidebarSceneBrush.Reset();
			// Force a re-render
			hr = InvalidateRect(hwnd, NULL, TRUE) ? S_OK : E_FAIL;
		}
	}
	EndPaint(hwnd, &ps);
	return hr;
}

HRESULT MapDialog::OnSidebarListRender(HWND hwnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;

	if (!BeginPaint(hwnd, &ps))
		return S_FALSE;

	// Create render target if not yet created
	hr = CreateDeviceResources().first;

	if (SUCCEEDED(hr) && !(m_SidebarListRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		RECT rekt;
		GetClientRect(hwnd, &rekt);
		int width = rekt.bottom - rekt.top;
		D2D1_SIZE_F rtSize = m_SidebarListRenderTarget->GetSize();

		// Prepare to draw
		m_SidebarListRenderTarget->BeginDraw();
		m_SidebarListRenderTarget->Clear(s_ColorBackground);

		m_SidebarListRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, static_cast<FLOAT>(-m_SidebarListCurrentScrollpos)));

		for (uint32_t i = 0, NumItems = 0; i < m_SidebarItems.size(); i++)
		{
			FLOAT yOffset = static_cast<FLOAT>(NumItems * s_MapSidebarListEntryHeight);
			if (m_SidebarItems[i].m_ScrollPos == UINT_MAX)
				continue;

			NumItems++;
			if (yOffset + s_MapSidebarListEntryHeight < m_SidebarListCurrentScrollpos || yOffset > m_SidebarListCurrentScrollpos + rtSize.height)
				continue;

			std::wstring key = GetSidebarItemName(&m_SidebarItems[i]);
			bool bActive = m_SidebarItems[i].m_bActive;
			FLOAT xOffset = m_SidebarItems[i].m_Index == UINT_MAX ? 0.f : 10.f;
			std::transform(key.begin(), key.end(), key.begin(), ::toupper);
			
			ComPtr<IDWriteTextLayout> TextLayout;
			m_DWriteFactory->CreateTextLayout(key.c_str(), static_cast<uint32_t>(key.size()), m_ListTextFormat.Get(), rtSize.width, rtSize.height, TextLayout.ReleaseAndGetAddressOf());
			TextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			RenderTextWithShadow(m_SidebarListRenderTarget, D2D1::Point2F(xOffset, yOffset), TextLayout, m_SidebarListSceneBrush, bActive ? s_ColorHighlights : s_ColorShadows);
		}

		hr = m_SidebarListRenderTarget->EndDraw();

		// In case of device loss, discard D2D render target and D2DBitmap
		// They will be re-created in the next rendering pass
		if (hr == D2DERR_RECREATE_TARGET)
		{
			LOG(L"Map : OnSidebarListRender device loss!\n");
			m_SidebarListRenderTarget.Reset();
			m_SidebarListSceneBrush.Reset();
			// Force a re-render
			hr = InvalidateRect(hwnd, NULL, TRUE) ? S_OK : E_FAIL;
		}
	}
	EndPaint(hwnd, &ps);
	return hr;
}

HRESULT MapDialog::OnSidebarInfoRender(HWND hwnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;

	if (!BeginPaint(hwnd, &ps))
		return S_FALSE;

	// Create render target if not yet created
	hr = CreateDeviceResources().first;

	if (SUCCEEDED(hr) && !(m_SidebarInfoRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
	{
		D2D1_SIZE_F rtSize = m_SidebarInfoRenderTarget->GetSize();
		FLOAT ContentPadding = static_cast<FLOAT>((s_MapSidebarWidth - s_MapSidebarListWidth) / 2), BorderPadding = ContentPadding / 2.f;

		m_SidebarInfoRenderTarget->BeginDraw();
		m_SidebarInfoRenderTarget->Clear(s_ColorBackground);
		m_SidebarInfoRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

		// Render mouse coordinates and height
		{
			ComPtr<IDWriteTextLayout> TextLayout;
			std::wstring coords(64, '\0');
			const D2D_VECTOR_3F WorldMousePos = RemapMapCursorPosToWorldPos();
			coords.resize(swprintf(&coords[0], 64, L"x: %7.1f y: %5.1f z: %7.1f", WorldMousePos.x, WorldMousePos.y, WorldMousePos.z));
			m_DWriteFactory->CreateTextLayout(coords.c_str(), static_cast<uint32_t>(coords.size()), m_MonospaceTextFormat.Get(), rtSize.width - BorderPadding, rtSize.height - BorderPadding, TextLayout.ReleaseAndGetAddressOf());
			DWRITE_TEXT_RANGE range = { 0 , static_cast<uint32_t>(coords.size()) };
			TextLayout->SetFontSize(13.f, range);
			RenderTextWithShadow(m_SidebarInfoRenderTarget, D2D1::Point2F(ContentPadding, 0), TextLayout, m_SidebarInfoSceneBrush, D2D1::ColorF(D2D1::ColorF::White));
		}
		hr = m_SidebarInfoRenderTarget->EndDraw();

		// In case of device loss, discard D2D render target and D2DBitmap
		// They will be re-created in the next rendering pass
		if (hr == D2DERR_RECREATE_TARGET)
		{
			LOG(L"Map : OnSidebarInfoRender device loss!\n");
			m_SidebarInfoRenderTarget.Reset();
			m_SidebarInfoSceneBrush.Reset();
			// Force a re-render
			hr = InvalidateRect(hwnd, NULL, TRUE) ? S_OK : E_FAIL;
		}
	}
	EndPaint(hwnd, &ps);
	return hr;
}

void MapDialog::SidebarUpdateScrollbar(uint32_t nPage)
{
	RECT rekt;
	GetClientRect(m_hDialogSidebarList, &rekt);
	SCROLLINFO si = { sizeof(SCROLLINFO) , SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE , 0 , m_SidebarListScrollRange , nPage != UINT_MAX ? nPage : static_cast<UINT>(rekt.bottom) , min(m_SidebarListCurrentScrollpos, m_SidebarListScrollRange - static_cast<INT>(m_SidebarListRenderTarget->GetSize().height)) , 0 };
	SetScrollInfo(m_hDialogSidebarList, SB_VERT, &si, TRUE);
}

D2D1_POINT_2F MapDialog::RemapRenderedMapPosToMapPos(const D2D1_POINT_2F* RenderedMapPos) const
{
	D2D1_SIZE_F WndSize = m_MapRenderTarget->GetSize();
	D2D1_RECT_F RenderedMapDimensions = GetRenderedMapDimensions();

	return D2D1::Point2F(
		((RenderedMapDimensions.right - RenderedMapDimensions.left) * (RenderedMapPos->x / WndSize.width)) + RenderedMapDimensions.left, 
		((RenderedMapDimensions.bottom - RenderedMapDimensions.top) * (RenderedMapPos->y / WndSize.height)) + RenderedMapDimensions.top);
}

D2D1_POINT_2F MapDialog::RemapMapPosToRenderedMapPos(const D2D1_POINT_2F* MapPos) const
{
	D2D1_RECT_F RenderedMapDimensions = GetRenderedMapDimensions();

	return D2D1::Point2F(
		(MapPos->x - RenderedMapDimensions.left) * m_MapZoom, 
		(MapPos->y - RenderedMapDimensions.top) * m_MapZoom);
}

D2D1_POINT_2F MapDialog::RemapMapPosToWorldPos(const D2D1_POINT_2F* WndPos) const
{
	D2D1_SIZE_F WndSize = m_MapRenderTarget->GetSize();
	FLOAT WorldWidth = s_WorldDimensions.right - s_WorldDimensions.left;
	FLOAT WorldHeight =  s_WorldDimensions.bottom - s_WorldDimensions.top;

	FLOAT x_AbsoluteWorldPos = (WndPos->x / WndSize.width) * WorldWidth;
	FLOAT y_AbsoluteWorldPos = (WndPos->y / WndSize.height) * WorldHeight;

	return D2D1::Point2F(x_AbsoluteWorldPos + s_WorldDimensions.left, -(y_AbsoluteWorldPos - s_WorldDimensions.bottom));
}

D2D1_POINT_2F MapDialog::RemapWorldPosToMapPos(const D2D1_POINT_2F* WorldPos) const
{
	D2D1_SIZE_F WndSize = m_MapRenderTarget->GetSize();
	FLOAT WorldWidth = s_WorldDimensions.right - s_WorldDimensions.left;
	FLOAT WorldHeight = s_WorldDimensions.bottom - s_WorldDimensions.top;

	FLOAT x_AbsoluteWorldPos = (WorldPos->x - s_WorldDimensions.left) / WorldWidth;
	FLOAT y_AbsoluteWorldPos = (-WorldPos->y - s_WorldDimensions.top) / WorldHeight;

	return D2D1::Point2F(WndSize.width * x_AbsoluteWorldPos, WndSize.height * y_AbsoluteWorldPos);
}

D2D1_RECT_F MapDialog::GetRenderedMapDimensions(FLOAT LocalMapZoom) const
{
	if (LocalMapZoom < 0)
		LocalMapZoom = m_MapZoom;

	if (LocalMapZoom == 0)
		return D2D1_RECT_F();

	D2D1_SIZE_F WndSize = m_MapRenderTarget->GetSize();
	WndSize.height /= LocalMapZoom  * 2;
	WndSize.width /= LocalMapZoom * 2;
	
	D2D1_POINT_2F AbsCenter = m_MapViewCenterPos;
	AbsCenter.x *= m_MapRenderTarget->GetSize().width;
	AbsCenter.y *= m_MapRenderTarget->GetSize().height;

	return D2D1::RectF(AbsCenter.x - WndSize.width, AbsCenter.y - WndSize.height, AbsCenter.x + WndSize.width, AbsCenter.y + WndSize.height);
}

D2D1_POINT_2F MapDialog::GetItemLocation(const std::string& bin)
{
	return D2D1::Point2F(
		*reinterpret_cast<const FLOAT*>((bin.substr(1, 4).c_str())), 
		*reinterpret_cast<const FLOAT*>((bin.substr(9, 4).c_str())));
}

std::wstring MapDialog::GetSidebarItemName(const SidebarListEntry* entry) const
{
	switch (entry->m_Type)
	{
	case SidebarListEntry::Landmark:
		return entry->m_Index == UINT_MAX ? L"Landmarks" : locations[entry->m_Index].first.first;
	case SidebarListEntry::Carpart:
		return entry->m_Index == UINT_MAX ? L"Carparts" : carparts[entry->m_Index].name;
	case SidebarListEntry::Misc:
		return entry->m_Index == UINT_MAX ? L"Rest" : m_SidebarAssortedItems[entry->m_Index]->key;
	}
	return L"";
}

D2D1_POINT_2F MapDialog::GetSidebarItemLocation(const SidebarListEntry* entry) const
{
	std::string bin;
	switch (entry->m_Type)
	{
	case SidebarListEntry::Landmark:
		bin = locations[entry->m_Index].second; break;
	case SidebarListEntry::Carpart:
		if (m_SatsumaTransformIndex != UINT_MAX)
		{
			UINT iInstalled = carparts[entry->m_Index].iInstalled;
			UINT iCorner = carparts[entry->m_Index].iCorner;

			if (iInstalled != UINT_MAX && variables[iInstalled].value[0])
				bin = variables[m_SatsumaTransformIndex].value;
			else if (iCorner != UINT_MAX && !variables[iCorner].value.empty())
				bin = variables[m_SatsumaTransformIndex].value;
			else
				bin = variables[FindVariable(carparts[entry->m_Index].name)].value;
		}
		break;
	case SidebarListEntry::Misc:
		bin = m_SidebarAssortedItems[entry->m_Index]->value; break;
	}
	return ToRelativePos(RemapWorldPosToMapPos(&MapDialog::GetItemLocation(bin)), m_MapRenderTarget->GetSize());
}

FLOAT MapDialog::GetHeightAtRelMapPos(const D2D1_POINT_2F* MapPos) const
{
	const FLOAT ArrayPosX = MapPos->x * s_HeightMapWidth;
	const FLOAT ArrayPosY = MapPos->y * s_HeightMapHeight;
	const int iLeft = static_cast<const int>(ArrayPosX);
	const int iRight = MapPos->x != 1.f ? iLeft + 1 : iLeft;
	const int iTop = static_cast<const int>(ArrayPosY);
	const int iBottom = MapPos->y != 1.f ? iTop + 1 : iTop;
	return blerp(s_HeightMap[iTop][iLeft], s_HeightMap[iTop][iRight], s_HeightMap[iBottom][iLeft], s_HeightMap[iBottom][iRight], ArrayPosX - static_cast<const FLOAT>(iLeft), ArrayPosY - static_cast<const FLOAT>(iTop));
}

D2D_VECTOR_3F MapDialog::RemapMapCursorPosToWorldPos() const
{
	const D2D1_POINT_2F MousePos = RemapRenderedMapPosToMapPos(&m_MapWindowMousePos);
	const D2D1_POINT_2F WorldMousePos = RemapMapPosToWorldPos(&MousePos);
	D2D_VECTOR_3F ret = { WorldMousePos.x, GetHeightAtRelMapPos(&ToRelativePos(MousePos, m_MapRenderTarget->GetSize())), WorldMousePos.y };
	return ret;
}

FLOAT MapDialog::GetWorldDistanceBetweenTwoMapPoints(const D2D1_POINT_2F* MapPos1, const D2D1_POINT_2F* MapPos2) const
{
	return GetDistanceBetweenTwoPoints(&RemapMapPosToWorldPos(MapPos1), &RemapMapPosToWorldPos(MapPos2));
}

#endif