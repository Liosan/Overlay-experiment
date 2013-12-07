#include "OverlayInitializer.h"
#include "OverlayData.h"
#include "KeyboardHandler.h"
#include "Renderer.h"

#include <tlhelp32.h>
#include <sstream>
#include <set>
#include "detours\detours.h"

void OverlayInitializer::execute(OverlayData & overlayData) 
{	
	overlayData.threadIds = this->findThreads();
	overlayData.wnd = this->findWindow(overlayData);
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

HWND OverlayInitializer::findWindow(OverlayData & overlayData) const
{
	// search for windows owned by our threads, matching WindowTitle
	std::set<HWND> windows;
	for(std::vector<DWORD>::const_iterator it = overlayData.threadIds.begin(); it != overlayData.threadIds.end(); it++)
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
		std::ostringstream oss;
		oss << "Result: " << result << ", err: " << GetLastError() << "; wnd=" << overlayData.wnd;
		MessageBox(NULL, oss.str().c_str(),"Foo", MB_OK);
	}
}

// TODO integrate, comment
DWORD * FindVTable(OverlayData & overlayData)
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

	DWORD * vTablePtr = (DWORD*)*((DWORD*)directXDevice);
	
	std::ostringstream oss;
	oss << "vTablePtr: " << vTablePtr << ", directXDevice: " << directXDevice;
	MessageBox(NULL, oss.str().c_str(),"Foo", MB_OK);

	directXDevice->Release();
	directX->Release();

	return vTablePtr;
}

void OverlayInitializer::hookRendering(OverlayData & overlayData) const
{
	DWORD * vtablePtr = FindVTable(overlayData);
	
	DX_Present_t const originalDXPresent = (DX_Present_t) vtablePtr[17];
    // pReset = (DX_Reset_t) vtablePtr[16];
	
	std::ostringstream oss1;
	oss1 << "hookRendering grabbed vtablePtr: " << vtablePtr << "; originalDXPresent: " << originalDXPresent;
	MessageBox(NULL, oss1.str().c_str(), "Foo", MB_OK);

	overlayData.renderer = new Renderer(originalDXPresent);

	DetourTransactionBegin();
	if(DetourAttach(&(PVOID&)originalDXPresent, Renderer::DXPresentForwarder) == ERROR_INVALID_HANDLE)
	{
		MessageBox(NULL, "Attach failed","Detours", MB_OK);
	}
	DetourTransactionCommit();
	
	std::ostringstream oss;
	oss << "Result: " <<  ", err: " << GetLastError() << "; originalDXPresent=" << originalDXPresent;
	MessageBox(NULL, oss.str().c_str(),"Foo", MB_OK);
}
