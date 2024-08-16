#include "Comm.h"
#include "../Tools.h"
#include "CommWin7.h"
#include "CommWin10.h"


NTSTATUS RegCommCallback(CommCallbackProc callback)
{
	RTL_OSVERSIONINFOEXW version = { 0 };
	RtlGetVersion(&version);

	if (version.dwBuildNumber == 7600 || version.dwBuildNumber == 7601)
	{
		return RegCommCallbackWin7(callback);
	}

	return RegCommCallbackWin10(callback);
}

VOID UnRegCommCallback()
{
	RTL_OSVERSIONINFOEXW version = { 0 };
	RtlGetVersion(&version);

	if (version.dwBuildNumber == 7600 || version.dwBuildNumber == 7601)
	{
		UnRegCommCallbackWin7();
		return;
	}

	UnRegCommCallbackWin10();
}