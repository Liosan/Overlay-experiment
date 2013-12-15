#ifndef __INPUT_HANDLER_H__
#define __INPUT_HANDLER_H__

#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h> 

struct OverlayData;

typedef HRESULT (WINAPI * DI_GetDeviceData_t)(IDirectInputDeviceW *, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

/**
 * Implements a custom WindowProc which intercepts keyboard operations. 
 * Ctrl-space is detected and causes the the Overlay to be enabled.
 * All other messages are forwarded to the originalWndProc, unless the overlay is enabled - in that case mouse messages are blocked.
 * Also implements DirectInput GetDeviceData in order to block keyboard when the overlay is enabled.
 */
class InputHandler
{
public:
	InputHandler(OverlayData & overlayData, WNDPROC const originalWndProc);

	static LRESULT CALLBACK customWindowProcForwarder(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static HRESULT WINAPI DIGetDeviceDataForwarder(
		IDirectInputDeviceW * pDevice,
		DWORD cbObjectData,
		LPDIDEVICEOBJECTDATA rgdod,
		LPDWORD pdwInOut,
		DWORD dwFlags
	);
private:
	LRESULT customWindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void forwardMessage(UINT msg, LPARAM lParam);

	HRESULT WINAPI DIGetDeviceDataCustom(
		IDirectInputDeviceW * pDevice,
		DWORD cbObjectData,
		LPDIDEVICEOBJECTDATA rgdod,
		LPDWORD pdwInOut,
		DWORD dwFlags
	);

public:
	OverlayData & overlayData;
	WNDPROC const originalWndProc;
	DI_GetDeviceData_t originalDIGetDeviceData; // modified by Detours
};

#endif
