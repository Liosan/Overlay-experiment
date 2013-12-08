#include "OverlayInjector.h"
#include <iostream>

OverlayInjector::OverlayInjector(std::string const & processName):
	processFinder(processName)
{
}

void OverlayInjector::run()
{
	if (this->processFinder.isRunning()) 
	{
		std::cerr << "Process " << this->processFinder.getProcessName() << " running\n";
		this->injectDll();
	} 
	else 
	{
		std::cerr << "Process " << this->processFinder.getProcessName() << " not running\n";
	}
}

std::string OverlayInjector::executablePath() const
{
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of( "\\/" );
    return std::string(buffer).substr(0, pos);
}

void OverlayInjector::injectDll() 
{
	HANDLE const process = this->processFinder.getHandle();
	std::string const pathToDll = executablePath() + "\\OverlayDLL.dll";
	
	DWORD dwAttrib = GetFileAttributes(pathToDll.c_str());
	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		std::cerr << "Dll file does not exist! " << pathToDll << "\n";
		return;
	}
	else
	{
		std::cerr << "Going to inject dll " << pathToDll << "\n";
	}
	
	LPVOID const addr = (LPVOID) GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"); // TODO test for error
	LPVOID const allocatedBuf = (LPVOID) VirtualAllocEx(
		process, 
		NULL, 
		pathToDll.size(), 
		MEM_RESERVE | MEM_COMMIT, 
		PAGE_READWRITE
	); // TODO test for error
	WriteProcessMemory(
		process, 
		allocatedBuf, 
		pathToDll.c_str(), 
		pathToDll.size(), 
		NULL
	); // TODO test for error

	HANDLE const threadID = CreateRemoteThread(
		process, 
		NULL, 
		0, 
		(LPTHREAD_START_ROUTINE)addr, 
		allocatedBuf, 
		NULL, 
		NULL
	);
	if(threadID == NULL) {
		std::cerr << "Error: the remote thread could not be created.\n";
	}
	else 
	{
		std::cerr <<  "Success: the remote thread was successfully created.\n";
	}
}
