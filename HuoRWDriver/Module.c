#include "Module.h"
#include "Export.h"
#include "Tools.h"

ULONG_PTR GetModuleX86(PEPROCESS process, PPEB32 peb32, PUNICODE_STRING moduleName, ULONG_PTR *sizeOfImage)
{
	SIZE_T retSize = 0;
	//�޸�����ֹȱҳ
	MmCopyVirtualMemory(process, peb32, process, peb32, sizeof(PEB32), UserMode, &retSize);

	PPEB_LDR_DATA32 ldr = (PPEB_LDR_DATA32)peb32->Ldr;

	PLIST_ENTRY32 list32 = (PLIST_ENTRY32)&ldr->InLoadOrderModuleList;
	PLDR_DATA_TABLE_ENTRY32 listNext = (PLDR_DATA_TABLE_ENTRY32)list32->Flink;

	ULONG_PTR moduleBase = 0;

	//����������ѯģ��
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
	//�޸�����ֹȱҳ
	MmCopyVirtualMemory(process, peb, process, peb, sizeof(PEB), UserMode, &retSize);

	PPEB_LDR_DATA ldr = (PPEB_LDR_DATA)peb->Ldr;

	PLIST_ENTRY list = (PLIST_ENTRY)&ldr->InLoadOrderModuleList;
	PLDR_DATA_TABLE_ENTRY listNext = (PLDR_DATA_TABLE_ENTRY)list->Flink;

	ULONG_PTR moduleBase = 0;

	//����������ѯģ��
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

	//ת����д
	_wcsupr(uModuleName.Buffer);

	//���ӽ���
	KeStackAttachProcess(process, &apc);

	do 
	{
		//3����ַ�����������������޸����ˣ���ȱҳ
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