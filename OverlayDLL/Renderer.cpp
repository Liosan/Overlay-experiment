#include "Renderer.h"
#include "OverlayData.h"

Renderer::Renderer(DX_Present_t const originalDXPresent):
	originalDXPresent(originalDXPresent)
{
}

HRESULT WINAPI Renderer::DXPresentForwarder(
	LPDIRECT3DDEVICE9 pDevice, 
	const RECT *pSourceRect,
	const RECT *pDestRect,
	HWND hDestWindowOverride,
	const RGNDATA *pDirtyRegion
) {
	MessageBox(NULL,"DXPresentForwarder", "Foo", MB_OK);
	return processOverlayData->renderer->DXPresentCustom(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT Renderer::DXPresentCustom(
	LPDIRECT3DDEVICE9 pDevice, 
	const RECT *pSourceRect,
	const RECT *pDestRect,
	HWND hDestWindowOverride,
	const RGNDATA *pDirtyRegion
) {
	MessageBox(NULL,"DXPresentCustom", "Foo", MB_OK);
	return this->originalDXPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}
