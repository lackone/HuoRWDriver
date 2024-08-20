#include "Tools.h"

ULONG_PTR QueryModule(PUCHAR moduleName, ULONG_PTR *moduleSize)
{
	RTL_PROCESS_MODULES rtlModules = { 0 };
	PRTL_PROCESS_MODULES modules = &rtlModules;
	BOOLEAN isAllocate = FALSE;

	//测量长度
	ULONG ret = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, modules, sizeof(RTL_PROCESS_MODULES), &ret);

	//分配实际内存
	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		modules = ExAllocatePool(PagedPool, ret + sizeof(PRTL_PROCESS_MODULES));
		if (!modules)
		{
			return 0;
		}
		memset(modules, 0, ret + sizeof(PRTL_PROCESS_MODULES));

		//再查一次
		status = ZwQuerySystemInformation(SystemModuleInformation, modules, ret + sizeof(PRTL_PROCESS_MODULES), &ret);

		if (!NT_SUCCESS(status))
		{
			ExFreePool(modules);
			return 0;
		}

		isAllocate = TRUE;
	}

	PUCHAR copyModuleName = NULL;
	ULONG_PTR moduleBase = 0;

	do 
	{
		if (_stricmp(moduleName, "ntoskrnl.exe") == 0 || _stricmp(moduleName, "ntkrnlpa.exe") == 0)
		{
			PRTL_PROCESS_MODULE_INFORMATION moduleInfo = &modules->Modules[0];
			moduleBase = moduleInfo->ImageBase;
			if (moduleSize)
			{
				*moduleSize = moduleInfo->ImageSize;
			}
			break;
		}

		copyModuleName = ExAllocatePool(PagedPool, strlen(moduleName) + 1);
		if (copyModuleName == NULL)
		{
			break;
		}
		memset(copyModuleName, 0, strlen(moduleName) + 1);
		memcpy(copyModuleName, moduleName, strlen(moduleName));
		_strupr(copyModuleName);

		for (int i = 0; i < modules->NumberOfModules; i++)
		{
			PRTL_PROCESS_MODULE_INFORMATION moduleInfo = &modules->Modules[i];
			PUCHAR pathName = _strupr(moduleInfo->FullPathName);

			DbgPrintEx(77, 0, "FileName = %s FullPathName = %s \r\n",
				moduleInfo->FullPathName + moduleInfo->OffsetToFileName,
				moduleInfo->FullPathName
			);

			if (strstr(pathName, copyModuleName))
			{
				moduleBase = moduleInfo->ImageBase;
				if (moduleSize)
				{
					*moduleSize = moduleInfo->ImageSize;
				}
				break;
			}
		}

	} while (0);


	if (copyModuleName)
	{
		ExFreePool(copyModuleName);
	}

	if (isAllocate)
	{
		ExFreePool(modules);
	}

	return moduleBase;
}

/**
 * 删除注册表项
 */
NTSTATUS DeleteRegisterPath(PUNICODE_STRING pReg)
{
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"DisplayName");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"ErrorControl");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"ImagePath");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"Start");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"Type");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, pReg->Buffer, L"WOW64");

	PWCH enumPath = (PWCH)ExAllocatePool(PagedPool, pReg->MaximumLength + 0x100);
	if (enumPath == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	memset(enumPath, 0, pReg->MaximumLength + 0x100);
	memcpy(enumPath, pReg->Buffer, pReg->Length);
	wcscat_s(enumPath, pReg->MaximumLength + 0x100, L"\\Enum");

	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, enumPath, L"enumPath");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, enumPath, L"INITSTARTFAILED");
	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, enumPath, L"NextInstance");

	//删除Enum
	HANDLE hEnum = NULL;
	OBJECT_ATTRIBUTES enumObj = { 0 };
	UNICODE_STRING enumName = { 0 };
	RtlInitUnicodeString(&enumName, enumPath);
	InitializeObjectAttributes(&enumObj, &enumName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	NTSTATUS status = ZwOpenKey(&hEnum, KEY_ALL_ACCESS, &enumObj);

	if (NT_SUCCESS(status))
	{
		ZwDeleteKey(hEnum);
		ZwClose(hEnum);
	}

	ExFreePool(enumPath);

	//删除根部
	HANDLE hRoot = NULL;
	OBJECT_ATTRIBUTES rootObj = { 0 };
	InitializeObjectAttributes(&rootObj, pReg, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwOpenKey(&hRoot, KEY_ALL_ACCESS, &rootObj);

	if (NT_SUCCESS(status))
	{
		ZwDeleteKey(hRoot);
		ZwClose(hRoot);
		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS DeleteSelfFile(PWCH filePath)
{
	HANDLE hFile = NULL;

	OBJECT_ATTRIBUTES objAttr = { 0 };
	UNICODE_STRING name = { 0 };
	RtlInitUnicodeString(&name, filePath);

	InitializeObjectAttributes(&objAttr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	IO_STATUS_BLOCK ioBlock = { 0 };

	NTSTATUS status = ZwCreateFile(
		&hFile,
		GENERIC_READ, 
		&objAttr, 
		&ioBlock, 
		NULL, 
		FILE_ATTRIBUTE_NORMAL, 
		FILE_SHARE_READ, 
		FILE_OPEN_IF, 
		FILE_NON_DIRECTORY_FILE, 
		NULL, 
		NULL
	);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	PFILE_OBJECT pFileObj = NULL;

	status = ObReferenceObjectByHandle(hFile, FILE_ALL_ACCESS, *IoFileObjectType, KernelMode, &pFileObj, NULL);

	if (!NT_SUCCESS(status))
	{
		ZwClose(hFile);
		return status;
	}

	pFileObj->DeleteAccess = TRUE;
	pFileObj->DeletePending = FALSE;

	pFileObj->SectionObjectPointer->DataSectionObject = NULL;
	pFileObj->SectionObjectPointer->ImageSectionObject = NULL;
	//pFileObj->SectionObjectPointer->SharedCacheMap = NULL;

	ObDereferenceObject(pFileObj);
	ZwClose(hFile);

	status = ZwDeleteFile(&objAttr);

	return status;
}