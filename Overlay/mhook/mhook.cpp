//Copyright (c) 2007, Marton Anka
//
//Permission is hereby granted, free of charge, to any person obtaining a 
//copy of this software and associated documentation files (the "Software"), 
//to deal in the Software without restriction, including without limitation 
//the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//and/or sell copies of the Software, and to permit persons to whom the 
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included 
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
//IN THE SOFTWARE.

#include "mhook.h"
#include "tlhelp32.h"

//=========================================================================
// The trampoline structure - stores every bit of info about a hook
typedef struct _tagMHOOKS_TRAMPOLINE {
	PBYTE	pSystemFunction;			// the original system function
	PBYTE	pOriginalCode;				// the first few bytes of the system function
	DWORD	cbOriginalCode;				// number of bytes pointed to by above
	PBYTE	pHookFunction;				// the hook function that we provide
	BYTE	codeJumpToHookFunction[32];	// placeholder for code that jumps to the hook function
	BYTE	codeTrampoline[32];			// placeholder for code that holds the first few
										//   bytes from the system function and a jump to the remainder
										//   in the original location
} MHOOKS_TRAMPOLINE;

//=========================================================================
// Global vars
static BOOL g_bVarsInitialized = FALSE;
static CRITICAL_SECTION g_cs;
static MHOOKS_TRAMPOLINE* g_pHooks[MHOOKS_MAX_SUPPORTED_HOOKS];
static DWORD g_nHooksInUse = 0;
static HANDLE* g_hThreadHandles = NULL;
static DWORD g_nThreadHandles = 0;

//=========================================================================
// Toolhelp defintions so the functions can be dynamically bound to
typedef HANDLE (WINAPI * _CreateToolhelp32Snapshot)(
	DWORD dwFlags,       
	DWORD th32ProcessID  
	);

typedef BOOL (WINAPI * _Thread32First)(
									   HANDLE hSnapshot,     
									   LPTHREADENTRY32 lpte
									   );

typedef BOOL (WINAPI * _Thread32Next)(
									  HANDLE hSnapshot,     
									  LPTHREADENTRY32 lpte
									  );

//=========================================================================
// Bring in the toolhelp functions from kernel32
_CreateToolhelp32Snapshot fnCreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot) GetProcAddress(GetModuleHandle(L"kernel32"), "CreateToolhelp32Snapshot");
_Thread32First fnThread32First = (_Thread32First) GetProcAddress(GetModuleHandle(L"kernel32"), "Thread32First");
_Thread32Next fnThread32Next = (_Thread32Next) GetProcAddress(GetModuleHandle(L"kernel32"), "Thread32Next");

//=========================================================================
static VOID EnterCritSec() {
	if (!g_bVarsInitialized) {
		InitializeCriticalSection(&g_cs);
		ZeroMemory(g_pHooks, sizeof(g_pHooks));
		g_bVarsInitialized = TRUE;
	}
	EnterCriticalSection(&g_cs);
}

//=========================================================================
static VOID LeaveCritSec() {
	LeaveCriticalSection(&g_cs);
}

//=========================================================================
// Internal function:
// 
// Skip over jumps that lead to the real function. Gets around import
// jump tables, etc.
//=========================================================================
static PBYTE SkipJumps(PBYTE pbCode) {
#ifdef _M_IX86_X64
	if (pbCode[0] == 0xff && pbCode[1] == 0x25) {
#ifdef _M_IX86
		// on x86 we have an absolute pointer...
		PBYTE pbTarget = *(PBYTE *)&pbCode[2];
		// ... that shows us an absolute pointer.
		return SkipJumps(*(PBYTE *)pbTarget);
#elif defined _M_X64
		// on x64 we have a 32-bit offset...
		INT32 lOffset = *(INT32 *)&pbCode[2];
		// ... that shows us an absolute pointer
		return SkipJumps(*(PBYTE*)(pbCode + 6 + lOffset));
#endif
	} else if (pbCode[0] == 0xe9) {
		// here the behavior is identical, we have...
		// ...a 32-bit offset to the destination.
		return SkipJumps(pbCode + 5 + *(INT32 *)&pbCode[1]);
	} else if (pbCode[0] == 0xeb) {
		// and finally an 8-bit offset to the destination
		return SkipJumps(pbCode + 2 + *(CHAR *)&pbCode[1]);
	}
#else
#error unsupported platform
#endif
	return pbCode;
}

//=========================================================================
// Internal function:
//
// Writes code at pbCode that jumps to pbJumpTo. Will attempt to do this
// in as few bytes as possible. Important on x64 where the long jump
// (0xff 0x25 ....) can take up 14 bytes.
//=========================================================================
static PBYTE EmitJump(PBYTE pbCode, PBYTE pbJumpTo) {
#ifdef _M_IX86_X64
	PBYTE pbJumpFrom = pbCode + 5;
	SIZE_T cbDiff = pbJumpFrom > pbJumpTo ? pbJumpFrom - pbJumpTo : pbJumpTo - pbJumpFrom;
	ODPRINTF((L"Jumping from %p to %p, diff is %p", pbJumpFrom, pbJumpTo, cbDiff));
	if (cbDiff <= 0x7fff0000) {
		pbCode[0] = 0xe9;
		pbCode += 1;
		*((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbJumpTo - pbJumpFrom);
		pbCode += sizeof(DWORD);
	} else {
		pbCode[0] = 0xff;
		pbCode[1] = 0x25;
		pbCode += 2;
#ifdef _M_IX86
		// on x86 we write an absolute address (just behind the instruction)
		*((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbCode + sizeof(DWORD));
#elif defined _M_X64
		// on x64 we write the relative address of the same location
		*((PDWORD)pbCode) = (DWORD)0;
#endif
		pbCode += sizeof(DWORD);
		*((PDWORD_PTR)pbCode) = (DWORD_PTR)(pbJumpTo);
		pbCode += sizeof(DWORD_PTR);
	}
#else 
#error unsupported platform
#endif
	return pbCode;
}

//=========================================================================
// Internal function:
//
// Will try to allocate the trampoline structure within 2 gigabytes of
// the target function. 
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineAlloc(PBYTE pSystemFunction) {

	MHOOKS_TRAMPOLINE* pTrampoline = NULL;

	// do we have room to store this guy?
	if (g_nHooksInUse < MHOOKS_MAX_SUPPORTED_HOOKS) {

		// determine lower and upper bounds for the allocation locations
		PBYTE pLower = pSystemFunction < (PBYTE)(DWORD_PTR)0x0000000080000000 ? 
							NULL : (PBYTE)(pSystemFunction - (PBYTE)0x7fff0000);
		PBYTE pUpper = pSystemFunction < (PBYTE)(DWORD_PTR)0xffffffff80000000 ? 
			(PBYTE)(pSystemFunction + (DWORD_PTR)0x7ff80000) : (PBYTE)(DWORD_PTR)0xfffffffffff80000;
		ODPRINTF((L"Allocating for %p between %p and %p", pSystemFunction, pLower, pUpper));

		// go through the available memory blocks and try to allocate a chunk for us
		for (PBYTE pbAlloc = pLower; pbAlloc < pUpper;) {
			// determine current state
			MEMORY_BASIC_INFORMATION mbi;
			ODPRINTF((L"Looking at %p", pbAlloc));
			if (!VirtualQuery(pbAlloc, &mbi, sizeof(mbi)))
				break;
			// free & large enough?
			if (mbi.State == MEM_FREE && mbi.RegionSize >= sizeof(MHOOKS_TRAMPOLINE)) {
				// yes, grab it
				pTrampoline = (MHOOKS_TRAMPOLINE*)VirtualAlloc(pbAlloc, sizeof(MHOOKS_TRAMPOLINE), MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READ);
				if (pTrampoline) {
					ODPRINTF((L"Allocated %p as the trampoline", pTrampoline));
					break;
				}
			}
			// continue the search
			pbAlloc = (PBYTE)mbi.BaseAddress + mbi.RegionSize;
		}

		// found one
		if (pTrampoline) {
			// put it into our list so we know we'll have to free it
			for (DWORD i=0; i<MHOOKS_MAX_SUPPORTED_HOOKS; i++) {
				if (g_pHooks[i] == NULL) {
					g_pHooks[i] = pTrampoline;
					g_nHooksInUse++;
					break;
				}
			}
		}
	}

	return pTrampoline;
}

//=========================================================================
// Internal function:
//
// Return the internal trampoline structure that belongs to a hooked function.
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineGet(PBYTE pHookedFunction) {
	for (DWORD i=0; i<MHOOKS_MAX_SUPPORTED_HOOKS; i++) {
		if (g_pHooks[i]) {
			if (g_pHooks[i]->codeTrampoline == pHookedFunction)
				return g_pHooks[i];
		}
	}
	return NULL;
}

//=========================================================================
// Internal function:
//
// Free a trampoline structure.
//=========================================================================
static VOID TrampolineFree(MHOOKS_TRAMPOLINE* pTrampoline, BOOL bNeverUsed) {
	for (DWORD i=0; i<MHOOKS_MAX_SUPPORTED_HOOKS; i++) {
		if (g_pHooks[i] == pTrampoline) {
			g_pHooks[i] = NULL;
			// It might be OK to call VirtualFree, but most likely it isn't: 
			// If a thread has some of our trampoline code on its stack
			// and we yank the region from underneath it then it will
			// surely crash upon returning. So instead of freeing the 
			// memory we just let it leak. Ugly...
			if (bNeverUsed)
				VirtualFree(pTrampoline, 0, MEM_RELEASE);
			g_nHooksInUse--;
			break;
		}
	}
}

//=========================================================================
// Internal function:
//
// Suspend a given thread and try to make sure that its instruction
// pointer is not in the given range.
//=========================================================================
static HANDLE SuspendOneThread(DWORD dwThreadId, PBYTE pbCode, DWORD cbBytes) {
	// open the thread
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
	if (GOOD_HANDLE(hThread)) {
		// attempt suspension
		DWORD dwSuspendCount = SuspendThread(hThread);
		if (dwSuspendCount != -1) {
			// see where the IP is
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_CONTROL;
			int nTries = 0;
			while (GetThreadContext(hThread, &ctx)) {
#ifdef _M_IX86
				PBYTE pIp = (PBYTE)ctx.Eip;
#elif defined _M_X64
				PBYTE pIp = (PBYTE)ctx.Rip;
#endif
				if (pIp >= pbCode && pIp < (pbCode + cbBytes)) {
					if (nTries < 3) {
						// oops - we should try to get the instruction pointer out of here. 
						ODPRINTF((L"mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE", dwThreadId, pIp));
						ResumeThread(hThread);
						Sleep(100);
						SuspendThread(hThread);
						nTries++;
					} else {
						// we gave it all we could.
						ODPRINTF((L"mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE - CAN'T FIX", dwThreadId, pIp));
						ResumeThread(hThread);
						CloseHandle(hThread);
						hThread = NULL;
						break;
					}
				} else {
					// success, the IP is not conflicting
					ODPRINTF((L"mhooks: SuspendOneThread: Successfully suspended thread %d - IP is at %p", dwThreadId, pIp));
					break;
				}
			}
		} else {
			// couldn't suspend
			CloseHandle(hThread);
			hThread = NULL;
		}
	}
	return hThread;
}

//=========================================================================
// Internal function:
//
// Resumes all previously suspended threads in the current process.
//=========================================================================
static VOID ResumeOtherThreads() {
	// make sure things go as fast as possible
	INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	// go through our list
	for (DWORD i=0; i<g_nThreadHandles; i++) {
		// and resume & close thread handles
		ResumeThread(g_hThreadHandles[i]);
		CloseHandle(g_hThreadHandles[i]);
	}
	// clean up
	free(g_hThreadHandles);
	g_hThreadHandles = NULL;
	g_nThreadHandles = 0;
	SetThreadPriority(GetCurrentThread(), nOriginalPriority);
}

//=========================================================================
// Internal function:
//
// Suspend all threads in this process while trying to make sure that their 
// instruction pointer is not in the given range.
//=========================================================================
static BOOL SuspendOtherThreads(PBYTE pbCode, DWORD cbBytes) {
	BOOL bRet = FALSE;
	// make sure we're the most important thread in the process
	INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	// get a view of the threads in the system
	HANDLE hSnap = fnCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if (GOOD_HANDLE(hSnap)) {
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		// count threads in this process (except for ourselves)
		DWORD nThreadsInProcess = 0;
		if (fnThread32First(hSnap, &te)) {
			do {
				if (te.th32OwnerProcessID == GetCurrentProcessId()) {
					if (te.th32ThreadID != GetCurrentThreadId()) {
						nThreadsInProcess++;
					}
				}
				te.dwSize = sizeof(te);
			} while(fnThread32Next(hSnap, &te));
		}
		ODPRINTF((L"mhooks: SuspendOtherThreads: counted %d other threads", nThreadsInProcess));
		if (nThreadsInProcess) {
			// alloc buffer for the handles we really suspended
			g_hThreadHandles = (HANDLE*)malloc(nThreadsInProcess*sizeof(HANDLE));
			if (g_hThreadHandles) {
				ZeroMemory(g_hThreadHandles, nThreadsInProcess*sizeof(HANDLE));
				DWORD nCurrentThread = 0;
				BOOL bFailed = FALSE;
				te.dwSize = sizeof(te);
				// go through every thread
				if (fnThread32First(hSnap, &te)) {
					do {
						if (te.th32OwnerProcessID == GetCurrentProcessId()) {
							if (te.th32ThreadID != GetCurrentThreadId()) {
								// attempt to suspend it
								g_hThreadHandles[nCurrentThread] = SuspendOneThread(te.th32ThreadID, pbCode, cbBytes);
								if (GOOD_HANDLE(g_hThreadHandles[nCurrentThread])) {
									ODPRINTF((L"mhooks: SuspendOtherThreads: successfully suspended %d", te.th32ThreadID));
									nCurrentThread++;
								} else {
									ODPRINTF((L"mhooks: SuspendOtherThreads: error while suspending thread %d: %d", te.th32ThreadID, gle()));
									// TODO: this might not be the wisest choice
									// but we can choose to ignore failures on
									// thread suspension:
									// bFailed = TRUE;
								}
							}
						}
						te.dwSize = sizeof(te);
					} while(fnThread32Next(hSnap, &te) && !bFailed);
				}
				g_nThreadHandles = nCurrentThread;
				bRet = !bFailed;
			}
		}
		CloseHandle(hSnap);
		//TODO: we might want to have another go to make sure all threads
		// in the current process (including those that might have been
		// created since we took the original snapshot) have been 
		// suspended.
	} else {
		ODPRINTF((L"mhooks: SuspendOtherThreads: can't CreateToolhelp32Snapshot: %d", gle()));
	}
	SetThreadPriority(GetCurrentThread(), nOriginalPriority);
	if (!bRet) {
		ODPRINTF((L"mhooks: SuspendOtherThreads: Had a problem, resuming all threads."));
		ResumeOtherThreads();
	}
	return bRet;
}

//=========================================================================
BOOL Mhook_SetHook(PVOID *ppSystemFunction, PVOID pHookFunction, PBYTE pStoredSystemFunction, DWORD dwInstructionLength) {
	PVOID pSystemFunction = *ppSystemFunction;
	// ensure thread-safety
	EnterCritSec();
	ODPRINTF((L"mhooks: Mhook_SetHook: Started on the job: %p / %p / %p / %d", pSystemFunction, pHookFunction, pStoredSystemFunction, dwInstructionLength));
	// find the real functions (jump over jump tables, if any)
	pSystemFunction = SkipJumps((PBYTE)pSystemFunction);
	pHookFunction   = SkipJumps((PBYTE)pHookFunction);
	ODPRINTF((L"mhooks: Mhook_SetHook: Started on the job: %p / %p / %p / %d", pSystemFunction, pHookFunction, pStoredSystemFunction, dwInstructionLength));
	// suspend every other thread in this process, and make sure their IP 
	// is not in the code we're about to overwrite.
	SuspendOtherThreads((PBYTE)pSystemFunction, dwInstructionLength);
	// allocate a trampoline structure (note: it is pretty wasteful to get
	// VirtualAlloc to grab chunks of memory smaller than 100 bytes)
	MHOOKS_TRAMPOLINE* pTrampoline = TrampolineAlloc((PBYTE)pSystemFunction);
	if (pTrampoline) {
		ODPRINTF((L"mhooks: Mhook_SetHook: allocated structure at %p", pTrampoline));
		// open ourselves so we can VirtualProtectEx
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		if (GOOD_HANDLE(hProc)) {
			ODPRINTF((L"mhooks: Mhook_SetHook: process opened"));
			DWORD dwOldProtectSystemFunction = 0;
			DWORD dwOldProtectTrampolineFunction = 0;
			// set the system function to PAGE_EXECUTE_READWRITE
			if (VirtualProtectEx(hProc, pSystemFunction, dwInstructionLength, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction)) {
				ODPRINTF((L"mhooks: Mhook_SetHook: readwrite set on system function"));
				// make sure the code is what we believed it would be
				if (!memcmp(pStoredSystemFunction, pSystemFunction, dwInstructionLength)) {
					ODPRINTF((L"mhooks: Mhook_SetHook: memcmp checks out"));
					// mark our trampoline buffer to PAGE_EXECUTE_READWRITE
					if (VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtectTrampolineFunction)) {
						ODPRINTF((L"mhooks: Mhook_SetHook: readwrite set on trampoline structure"));

						// create our trampoline function
						PBYTE pbCode = pTrampoline->codeTrampoline;
						// whatever was there in the beginning...
						for (DWORD i = 0; i<dwInstructionLength; i++) {
							pbCode[i] = pStoredSystemFunction[i];
						}
						pbCode += dwInstructionLength;
						// plus a jump to the continuation in the original location
						pbCode = EmitJump(pbCode, ((PBYTE)pSystemFunction) + dwInstructionLength);
						ODPRINTF((L"mhooks: Mhook_SetHook: updated the trampoline"));

						// create the function that jumps to the hook (we need this because 
						// jumping from the API to the hook directly might be a long jump, 
						// which is 14 bytes on x64, and we want to avoid that - the API
						// may or may not have room for such stuff. we do though.)
						pbCode = pTrampoline->codeJumpToHookFunction;
						pbCode = EmitJump(pbCode, (PBYTE)pHookFunction);
						ODPRINTF((L"mhooks: Mhook_SetHook: created reverse trampoline"));

						// update the API itself
						pbCode = (PBYTE)pSystemFunction;
						pbCode = EmitJump(pbCode, pTrampoline->codeJumpToHookFunction);
						ODPRINTF((L"mhooks: Mhook_SetHook: Hooked the function!"));

						// update data members
						pTrampoline->cbOriginalCode = dwInstructionLength;
						pTrampoline->pOriginalCode = pStoredSystemFunction;
						pTrampoline->pSystemFunction = (PBYTE)pSystemFunction;
						pTrampoline->pHookFunction = (PBYTE)pHookFunction;

						// flush instruction cache and restore original protection
						FlushInstructionCache(hProc, pTrampoline->codeTrampoline, dwInstructionLength);
						VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction);
					} else {
						ODPRINTF((L"mhooks: Mhook_SetHook: failed VirtualProtectEx 2: %d", gle()));
					}
				} else {
					ODPRINTF((L"mhooks: Mhook_SetHook: failed memcmp"));
				}
				// flush instruction cache and restore original protection
				FlushInstructionCache(hProc, pSystemFunction, dwInstructionLength);
				VirtualProtectEx(hProc, pSystemFunction, dwInstructionLength, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
			} else {
				ODPRINTF((L"mhooks: Mhook_SetHook: failed VirtualProtectEx 1: %d", gle()));
			}
			CloseHandle(hProc);
		} else {
			ODPRINTF((L"mhooks: Mhook_SetHook: could not open process: %d", gle()));
		}
		if (pTrampoline->pSystemFunction) {
			*ppSystemFunction = pTrampoline->codeTrampoline;
		} else {
			// if we failed discard the trampoline (forcing VirtualFree)
			TrampolineFree(pTrampoline, TRUE);
			pTrampoline = NULL;
		}
	}
	// resume everybody else
	ResumeOtherThreads();
	LeaveCritSec();
	return (pTrampoline != NULL);
}

//=========================================================================
BOOL Mhook_Unhook(PVOID *ppHookedFunction) {
	ODPRINTF((L"mhooks: Mhook_Unhook: %p", *ppHookedFunction));
	BOOL bRet = FALSE;
	EnterCritSec();
	// get the trampoline structure that corresponds to our function
	MHOOKS_TRAMPOLINE* pTrampoline = TrampolineGet((PBYTE)*ppHookedFunction);
	if (pTrampoline) {
		// make sure nobody's executing code where we're about to overwrite a few bytes
		SuspendOtherThreads(pTrampoline->pSystemFunction, pTrampoline->cbOriginalCode);
		ODPRINTF((L"mhooks: Mhook_Unhook: found struct at %p", pTrampoline));
		// open ourselves so we can VirtualProtectEx
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		if (GOOD_HANDLE(hProc)) {
			ODPRINTF((L"mhooks: Mhook_Unhook: process opened"));
			DWORD dwOldProtectSystemFunction = 0;
			// make memory writable
			if (VirtualProtectEx(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOriginalCode, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction)) {
				ODPRINTF((L"mhooks: Mhook_Unhook: readwrite set on system function"));
				PBYTE pbCode = (PBYTE)pTrampoline->pSystemFunction;
				for (DWORD i = 0; i<pTrampoline->cbOriginalCode; i++) {
					pbCode[i] = pTrampoline->pOriginalCode[i];
				}
				// flush instruction cache and make memory unwriteable
				FlushInstructionCache(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOriginalCode);
				VirtualProtectEx(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOriginalCode, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
				// return the original function pointer
				*ppHookedFunction = pTrampoline->pSystemFunction;
				bRet = TRUE;
				ODPRINTF((L"mhooks: Mhook_Unhook: sysfunc: %p", *ppHookedFunction));
				// free the trampoline while not really discarding it from memory
				TrampolineFree(pTrampoline, FALSE);
				ODPRINTF((L"mhooks: Mhook_Unhook: unhook successful"));
			} else {
				ODPRINTF((L"mhooks: Mhook_Unhook: failed VirtualProtectEx 1: %d", gle()));
			}
			CloseHandle(hProc);
		} else {
			ODPRINTF((L"mhooks: Mhook_Unhook: could not open process: %d", gle()));
		}
		// make the other guys runnable
		ResumeOtherThreads();
	}
	LeaveCritSec();
	return bRet;
}

//=========================================================================
