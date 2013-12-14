#include "OverlayInitializer.h"
#include "OverlayData.h"
#include "InputHandler.h"
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
	this->hookKeyboard(overlayData);
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
		if (te32.th32OwnerProcessID == ownerPID)
		{
			threadIds.push_back(te32.th32ThreadID);
		}
	}
	while (Thread32Next(threadSnap, &te32));
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
	overlayData.inputHandler = new InputHandler(originalWndProc);
	
	LONG const result = SetWindowLong(overlayData.wnd, GWL_WNDPROC, reinterpret_cast<LONG>(InputHandler::customWindowProcForwarder));
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

	std::cerr << "EndScene vTablePtr: " << vTablePtr << ", directXDevice: " << directXDevice << "\n";

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

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if(DetourAttach(&(PVOID&)(overlayData.renderer->originalDXEndScene), Renderer::DXEndSceneForwarder) == ERROR_INVALID_HANDLE)
	{
		MessageBox(NULL, "Attach of EndScene failed", "Detours failure", MB_OK);
		return;
	}
	DetourTransactionCommit();
	
	std::cerr << "EndScene attach complete!\n";
}

UINT_PTR * OverlayInitializer::findDirectInputDeviceVTable(OverlayData & overlayData) const
{
	typedef HRESULT (WINAPI * DI8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
	DI8Create_t pCreate = (DI8Create_t)(GetProcAddress(GetModuleHandle("dinput8.dll"), "DirectInput8Create"));
	if (pCreate == NULL)
	{
		std::cerr << "Locating DirectInput8Create failed!\n";
		return NULL;
	}

	LPDIRECTINPUT8 pDIObject = NULL;

	if (pCreate(
			GetModuleHandle(NULL),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			(void**)&pDIObject,
			NULL
		) != DI_OK
	) 
	{
		std::cerr << "DirectInput creation failed!\n";
		return NULL;
	}

	LPDIRECTINPUTDEVICE8 pDIKeyboardDevice = NULL; // TODO keyboard should be used
	if (pDIObject->CreateDevice(GUID_SysKeyboard, &pDIKeyboardDevice, NULL) != DI_OK)
	{
		std::cerr << "DirectInput keyboard device creation failed!\n";
		return NULL;
	}

	UINT_PTR * vTablePtr = (UINT_PTR *)(*((UINT_PTR *)pDIKeyboardDevice));
	std::cerr << "DirectInput device vTablePtr: " << vTablePtr << ", pDIKeyboardDevice: " << pDIKeyboardDevice << "\n";

	pDIKeyboardDevice->Release();
	pDIObject->Release();
	return vTablePtr;
}

void OverlayInitializer::hookKeyboard(OverlayData & overlayData) const
{
	std::cerr << "Attempting DirectInput keyboard hooks...\n";
	UINT_PTR * vtablePtr = this->findDirectInputDeviceVTable(overlayData);	

	OverlayData::getSingleton()->inputHandler->originalDIGetDeviceData = (DI_GetDeviceData_t) vtablePtr[10]; // offset in dinput.h

	std::cerr << "originalDIGetDeviceData: " << OverlayData::getSingleton()->inputHandler->originalDIGetDeviceData 
		<< "; InputHandler::DIGetDeviceDataForwarder: " << InputHandler::DIGetDeviceDataForwarder << "\n";

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if(DetourAttach(&(PVOID&)(OverlayData::getSingleton()->inputHandler->originalDIGetDeviceData), InputHandler::DIGetDeviceDataForwarder) == ERROR_INVALID_HANDLE)
	{
		MessageBox(NULL, "Attach of DirectInput GetDeviceData failed", "Detours failure", MB_OK);
		return;
	}
	DetourTransactionCommit();

	std::cerr << "DirectInput attach complete!\n";	
}