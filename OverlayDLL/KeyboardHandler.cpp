#include "KeyboardHandler.h"
#include "OverlayData.h"


KeyboardHandler::KeyboardHandler(WNDPROC const originalWndProc):
	originalWndProc(originalWndProc)
{
}
	
LRESULT CALLBACK KeyboardHandler::customWindowProcForwarder(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return OverlayData::getSingleton()->keyboardHandler->customWindowProc(wnd, msg, wParam, lParam);
}

LRESULT KeyboardHandler::customWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{ // TODO this should be based on DI_GetDeviceData_t
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