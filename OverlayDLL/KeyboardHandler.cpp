#include "KeyboardHandler.h"
#include "OverlayData.h"


KeyboardHandler::KeyboardHandler(WNDPROC const originalWndProc):
	originalWndProc(originalWndProc)
{
}
	
LRESULT CALLBACK KeyboardHandler::customWindowProcForwarder(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return processOverlayData->keyboardHandler->customWindowProc(wnd, msg, wParam, lParam);
}

LRESULT KeyboardHandler::customWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:
		if ((wParam == VK_SPACE) && (GetKeyState(VK_CONTROL) != 0))
		{
			processOverlayData->overlayEnabled = !processOverlayData->overlayEnabled;
		}
		break;
	}
	return CallWindowProc(originalWndProc, wnd, msg, wParam, lParam);
}