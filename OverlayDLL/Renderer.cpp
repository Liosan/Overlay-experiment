#include "Renderer.h"
#include "Gui.h"
#include "OverlayData.h"
#include <iostream>

Renderer::Renderer(DX_EndScene_t const originalDXEndScene):
	running(true),
	initialized(false),
	font(NULL),
	originalDXEndScene(originalDXEndScene),
	currentDevice(NULL)
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
		this->currentDevice = pDevice;
		this->initialize();

		OverlayData::getSingleton()->gui->drawOverlay();
		
		if (once) { 
			std::cerr << "DXEndSceneCustom; calling original: " << this->originalDXEndScene << "\n"; 
			once = false; 
		}
		this->currentDevice = NULL;
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

void Renderer::initialize()
{
	if (!this->initialized)
	{
		std::cerr << "initialize with device: " << this->currentDevice << "\n"; 
		// create font
		D3DXCreateFont(
			this->currentDevice, 
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

		// create sprites and textures
		D3DXCreateSprite(this->currentDevice, &this->sprite);
		std::string const pathToSpriteDir = this->pathToSpriteDirectory();
		HRESULT result;
		
		std::string greyTexPath = pathToSpriteDir + "grey.png";
		result = D3DXCreateTextureFromFile(this->currentDevice, greyTexPath.c_str(), &this->greyTexture);
		if (result !=  D3D_OK) std::cerr << "Unable to load " << greyTexPath << "!\n";
		
		std::string gogTexPath = pathToSpriteDir + "gog.png";
		result = D3DXCreateTextureFromFile(this->currentDevice, gogTexPath.c_str(), &this->gogTexture);
		if (result !=  D3D_OK) std::cerr << "Unable to load " << gogTexPath << "!\n";
		
		this->initialized = true;
	}
}

void Renderer::drawText(int x, int y, int w, int h, std::string const & text)
{	
	RECT rct = { x, y, x + w, y + h };
	this->font->DrawTextA(NULL, text.c_str(), -1, &rct, DT_NOCLIP, 0xFFFFFFFF);
}

void Renderer::drawQuad(float x, float y, float z, float w, float h, DWORD color, IDirect3DTexture9 * texture)
{
	// using VBs would be faster, but they seemed a bit overkill
	D3DXVECTOR3 pos;
	pos.x = x;
	pos.y = y;
	pos.z = z;

	D3DSURFACE_DESC textureDesc;
	texture->GetLevelDesc(0, &textureDesc);
	D3DXVECTOR2 scalingFactor(w / (float)textureDesc.Width, h / (float)textureDesc.Height);
	D3DXMATRIX spriteMatrix;
	D3DXMatrixTransformation2D(&spriteMatrix, NULL, 0, &scalingFactor, NULL, 0, NULL);	

	this->sprite->Begin(D3DXSPRITE_ALPHABLEND);
	this->sprite->SetTransform (&spriteMatrix);
	this->sprite->Draw(texture, 0, 0, &pos, color);
	this->sprite->End();
}

