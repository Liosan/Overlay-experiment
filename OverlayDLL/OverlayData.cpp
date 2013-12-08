#include "OverlayData.h"

OverlayData * OverlayData::singleton = NULL;

OverlayData * OverlayData::getSingleton()
{
	return OverlayData::singleton;
}

void OverlayData::allocateSingleton()
{
	if (OverlayData::singleton == NULL)
	{
		OverlayData::singleton = new OverlayData();
	}
}
