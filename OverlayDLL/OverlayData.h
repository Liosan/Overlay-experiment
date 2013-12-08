#ifndef __OVERLAY_DATA_H__
#define __OVERLAY_DATA_H__

#include <Windows.h>
#include <vector>

class KeyboardHandler;
class Renderer;

/**
 * Stores the state of the overlay, as well as the pointers to it's components.
 * Initialized by OverlayInitializer.
 */
struct OverlayData
{
public:
	HWND wnd;
	bool overlayEnabled;

	KeyboardHandler * keyboardHandler;
	Renderer * renderer;

	static OverlayData * getSingleton();
	static void allocateSingleton();
private:
	// seems like the most convenient way to store crucial data since we are in an injected DLL.
	static OverlayData * singleton;
};


#endif
