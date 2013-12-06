#include "ProcessHook.h"
#include <tlhelp32.h>
#include <iostream>

ProcessHook::ProcessHook(std::string const & processName):
	processName(processName),
	hProcess(NULL)
{
	this->scanForProcess();
}

ProcessHook::~ProcessHook()
{
	if (this->hProcess != NULL)
	{
		CloseHandle(this->hProcess);
	}
}

bool ProcessHook::isRunning() 
{
	if (this->hProcess == NULL) 
	{
		return false;
	} 
	else 
	{
		DWORD exitCode;
		BOOL retVal = GetExitCodeProcess(this->hProcess, &exitCode);
		std::cerr << "retVal " << retVal << ", hProcess " << this->hProcess << ", exitCode: " << exitCode << "\n";
		return retVal && (exitCode == STILL_ACTIVE);
	}
}

HANDLE ProcessHook::getHandle()
{
	return this->hProcess;
}

HANDLE ProcessHook::scanForProcess() 
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	this->hProcess = NULL;

	if (Process32First(snapshot, &entry))
	{
		do
		{
			if (std::string(entry.szExeFile) == this->processName)
			{  
				std::cerr << "Process " << std::string(entry.szExeFile) << " found!\n";
				this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
			}
		}
		while (Process32Next(snapshot, &entry) && (hProcess == NULL));
	}

	CloseHandle(snapshot);

	return hProcess;
}
