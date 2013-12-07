#include <windows.h>
#include <stdio.h>
#include "OverlayInitializer.h"
#include "OverlayData.h"

OverlayData * processOverlayData;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		processOverlayData = new OverlayData();
		OverlayInitializer * initializer = new OverlayInitializer();
		initializer->execute(*processOverlayData);
		delete initializer;
		break;
	}
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

