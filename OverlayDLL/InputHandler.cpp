#include "InputHandler.h"
#include "OverlayData.h"
#include "Gui.h"
#include <iostream>
#include <Windowsx.h>


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
	case WM_XBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_MOUSEMOVE:
		if (OverlayData::getSingleton()->overlayEnabled)
		{
			this->forwardMessage(msg, lParam);
			msg = WM_NULL;
		}
		break;
	}
	return CallWindowProc(originalWndProc, wnd, msg, wParam, lParam);
}

void InputHandler::forwardMessage(UINT msg, LPARAM lParam)
{
	Gui * gui = OverlayData::getSingleton()->gui;
	switch(msg)
	{
	case WM_LBUTTONUP:
		gui->mouseButtonUp();
		break;
	case WM_LBUTTONDOWN:
		gui->mouseButtonDown();
		break;
	case WM_MOUSEMOVE:
		gui->mouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	}
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
