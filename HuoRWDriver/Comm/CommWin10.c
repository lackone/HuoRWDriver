#include "CommWin10.h"
#include "CommStruct.h"
#include "../SearchCode.h"

CommCallbackProc g_CommCallbackWin10 = NULL;

typedef NTSTATUS(NTAPI* xKdEnumerateDebuggingDevicesProc)(PVOID p1, PVOID p2, PVOID p3);

xKdEnumerateDebuggingDevicesProc OldxKdEnumerateDebuggingDevicesFunc = NULL;

PULONG64 UnxKdEnumerateDebuggingDevices = NULL;

NTSTATUS NTAPI NewxKdEnumerateDebuggingDevicesFunc(PVOID info, PVOID p2, PVOID p3)
{
	if (MmIsAddressValid(info))
	{
		PCommPackage package = (PCommPackage)info;

		if (package->id == 0x12345678)
		{
			package->retStatus = g_CommCallbackWin10(package);
		}
		else
		{
			if (OldxKdEnumerateDebuggingDevicesFunc)
			{
				OldxKdEnumerateDebuggingDevicesFunc(info, p2, p3);
			}
		}
	}
}

NTSTATUS RegCommCallbackWin10(CommCallbackProc callback)
{
	//NtConvertBetweenAuxiliaryCounterAndPerformanceCounter
	PUCHAR func = searchCode("ntoskrnl.exe", "PAGE", "488B05????75?488B05????E8", 0);

	if (func)
	{
		//注意这里要用有符号的
		ULONG64	offset = *(PLONG)(func + 3);
		PULONG64 repFunc = (PULONG64)((func + 7) + offset);

		UnxKdEnumerateDebuggingDevices = repFunc;

		//替换函数
		OldxKdEnumerateDebuggingDevicesFunc = repFunc[0];
		repFunc[0] = NewxKdEnumerateDebuggingDevicesFunc;

		g_CommCallbackWin10 = callback;
	}

	return STATUS_SUCCESS;
}

VOID UnRegCommCallbackWin10()
{
	if (OldxKdEnumerateDebuggingDevicesFunc)
	{
		UnxKdEnumerateDebuggingDevices[0] = OldxKdEnumerateDebuggingDevicesFunc;
	}

	OldxKdEnumerateDebuggingDevicesFunc = NULL;
}