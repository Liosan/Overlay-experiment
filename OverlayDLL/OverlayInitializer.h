#ifndef __OVERLAY_INITIALIZER_H__
#define __OVERLAY_INITIALIZER_H__

#include <Windows.h>
#include <vector>

char const * const WindowTitle = "The Witcher";

struct OverlayData;

/**
 * Sets up everything needed for the Overlay to work. Fills OverlayData structure.
 */
class OverlayInitializer 
{
public:
	// does not claim ownership
	void execute(OverlayData & overlayData);
private:
	/**
	 * Returns all threads of the current process.
	 */
	std::vector<DWORD> findThreads() const;

	/**
	 * Returns first found window owned by any thread in the current process, with matching WindowTitle.
	 */
	HWND findWindow() const;
	/**
	 * EnumThreadWindows callback, used by findWindow.
	 * Expects lParam to be a pointer to be a pointer to std::set<HWND> where the interesting windows are placed.
	 */
	static BOOL CALLBACK EnumThreadWndProc(HWND wnd, LPARAM lParam);

	void overrideWindowProc(OverlayData & overlayData) const;
};

#endif
