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
	HWND findWindow(OverlayData & overlayData) const;
	/**
	 * EnumThreadWindows callback, used by findWindow.
	 * Expects lParam to be a pointer to be a pointer to std::set<HWND> where the interesting windows are placed.
	 */
	static BOOL CALLBACK EnumThreadWndProc(HWND wnd, LPARAM lParam);

	/**
	 * Creates KeyboardHandler.
	 * Changes the used window proc so that all messages pass through KeyboardHandler.
	 */
	void overrideWindowProc(OverlayData & overlayData) const;

	/**
	 * Find adress of vtable for d3dDevice by creating a temporary object.
	 */
	UINT_PTR * OverlayInitializer::findD3dDeviceVTable(OverlayData & overlayData) const;

	/**
	 * Creates Renderer.
	 * Hooks DirectX methods so that the Renderer can be used.
	 */
	void hookRendering(OverlayData & overlayData) const;
};

#endif
