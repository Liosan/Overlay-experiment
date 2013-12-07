#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <Windows.h>
#include <d3d9.h>

typedef HRESULT (WINAPI * DX_Present_t)(
	LPDIRECT3DDEVICE9 pDevice, 
	const RECT *pSourceRect,
	const RECT *pDestRect,
	HWND hDestWindowOverride,
	const RGNDATA *pDirtyRegion
);

class Renderer
{
public:
	Renderer(DX_Present_t const originalDXPresent);

	static HRESULT WINAPI DXPresentForwarder(
		LPDIRECT3DDEVICE9 pDevice, 
		const RECT *pSourceRect,
		const RECT *pDestRect,
		HWND hDestWindowOverride,
		const RGNDATA *pDirtyRegion
	);

	DX_Present_t const originalDXPresent;
private:
	HRESULT DXPresentCustom(
		LPDIRECT3DDEVICE9 pDevice, 
		const RECT *pSourceRect,
		const RECT *pDestRect,
		HWND hDestWindowOverride,
		const RGNDATA *pDirtyRegion
	);

};

#endif
