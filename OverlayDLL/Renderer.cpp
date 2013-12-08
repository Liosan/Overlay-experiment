#include "Renderer.h"
#include "OverlayData.h"
#include <iostream>

Renderer::Renderer(DX_EndScene_t const originalDXEndScene):
	running(true),
	initialized(false),
	font(NULL),
	originalDXEndScene(originalDXEndScene)
{
}

void Renderer::terminate()
{
	this->running = false;
}

HRESULT WINAPI Renderer::DXEndSceneForwarder(LPDIRECT3DDEVICE9 const pDevice)
{
	return OverlayData::getSingleton()->renderer->DXEndSceneCustom(pDevice);
}

HRESULT Renderer::DXEndSceneCustom(LPDIRECT3DDEVICE9 const pDevice)
{
	static bool once = true;
	if (this->running)
	{
		this->initialize(pDevice);

		if (OverlayData::getSingleton()->overlayEnabled)
		{
			this->drawFullOverlay(pDevice);
		}
		else
		{
			this->drawOverlayHint(pDevice);
		}
		
		if (once) { 
			std::cerr << "DXEndSceneCustom; calling original: " << this->originalDXEndScene << "\n"; 
			std::cerr << "window size: (" << this->windowWidth << ", " << this->windowHeight << ")\n";
			once = false; 
		}
	}
	return this->originalDXEndScene(pDevice);
}

void Renderer::initialize(LPDIRECT3DDEVICE9 const pDevice)
{
	if (!this->initialized)
	{
		std::cerr << "initialize with device: " << pDevice << "\n"; 
		// create font
		D3DXCreateFont(
			pDevice, 
			16, 
			0, 
			FW_BOLD, 
			0, 
			FALSE, 
			DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, 
			ANTIALIASED_QUALITY, 
			DEFAULT_PITCH | FF_DONTCARE, 
			"Arial",
			&this->font 
		);

		// read window size		
		RECT windowRect;
		GetWindowRect(OverlayData::getSingleton()->wnd, &windowRect);		
		this->windowWidth = windowRect.right - windowRect.left;
		this->windowHeight = windowRect.bottom - windowRect.top;

		this->initialized = true;
	}
}

void Renderer::drawText(int x, int y, int w, int h, std::string const & text)
{	
	RECT rct = { x, y, x + w, y + h };
	this->font->DrawTextA(NULL, text.c_str(), -1, &rct, DT_NOCLIP, 0xFFFFFFFF);
}

void Renderer::drawOverlayHint(LPDIRECT3DDEVICE9 const pDevice)
{	
	this->drawText(this->windowWidth - 230, this->windowHeight - 60, 230, 50, "Press ctrl-space to enable overlay");
}

void Renderer::drawFullOverlay(LPDIRECT3DDEVICE9 const pDevice)
{
	int const centerX = this->windowWidth / 2;
	int const centerY = this->windowHeight / 2;

	// shadow the screen
	D3DRECT FullScreenRect = { 0, 0, this->windowWidth, this->windowHeight }; 
	pDevice->Clear(1, &FullScreenRect, D3DCLEAR_TARGET, 0xAA000000, 0, 0);

	// upper-center text
	this->drawText(centerX - 100, 20, 220, 50, "Overlay Test Application");

	// "button"
	D3DRECT BarRect = { centerX - 150, centerY - 80, centerX + 200, centerY - 20 }; 
	pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET, 0xFFAAAAAA, 0, 0);
	this->drawText(centerX - 140, centerY - 80, 220, 50, "This button is a lie");
	this->drawText(centerX - 140, centerY - 60, 220, 50, "Because I didn't implement mouse capture");
	this->drawText(centerX - 140, centerY - 40, 220, 50, "The background could use some alpha as well");

	// lower-right text
	this->drawText(this->windowWidth - 230, this->windowHeight - 60, 230, 50, "Press ctrl-space to disable overlay");
}
