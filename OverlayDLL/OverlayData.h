#ifndef __OVERLAY_DATA_H__
#define __OVERLAY_DATA_H__

#include <Windows.h>
#include <vector>

class KeyboardHandler;
class Renderer;

struct OverlayData
{
public:
	HWND wnd;
	std::vector<DWORD> threadIds;

	bool overlayEnabled;

	KeyboardHandler * keyboardHandler;
	Renderer * renderer;
};

// global pointer
// TODO wrap as static variable for OverlayData?
extern OverlayData * processOverlayData;

#endif
