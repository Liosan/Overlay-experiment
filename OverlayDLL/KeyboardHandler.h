#ifndef __KEYBOARD_HANDLER_H__
#define __KEYBOARD_HANDLER_H__

#include <Windows.h>

/**
 * Implements a custom WindowProc which intercepts keyboard operations. 
 * Ctrl-space is detected and causes the the Overlay to be enabled.
 * All other messages are forwarded to the originalWndProc.
 */
class KeyboardHandler // TODO handle GetDeviceData, and keyboard. TODO input handler, grab mouse position
{
public:
	KeyboardHandler(WNDPROC const originalWndProc);

	static LRESULT CALLBACK customWindowProcForwarder(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	void hookMessageHandling();

	LRESULT customWindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

	WNDPROC const originalWndProc;
};

#endif
