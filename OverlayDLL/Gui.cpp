#include "Gui.h"
#include "OverlayData.h"
#include "Renderer.h"

void Gui::drawOverlay()
{
	// read window size		
	RECT windowRect;
	GetWindowRect(OverlayData::getSingleton()->wnd, &windowRect);		
	this->windowWidth = windowRect.right - windowRect.left;
	this->windowHeight = windowRect.bottom - windowRect.top;

	if (OverlayData::getSingleton()->overlayEnabled)
	{
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
	int const centerX = this->windowWidth / 2;
	int const centerY = this->windowHeight / 2;

	// shadow the screen
	renderer->drawQuad(0, 0, this->windowWidth, this->windowHeight, 0xFFFFFFFF, renderer->greyTexture);

	// gog logo
	renderer->drawQuad(15, 15, 64, 64, 0xFFFFFFFF, renderer->gogTexture);

	// upper-center text
	renderer->drawText(centerX - 100, 20, 220, 50, "Overlay Test Application");

	// "button"
	renderer->drawText(centerX - 140, centerY - 80, 220, 50, "This button is a lie");
	renderer->drawText(centerX - 140, centerY - 60, 220, 50, "Because I didn't implement mouse capture");
	renderer->drawText(centerX - 140, centerY - 40, 220, 50, "The background could use some alpha as well");

	// lower-right text
	renderer->drawText(this->windowWidth - 230, this->windowHeight - 60, 230, 50, "Press ctrl-space to disable overlay");
}
