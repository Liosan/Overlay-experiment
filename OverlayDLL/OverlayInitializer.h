#ifndef __OVERLAY_INITIALIZER_H__
#define __OVERLAY_INITIALIZER_H__

#include <Windows.h>

char const * const WindowTitle = "The Witcher";

/**
 * Sets up everything needed for the Overlay to work.
 */
class OverlayInitializer 
{
public:
	void execute();
private:
	/**
	 * Returns first found window owned by any thread in the current process.
	 */
	HWND findWindow() const;

	void overrideWindowProc(HWND const wnd) const;
};

#endif
