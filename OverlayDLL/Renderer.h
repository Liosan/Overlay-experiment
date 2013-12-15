#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <string>

typedef HRESULT (WINAPI * DX_EndScene_t)(
	LPDIRECT3DDEVICE9 pDevice
);

/**
 * Display overlay components depending on current overlay state (enabled/disabled).
 * Does not currently support window minimizing then maximizing, resolution changes, fullscreen toggle etc.
 */
class Renderer
{
public:
	Renderer(DX_EndScene_t const originalDXEndScene);
	// Cease rendering. Helps with graceful termination.
	void terminate();
		
	void drawText(int x, int y, int w, int h, std::string const & text);
	void drawQuad(int x, int y, int w, int h, DWORD color, IDirect3DTexture9 * texture);

	static HRESULT WINAPI DXEndSceneForwarder(LPDIRECT3DDEVICE9 pDevice);
	DX_EndScene_t const originalDXEndScene; // must be public so can be modified by Detours call
	
	// publicly acessible since it's easier than proper texture management
	LPDIRECT3DTEXTURE9 gogTexture;
	LPDIRECT3DTEXTURE9 greyTexture;
private:	
	std::string pathToSpriteDirectory() const;
	void initialize();

	HRESULT DXEndSceneCustom(LPDIRECT3DDEVICE9 pDevice);

	bool running;
	bool initialized;
	LPD3DXFONT font;
	LPD3DXSPRITE sprite;
	LPDIRECT3DDEVICE9 currentDevice;
};

#endif
