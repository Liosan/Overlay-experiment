#include <windows.h>
#include <stdio.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	FILE * file = fopen("C:\\temp.txt", "a+");
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		fprintf(file, "DLL_PROCESS_ATTACH.\n");
		break;
	case DLL_PROCESS_DETACH:
		fprintf(file, "DLL_PROCESS_DETACH.\n");
		break;
	case DLL_THREAD_ATTACH:
		fprintf(file, "DLL_THREAD_ATTACH.\n");
		break;
	case DLL_THREAD_DETACH:
		fprintf(file, "DLL_THREAD_DETACH.\n");
		break;
	}
	return TRUE;
}

