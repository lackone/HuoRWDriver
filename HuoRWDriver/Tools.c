#include "Tools.h"
#include <intrin.h>

typedef NTSTATUS(NTAPI* ZwProtectVirtualMemoryProc)(
	__in HANDLE ProcessHandle,
	__inout PVOID* BaseAddress,
	__inout PSIZE_T RegionSize,
	__in ULONG NewProtect,
	__out PULONG OldProtect
	);

NTSTATUS NTAPI MyProtectVirtualMemory(
	__in HANDLE ProcessHandle,
	__inout PVOID* BaseAddress,
	__inout PSIZE_T RegionSize,
	__in ULONG NewProtect,
	__out PULONG OldProtect
)
{
	static ZwProtectVirtualMemoryProc func = NULL;

	if (!func)
	{
		UNICODE_STRING name = { 0 };
		RtlInitUnicodeString(&name, L"ZwIsProcessInJob");
		PUCHAR job = (PUCHAR)MmGetSystemRoutineAddress(&name);

		if (job)
		{
			job += 20;
			for (int i = 0; i < 0x100; i++)
			{
				if (job[i] == 0x48 && job[i + 1] == 0x8B && job[i + 2] == 0xC4)
				{
					func = (ZwProtectVirtualMemoryProc)(job + i);
					break;
				}
			}
		}
	}

	if (func)
	{
		return func(ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
	}

	return STATUS_NOT_IMPLEMENTED;
}

ULONG64 wpOff()
{
	//¹Ø±ÕÖÐ¶Ï
	_disable();
	ULONG64 cr0 = __readcr0();
	__writecr0(cr0 & (~0x10000));

	return cr0;
}

VOID wpOn(ULONG64 cr0)
{
	__writecr0(cr0);
	_enable();
}

VOID KernelSleep(ULONG64 ms, BOOLEAN alert)
{
	LARGE_INTEGER time = { 0 };
	time.QuadPart = ms * -10000;
	KeDelayExecutionThread(KernelMode, alert, &time);
}