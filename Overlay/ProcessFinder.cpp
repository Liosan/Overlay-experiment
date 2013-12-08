#include "ProcessFinder.h"

#include <tlhelp32.h>
#include <iostream>

ProcessFinder::ProcessFinder(std::string const & processName):
	processName(processName),
	hProcess(NULL)
{
	this->scanForProcess();
}

ProcessFinder::~ProcessFinder()
{
	if (this->hProcess != NULL)
	{
		CloseHandle(this->hProcess);
	}
}

std::string const & ProcessFinder::getProcessName() const
{
	return this->processName;
}

bool ProcessFinder::isRunning() const
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

HANDLE ProcessFinder::getHandle() const
{
	return this->hProcess;
}

HANDLE ProcessFinder::scanForProcess() 
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
