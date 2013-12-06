#ifndef __PROCESS_HOOK_H__
#define __PROCESS_HOOK_H__

#include <string>
#include <Windows.h>

class ProcessHook 
{
public:
	ProcessHook(std::string const & processName);
	~ProcessHook();

	bool isRunning();
	HANDLE getHandle();
private:
	HANDLE scanForProcess();

	std::string const processName;
	HANDLE hProcess;
};

#endif
