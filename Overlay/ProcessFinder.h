#ifndef __PROCESS_FINDER_H__
#define __PROCESS_FINDER_H__

#include <string>
#include <Windows.h>

class ProcessFinder 
{
public:
	ProcessFinder(std::string const & processName);
	~ProcessFinder();

	std::string const & getProcessName() const;
	bool isRunning() const;
	HANDLE getHandle() const;
private:
	HANDLE scanForProcess();

	std::string const processName;
	HANDLE hProcess;
};

#endif
