#include "Gui.h"
#include "OverlayData.h"
#include "Renderer.h"
#include <iostream>

Gui::Gui():
	buttonState(INACTIVE)
{
}

void Gui::mouseButtonUp()
{
	if (this->buttonState == CLICK)
	{
		this->buttonState = INACTIVE;
		OverlayData::getSingleton()->overlayEnabled = false;
	}
}

void Gui::mouseButtonDown()
{
	if (this->buttonState == HOVER)
	{
		this->buttonState = CLICK;
	}
}

void Gui::mouseMove(int x, int y)
{
	bool const hover = (x >= BUTTON_X) && (x <= BUTTON_X + BUTTON_WIDTH) && (y >= BUTTON_Y) && (y <= BUTTON_Y + BUTTON_HEIGHT);
	if (hover && (this->buttonState == INACTIVE))
	{
		this->buttonState = HOVER;
	} 
	else if (!hover && (this->buttonState == HOVER))
	{
		this->buttonState = INACTIVE;
	}
}

void Gui::drawOverlay()
{
	// read window size		
	RECT windowRect;
	GetWindowRect(OverlayData::getSingleton()->wnd, &windowRect);		
	this->windowWidth = windowRect.right - windowRect.left;
	this->windowHeight = windowRect.bottom - windowRect.top;

	if (OverlayData::getSingleton()->overlayEnabled)
	{
		std::cerr << "windowWidth: " << this->windowWidth << ", windowHeight: " << this->windowHeight << "\n";
		this->drawFullOverlay();
	}
	else
	{
		this->drawOverlayHint();
	}
}

void Gui::drawOverlayHint()
{	
	OverlayData::getSingleton()->renderer->drawText(this->windowWidth - 230, this->windowHeight - 60, 230, 50, "Press ctrl-space to enable overlay");
}

void Gui::drawFullOverlay()
{
	Renderer * renderer = OverlayData::getSingleton()->renderer;

	// shadow the screen
	renderer->drawQuad(0, 0, 0.02f, (float)this->windowWidth, (float)this->windowHeight, 0x77FFFFFF, renderer->greyTexture);

	// gog logo
	renderer->drawQuad(15, 15, 0.01f, 64, 64, 0xFFFFFFFF, renderer->gogTexture);

	// upper-center text
	renderer->drawText(this->windowWidth / 2 - 100, 20, 220, 50, "Overlay Test Application");

	this->drawButton();

	// lower-right text
	renderer->drawText(this->windowWidth - 230, this->windowHeight - 60, 230, 50, "Press ctrl-space to disable overlay");
}

void Gui::drawButton()
{	
	DWORD color;
	switch(this->buttonState)
	{
	case INACTIVE:
		color = 0xFF000000;
		break;
	case HOVER:
		color = 0xFFAA7788;
		break;
	case CLICK:
		color = 0xFF77BB88;
		break;
	}

	Renderer * renderer = OverlayData::getSingleton()->renderer;
	renderer->drawQuad((float)BUTTON_X, (float)BUTTON_Y, 0.02f, (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT, color, renderer->greyTexture);
	renderer->drawQuad(BUTTON_X + 1.f, BUTTON_Y + 1, 0.02f, BUTTON_WIDTH - 2.f, BUTTON_HEIGHT - 2.f, 0xFFFFFFFF, renderer->greyTexture);
	renderer->drawText(BUTTON_X + 5, BUTTON_Y +20, BUTTON_WIDTH, BUTTON_HEIGHT, "Press to disable overlay");
}
