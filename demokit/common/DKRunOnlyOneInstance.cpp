#include "stdafx.h"
#include "DKRunOnlyOneInstance.h"
#include <assert.h>

static HANDLE g_hEvent = NULL;

bool DKIsExistInstance(const char* pszInstName) {
	assert(pszInstName);
	if (!pszInstName)
		return true;
	HANDLE g_hEvent = CreateMutexA(NULL, FALSE, pszInstName);
	if (!g_hEvent)
		return true;
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		::CloseHandle(g_hEvent);
		g_hEvent = NULL;
		return true;
	}

	return false;
}


void DKClearInstanceFlag() {
	if (g_hEvent) {
		::CloseHandle( g_hEvent );
		g_hEvent = NULL;
	}
}
