#ifndef __OVERLAY_INJECTOR_H__
#define __OVERLAY_INJECTOR_H__

#include "ProcessFinder.h"

class OverlayInjector
{
public:
	OverlayInjector(std::string const & processName);

	void run();
private:
	std::string executablePath() const;
	void injectDll();

	ProcessFinder processFinder;
};


#endif
