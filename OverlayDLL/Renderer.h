#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9core.h>

typedef HRESULT (WINAPI * DX_EndScene_t)(
	LPDIRECT3DDEVICE9 pDevice
);

class Renderer
{
public:
	Renderer(DX_EndScene_t const originalDXEndScene);

	static HRESULT WINAPI DXEndSceneForwarder(LPDIRECT3DDEVICE9 pDevice);
	DX_EndScene_t const originalDXEndScene; // must be public so can be modified by Detours call
private:	
	void initialize(LPDIRECT3DDEVICE9 const pDevice);
	void drawOverlayHint(LPDIRECT3DDEVICE9 const pDevice);

	HRESULT DXEndSceneCustom(LPDIRECT3DDEVICE9 pDevice);

	bool initialized;
	LPD3DXFONT font;
};

#endif
