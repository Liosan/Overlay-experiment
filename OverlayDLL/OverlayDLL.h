#ifndef __OVERLAYDLL_H__
#define __OVERLAYDLL_H__

#ifdef OVERLAYDLL_EXPORTS
#define OVERLAYDLL_API __declspec(dllexport)
#else
#define OVERLAYDLL_API __declspec(dllimport)
#endif

#endif
