#ifndef __GUI_H__
#define __GUI_H__

#include <d3d9.h>

class Gui
{
public:
	Gui();

	void drawOverlay();

	void mouseButtonUp();
	void mouseButtonDown();
	void mouseMove(int x, int y);
private:
	static int const BUTTON_X = 400;
	static int const BUTTON_Y = 320;
	static int const BUTTON_WIDTH = 220;
	static int const BUTTON_HEIGHT = 50;
	enum ButtonState
	{
		INACTIVE,
		HOVER,
		CLICK
	};

	void drawOverlayHint();
	void drawFullOverlay();

	void drawButton();

	int windowWidth, windowHeight;
	ButtonState buttonState;
};

#endif
