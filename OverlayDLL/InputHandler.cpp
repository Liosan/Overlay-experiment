#include "InputHandler.h"
#include "OverlayData.h"


InputHandler::InputHandler(WNDPROC const originalWndProc):
	originalWndProc(originalWndProc)
{
}
	
LRESULT CALLBACK InputHandler::customWindowProcForwarder(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return OverlayData::getSingleton()->inputHandler->customWindowProc(wnd, msg, wParam, lParam);
}

HRESULT WINAPI InputHandler::DIGetDeviceDataForwarder(
	IDirectInputDeviceW * pDevice,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags
)
{
	return OverlayData::getSingleton()->inputHandler->DIGetDeviceDataCustom(pDevice, cbObjectData, rgdod, pdwInOut, dwFlags);
}

LRESULT InputHandler::customWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{ // keyboard handling could be done on DIGetDeviceDataCustom, but it's simpler here
	case WM_KEYUP:
		if ((wParam == VK_SPACE) && (GetKeyState(VK_CONTROL) != 0))
		{
			OverlayData::getSingleton()->overlayEnabled = !OverlayData::getSingleton()->overlayEnabled;
		}
		break;
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_MOUSEMOVE:
		if (OverlayData::getSingleton()->overlayEnabled)
		{
			msg = WM_NULL;
		}
		break;
	}
	return CallWindowProc(originalWndProc, wnd, msg, wParam, lParam);
}

HRESULT WINAPI InputHandler::DIGetDeviceDataCustom(
	IDirectInputDeviceW * pDevice,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags
)
{
	HRESULT const result = originalDIGetDeviceData(pDevice, cbObjectData, rgdod, pdwInOut, dwFlags);
	
	if (rgdod == NULL) 
	{
		return result;
	}

	if (OverlayData::getSingleton()->overlayEnabled)
	{
		*pdwInOut = 0;
	}
	return result;
}
