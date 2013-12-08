#include "Renderer.h"
#include "OverlayData.h"
#include <iostream>

Renderer::Renderer(DX_EndScene_t const originalDXEndScene):
	initialized(false),
	font(NULL),
	originalDXEndScene(originalDXEndScene)
{
}

HRESULT WINAPI Renderer::DXEndSceneForwarder(LPDIRECT3DDEVICE9 const pDevice)
{
	return processOverlayData->renderer->DXEndSceneCustom(pDevice);
}

HRESULT Renderer::DXEndSceneCustom(LPDIRECT3DDEVICE9 const pDevice)
{
	this->initialize(pDevice);
	this->drawOverlayHint(pDevice);
	static bool once = true;
	if (once) { 
		std::cerr << "DXEndSceneCustom; calling original: " << this->originalDXEndScene << "\n"; 
		once = false; 
	}
	return this->originalDXEndScene(pDevice);
}

void Renderer::initialize(LPDIRECT3DDEVICE9 const pDevice)
{
	if (!this->initialized)
	{
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
	RECT rct;
	rct.left = 20;
	rct.right = 1680;
	rct.top = 20;
	rct.bottom = 220;
	int result = this->font->DrawTextA(NULL, "Hello world", -1, &rct, DT_NOCLIP, 0xFFFFFFF);
	static bool once = true;
	if (once) { 
		std::cerr << "drawOverlayHint complete: " << result << "\n"; 
		once = false; 
	}
}
