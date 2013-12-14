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
	}
	return CallWindowProc(originalWndProc, wnd, msg, wParam, lParam);
}