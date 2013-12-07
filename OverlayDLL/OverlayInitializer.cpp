#include "OverlayInitializer.h"
#include "OverlayData.h"
#include "KeyboardHandler.h"

#include <tlhelp32.h>
#include <sstream>
#include <set>

void OverlayInitializer::execute(OverlayData & overlayData) 
{	
	overlayData.threadIds = this->findThreads();
	overlayData.wnd = this->findWindow(overlayData);
	this->overrideWindowProc(overlayData);
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
