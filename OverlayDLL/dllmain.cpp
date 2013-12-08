#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "OverlayInitializer.h"
#include "OverlayData.h"
#include "Renderer.h"

OverlayData * processOverlayData;

// allocate console to ease debugging
void createConsole()
{
	FILE *out, *err;

	AllocConsole();

	out = _fdopen(_open_osfhandle(PtrToUlong(GetStdHandle(STD_OUTPUT_HANDLE)), _O_TEXT), "w");
	err = _fdopen(_open_osfhandle(PtrToUlong(GetStdHandle(STD_ERROR_HANDLE)), _O_TEXT), "w");

	*stdout = *out;
	*stderr = *err;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

DWORD WINAPI initializationThread(LPVOID)
{	
	createConsole();
	processOverlayData = new OverlayData();
	OverlayInitializer * initializer = new OverlayInitializer();
	initializer->execute(*processOverlayData);
	delete initializer;
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		CreateThread(NULL, NULL, initializationThread, NULL, NULL, NULL);
		break;
	}
	case DLL_PROCESS_DETACH:
		processOverlayData->renderer->terminate();
		FreeConsole();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

