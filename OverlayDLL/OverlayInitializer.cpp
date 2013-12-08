#include "OverlayInitializer.h"
#include "OverlayData.h"
#include "KeyboardHandler.h"
#include "Renderer.h"

#include <tlhelp32.h>
#include <iostream>
#include <set>
#include "detours\detours.h"

void OverlayInitializer::execute(OverlayData & overlayData) 
{	
	overlayData.overlayEnabled = false;
	std::vector<DWORD> const threadIds = this->findThreads();
	overlayData.wnd = this->findWindow(overlayData, threadIds);
	this->overrideWindowProc(overlayData);
	this->hookRendering(overlayData);
}

std::vector<DWORD> OverlayInitializer::findThreads() const
{
	// enumerate threads of current process
	DWORD const ownerPID = GetCurrentProcessId();
	HANDLE const threadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
	THREADENTRY32 te32;  
	te32.dwSize = sizeof(THREADENTRY32); 
	Thread32First(threadSnap, &te32);
	std::vector<DWORD> threadIds;
	do 
	{ 
		if(te32.th32OwnerProcessID == ownerPID)
		{
			threadIds.push_back(te32.th32ThreadID);
		}
	}
	while(Thread32Next(threadSnap, &te32));
	CloseHandle(threadSnap);
	return threadIds;
}

BOOL CALLBACK OverlayInitializer::EnumThreadWndProc(HWND wnd, LPARAM lParam)
{
	std::set<HWND> * windows = reinterpret_cast<std::set<HWND> *>(lParam);
	if (wnd != NULL)
	{
		char buf[250];
		GetWindowText(wnd, buf, 250);
		if (std::string(buf).find(WindowTitle) != std::string::npos)
		{
			windows->insert(wnd);
		}
	}
	return TRUE; // continue enumerating
}

HWND OverlayInitializer::findWindow(OverlayData & overlayData, std::vector<DWORD> const & threadIds) const
{
	// search for windows owned by our threads, matching WindowTitle
	std::set<HWND> windows;
	for(std::vector<DWORD>::const_iterator it = threadIds.begin(); it != threadIds.end(); it++)
	{
		EnumThreadWindows(*it, EnumThreadWndProc, reinterpret_cast<LPARAM>(&windows));
	}
	// assume only one window is active
	return *windows.begin();
}

void OverlayInitializer::overrideWindowProc(OverlayData & overlayData) const
{	
	WNDPROC const originalWndProc = reinterpret_cast<WNDPROC>(GetWindowLong(overlayData.wnd, GWL_WNDPROC));
	overlayData.keyboardHandler = new KeyboardHandler(originalWndProc);
	LONG const result = SetWindowLong(overlayData.wnd, GWL_WNDPROC, reinterpret_cast<LONG>(KeyboardHandler::customWindowProcForwarder));

	if (result == 0)
	{
		std::cerr << "overrideWindowProc result: " << result << ", err: " << GetLastError() << "; wnd=" << overlayData.wnd << "\n";
	}
}

UINT_PTR * OverlayInitializer::findD3dDeviceVTable(OverlayData & overlayData) const
{
	typedef IDirect3D9 * (WINAPI * DXCreate_t)(UINT);
	DXCreate_t pCreate = (DXCreate_t)(GetProcAddress(GetModuleHandle("d3d9.dll"), "Direct3DCreate9"));
    LPDIRECT3D9 directX = pCreate(D3D_SDK_VERSION);
	
	D3DPRESENT_PARAMETERS dxPresentParams;
    ZeroMemory( &dxPresentParams, sizeof(dxPresentParams) );
    dxPresentParams.Windowed = TRUE;
    dxPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    dxPresentParams.BackBufferFormat = D3DFMT_UNKNOWN;
	
	LPDIRECT3DDEVICE9 directXDevice;
    directX->CreateDevice(
        D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL, 
        overlayData.wnd, 
		D3DCREATE_HARDWARE_VERTEXPROCESSING, 
        &dxPresentParams, 
		&directXDevice
	);

	UINT_PTR * vTablePtr = (UINT_PTR *)(*((UINT_PTR *)directXDevice));

	std::cerr << "vTablePtr: " << vTablePtr << ", directXDevice: " << directXDevice << "\n";

	directXDevice->Release();
	directX->Release();

	return vTablePtr;
}

void OverlayInitializer::hookRendering(OverlayData & overlayData) const
{
	UINT_PTR * vtablePtr = this->findD3dDeviceVTable(overlayData);	

	DX_EndScene_t const originalDXEndScene = (DX_EndScene_t) vtablePtr[42]; // offset in d3d9.h for 9.0c
	overlayData.renderer = new Renderer(originalDXEndScene);

	std::cerr << "originalDXEndScene: " << originalDXEndScene << "; Renderer::DXEndSceneForwarder: " << Renderer::DXEndSceneForwarder << "\n";

	//DWORD prot;
	//VirtualProtect(&vtablePtr[42], sizeof(DWORD), PAGE_READWRITE, &prot);
	//*(PDWORD)&vtablePtr[42] = (DWORD)Renderer::DXEndSceneForwarder;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if(DetourAttach(&(PVOID&)(overlayData.renderer->originalDXEndScene), Renderer::DXEndSceneForwarder) == ERROR_INVALID_HANDLE)
	{
		MessageBox(NULL, "Attach of EndScene failed","Detours failure", MB_OK);
	}
	DetourTransactionCommit();
	
	std::cerr << "Attach complete!\n";
}
