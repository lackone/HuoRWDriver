#include "Module.h"
#include "Export.h"
#include "Tools.h"

ULONG_PTR GetModuleX86(PEPROCESS process, PPEB32 peb32, PUNICODE_STRING moduleName, ULONG_PTR *sizeOfImage)
{
	SIZE_T retSize = 0;
	//修复，防止缺页
	MmCopyVirtualMemory(process, peb32, process, peb32, sizeof(PEB32), UserMode, &retSize);

	PPEB_LDR_DATA32 ldr = (PPEB_LDR_DATA32)peb32->Ldr;

	PLIST_ENTRY32 list32 = (PLIST_ENTRY32)&ldr->InLoadOrderModuleList;
	PLDR_DATA_TABLE_ENTRY32 listNext = (PLDR_DATA_TABLE_ENTRY32)list32->Flink;

	ULONG_PTR moduleBase = 0;

	//遍历链表，查询模块
	while (list32 != listNext)
	{
		PWCH BaseDllName = listNext->BaseDllName.Buffer;

		UNICODE_STRING uBaseDllName = { 0 };
		RtlInitUnicodeString(&uBaseDllName, BaseDllName);

		if (RtlCompareUnicodeString(&uBaseDllName, moduleName, TRUE) == 0)
		{
			Log("DllBase = %llx SizeOfImage = %llx BaseDllName = %wZ", listNext->DllBase, listNext->SizeOfImage, uBaseDllName);

			moduleBase = listNext->DllBase;

			if (sizeOfImage)
			{
				*sizeOfImage = listNext->SizeOfImage;
			}

			break;
		}

		listNext = (PLDR_DATA_TABLE_ENTRY32)(listNext->InLoadOrderLinks.Flink);
	}

	return moduleBase;
}

ULONG_PTR GetModuleX64(PEPROCESS process, PPEB peb, PUNICODE_STRING moduleName, ULONG_PTR* sizeOfImage)
{
	SIZE_T retSize = 0;
	//修复，防止缺页
	MmCopyVirtualMemory(process, peb, process, peb, sizeof(PEB), UserMode, &retSize);

	PPEB_LDR_DATA ldr = (PPEB_LDR_DATA)peb->Ldr;

	PLIST_ENTRY list = (PLIST_ENTRY)&ldr->InLoadOrderModuleList;
	PLDR_DATA_TABLE_ENTRY listNext = (PLDR_DATA_TABLE_ENTRY)list->Flink;

	ULONG_PTR moduleBase = 0;

	//遍历链表，查询模块
	while (list != listNext)
	{
		PWCH BaseDllName = listNext->BaseDllName.Buffer;

		UNICODE_STRING uBaseDllName = { 0 };
		RtlInitUnicodeString(&uBaseDllName, BaseDllName);

		if (RtlCompareUnicodeString(&uBaseDllName, moduleName, TRUE) == 0)
		{
			Log("DllBase = %llx SizeOfImage = %llx BaseDllName = %wZ", listNext->DllBase, listNext->SizeOfImage, uBaseDllName);

			moduleBase = listNext->DllBase;

			if (sizeOfImage)
			{
				*sizeOfImage = listNext->SizeOfImage;
			}

			break;
		}

		listNext = (PLDR_DATA_TABLE_ENTRY)(listNext->InLoadOrderLinks.Flink);
	}

	return moduleBase;
}

ULONG_PTR GetModuleR3(HANDLE pid, PCHAR moduleName, ULONG_PTR* sizeOfImage)
{
	if (!moduleName)
	{
		return 0;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return 0;
	}

	ULONG_PTR moduleBase = 0;

	KAPC_STATE apc = { 0 };

	STRING aModuleName = { 0 };
	RtlInitAnsiString(&aModuleName, moduleName);

	UNICODE_STRING uModuleName = { 0 };
	status = RtlAnsiStringToUnicodeString(&uModuleName, &aModuleName, TRUE);
	if (!NT_SUCCESS(status))
	{
		return 0;
	}

	//转换大写
	_wcsupr(uModuleName.Buffer);

	//附加进程
	KeStackAttachProcess(process, &apc);

	do 
	{
		//3环地址，又是隐藏驱动，修复不了，会缺页
		PPEB peb = (PPEB)PsGetProcessPeb(process);

		PPEB32 peb32 = (PPEB32)PsGetProcessWow64Process(process);

		if (peb32)
		{
			moduleBase = GetModuleX86(process, peb32, &uModuleName, sizeOfImage);
		}
		else
		{
			moduleBase = GetModuleX64(process, peb, &uModuleName, sizeOfImage);
		}

	} while (0);

	KeUnstackDetachProcess(&apc);

	RtlFreeUnicodeString(&uModuleName);

	return moduleBase;
}