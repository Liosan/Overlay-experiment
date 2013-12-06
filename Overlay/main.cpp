#include "OverlayInjector.h"
#include <iostream>

int main(int argc, char * argv[])
{
	OverlayInjector overlayInjector("witcher.exe");
	overlayInjector.run();
	std::cin.get();
	return 0;
}
