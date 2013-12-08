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
	return processOverlayData->renderer->DXEndSceneCustom(pDevice);
}

HRESULT Renderer::DXEndSceneCustom(LPDIRECT3DDEVICE9 const pDevice)
{
	static bool once = true;
	if (this->running)
	{
		this->initialize(pDevice);
		this->drawOverlayHint(pDevice);
		if (once) { 
			std::cerr << "DXEndSceneCustom; calling original: " << this->originalDXEndScene << "\n"; 
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
		this->initialized = true;
	}
}

void Renderer::drawOverlayHint(LPDIRECT3DDEVICE9 const pDevice)
{
	std::cerr << "drawOverlayHint with device: " << pDevice << "\n"; 
	D3DCOLOR rectColor = D3DCOLOR_XRGB(200,255,255);
	D3DRECT BarRect = { 20, 50, 120, 120 }; 
	pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET, rectColor, 0, 0);
	
	RECT rct = { 20, 20, 220, 60 };
	this->font->DrawTextA(NULL, "Hello world", -1, &rct, DT_NOCLIP, 0xFFFFFFFF);
}
