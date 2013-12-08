#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <string>

typedef HRESULT (WINAPI * DX_EndScene_t)(
	LPDIRECT3DDEVICE9 pDevice
);

class Renderer
{
public:
	Renderer(DX_EndScene_t const originalDXEndScene);
	void terminate();

	static HRESULT WINAPI DXEndSceneForwarder(LPDIRECT3DDEVICE9 pDevice);
	DX_EndScene_t const originalDXEndScene; // must be public so can be modified by Detours call
private:	
	void initialize(LPDIRECT3DDEVICE9 const pDevice);

	void drawText(int x, int y, int w, int h, std::string const & text);

	void drawOverlayHint(LPDIRECT3DDEVICE9 const pDevice);
	void drawFullOverlay(LPDIRECT3DDEVICE9 const pDevice);

	HRESULT DXEndSceneCustom(LPDIRECT3DDEVICE9 pDevice);

	bool running;
	bool initialized;
	LPD3DXFONT font;

	int windowWidth;
	int windowHeight;
};

#endif
