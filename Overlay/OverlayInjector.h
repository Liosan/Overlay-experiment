#ifndef __OVERLAY_H__
#define __OVERLAY_H__

#include "ProcessHook.h"

class OverlayInjector
{
public:
	OverlayInjector(std::string const & processName);

	// will not return until the hooked application terminates
	void run();
private:
	void injectDll();

	ProcessHook processHook;
};


#endif
