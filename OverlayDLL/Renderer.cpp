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

std::string Renderer::pathToSpriteDirectory() const
{	
	char buf[250];
	GetModuleFileName(GetModuleHandle("OverlayDLL.dll"), buf, 250);
	
	std::string pathToDll(buf);
	pathToDll = pathToDll.substr(0, pathToDll.find_last_of('\\')); // remove .dll file
	return pathToDll.substr(0, pathToDll.find_last_of('\\')) + "\\"; // remove Debug\ dir
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
		D3DVIEWPORT9 viewport;
		pDevice->GetViewport(&viewport);
		std::cerr << "Viewport: " << viewport.X << " / " << viewport.Y << " "
			<< viewport.Width << " / " << viewport.Height 
			<< " MinZ: " << viewport.MinZ << ", MaxZ: " << viewport.MaxZ << "\n";

		// create sprites and textures
		D3DXCreateSprite(pDevice, &this->sprite);
		std::string const pathToSpriteDir = this->pathToSpriteDirectory();
		HRESULT result;
		
		std::string greyTexPath = pathToSpriteDir + "grey.png";
		result = D3DXCreateTextureFromFile(pDevice, greyTexPath.c_str(), &this->greyTexture);
		if (result !=  D3D_OK) std::cerr << "Unable to load " << greyTexPath << "!\n";
		
		std::string gogTexPath = pathToSpriteDir + "gog.png";
		D3DXCreateTextureFromFile(pDevice, gogTexPath.c_str(), &this->gogTexture);
		if (result !=  D3D_OK) std::cerr << "Unable to load " << gogTexPath << "!\n";
		
		this->initialized = true;
	}
}

void Renderer::drawText(int x, int y, int w, int h, std::string const & text)
{	
	RECT rct = { x, y, x + w, y + h };
	this->font->DrawTextA(NULL, text.c_str(), -1, &rct, DT_NOCLIP, 0xFFFFFFFF);
}

void Renderer::drawQuad(LPDIRECT3DDEVICE9 const pDevice, int x, int y, int w, int h, DWORD color, IDirect3DTexture9 * texture)
{
	// VBs would be a faster way, but they seemed a bit overkill
	// TODO handle x, y, w, h
	D3DXVECTOR3 pos;
	pos.x = 15.f;
	pos.y = 15.f;
	pos.z = 0.1f;
	this->sprite->Begin(D3DXSPRITE_ALPHABLEND);
	this->sprite->Draw(texture, 0, 0, &pos, color);
	this->sprite->End();
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
	this->drawQuad(pDevice, 0, 0, this->windowWidth, this->windowHeight, 0xFFAAAAAA, this->gogTexture);

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
