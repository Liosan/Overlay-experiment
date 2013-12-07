#ifndef __OVERLAY_DATA_H__
#define __OVERLAY_DATA_H__

#include <Windows.h>
#include <vector>

class KeyboardHandler;

struct OverlayData
{
public:
	HWND wnd;

	KeyboardHandler * keyboardHandler;
	std::vector<DWORD> threadIds;
};

extern OverlayData * processOverlayData;

#endif
