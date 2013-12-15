#ifndef __GUI_H__
#define __GUI_H__

#include <d3d9.h>

class Gui
{
public:
	void drawOverlay();
private:	
	void drawOverlayHint();
	void drawFullOverlay();

	int windowWidth, windowHeight;
};

#endif
