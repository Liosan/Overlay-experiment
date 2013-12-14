#ifndef __OVERLAY_DATA_H__
#define __OVERLAY_DATA_H__

#include <Windows.h>
#include <vector>

class InputHandler;
class Renderer;

/**
 * Stores the state of the overlay, as well as the pointers to it's components.
 * Initialized by OverlayInitializer, destroyed upon DLL_DETACH in dllmain.
 * These responsibilities are intentionaly taken out of OverlayData constructor/destructor
 * since it seemed more appropriate for an object bundling a few pointers together to be externally managed.
 */
struct OverlayData
{
public:
	HWND wnd;
	bool overlayEnabled;

	InputHandler * inputHandler;
	Renderer * renderer;

	static OverlayData * getSingleton();
	static void allocateSingleton();
private:
	// seems like the most convenient way to store crucial data since we are in an injected DLL.
	static OverlayData * singleton;
};


#endif
