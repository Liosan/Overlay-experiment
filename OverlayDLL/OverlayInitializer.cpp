#include "OverlayInitializer.h"

#include <tlhelp32.h>
#include <sstream>
#include <vector>
#include <set>

void OverlayInitializer::execute() 
{
	HWND const wnd = this->findWindow();
	this->overrideWindowProc(wnd);
}

// TODO cleanup
static WNDPROC originalWndProc;

// TODO cleanup - move to separate class
LRESULT CALLBACK CustomWindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
		MessageBox(NULL,"Talk talk from CustomWindowProc", "Foo", MB_OK);
		return DefWindowProc (wnd, msg, wparam, lparam);
	default:
		return CallWindowProc(originalWndProc, wnd, msg, wparam, lparam);
	}
}

// TODO cleanup
BOOL CALLBACK EnumThreadWndProc(HWND wnd, LPARAM lParam)
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
	return TRUE;
}

HWND OverlayInitializer::findWindow() const
{
	// enumerate threads of current process
	DWORD ownerPID = GetCurrentProcessId();
	HANDLE threadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
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
	} while(Thread32Next(threadSnap, &te32));
	CloseHandle(threadSnap);

	// search for windows owned by our threads, matching WindowTitle
	std::set<HWND> windows;
	for(std::vector<DWORD>::const_iterator it = threadIds.begin(); it != threadIds.end(); it++)
	{
		EnumThreadWindows(*it, EnumThreadWndProc, reinterpret_cast<LPARAM>(&windows));
	}
	// assume only one window is active
	return *windows.begin();
}

void OverlayInitializer::overrideWindowProc(HWND const wnd) const
{
	originalWndProc = reinterpret_cast<WNDPROC>(GetWindowLong(wnd, GWL_WNDPROC));
	LONG result = SetWindowLong(wnd, GWL_WNDPROC, reinterpret_cast<LONG>(CustomWindowProc));
	if (result == 0)
	{
		std::ostringstream oss;
		oss << "Result: " << result << ", err: " << GetLastError() << "; wnd=" << GetActiveWindow();
		MessageBox(NULL, oss.str().c_str(),"Foo", MB_OK);
	}
}
