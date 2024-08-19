#include "ProcessFake.h"
#include "Export.h"

/**
 * 	获取SeAuditProcessCreationInfo偏移
 */
ULONG GetLocateProcessImageNameOffset()
{
	RTL_OSVERSIONINFOW version = { 0 };

	RtlGetVersion(&version);

	UNICODE_STRING fnName = { 0 };
	RtlInitUnicodeString(&fnName, L"PsGetProcessPeb");
	PUCHAR fnAddr = (PUCHAR)MmGetSystemRoutineAddress(&fnName);

	ULONG pebOffset = *(PULONG)(fnAddr + 3); //拿到PEB的offset

	ULONG seOffset = 0;

	//win7 58
	//win11 70
	//win10 1507  68
	if (version.dwBuildNumber == 7600 || version.dwBuildNumber == 7601)
	{
		seOffset = 0x58;
	}
	else if(version.dwBuildNumber > 7601 && version.dwBuildNumber <= 10240) 
	{
		seOffset = 0x68;
	}
	else 
	{
		seOffset = 0x70;
	}

	return pebOffset + seOffset;
}

ULONG GetProcessFileObjectOffset()
{
	RTL_OSVERSIONINFOEXW version = { 0 };
	RtlGetVersion(&version);

	if (version.dwMajorVersion == 10)
	{
		if (version.dwBuildNumber != 10240)
		{
			UNICODE_STRING name = { 0 };

			//获取 ImageFilePointer
			RtlInitUnicodeString(&name, L"PsGetProcessImageFileName");
			PUCHAR addr = (PUCHAR)MmGetSystemRoutineAddress(&name);
			ULONG offset = *(PULONG)(addr + 3);

			if (offset)
			{
				offset -= 8;
			}

			return offset;
		}
	}

	return 0;
}

PVOID GetTokenUserSidPointer(PVOID token)
{
	RTL_OSVERSIONINFOW version = { 0 };

	RtlGetVersion(&version);

	int offset = 0;

	PVOID result = NULL;

	if (version.dwBuildNumber == 7600 || version.dwBuildNumber == 7601)
	{
		offset = 0x90;
	}
	else
	{
		offset = 0x98;
	}

	if (offset)
	{
		//获取 UserAndGroups
		ULONG64 userGs = *(PULONG64)((ULONG64)token + offset);
		if (userGs)
		{
			result = (PVOID)(*(PULONG64)userGs);
		}
	}

	return result;
}

/**
 * 设置ImageName
 */
VOID resetProcessImageName(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	PUCHAR fakeImageName = PsGetProcessImageFileName(fakeProcess);
	PUCHAR srcImageName = PsGetProcessImageFileName(srcProcess);

	memcpy(fakeImageName, srcImageName, 15);
}

/**
 * 设置全路径
 */
VOID resetProcessFullName(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	PUNICODE_STRING srcName = NULL;

	NTSTATUS status = SeLocateProcessImageName(srcProcess, &srcName);
	if (!NT_SUCCESS(status))
	{
		return;
	}

	ULONG seOffset = GetLocateProcessImageNameOffset();

	POBJECT_NAME_INFORMATION fakeNameInfo = (POBJECT_NAME_INFORMATION)*((PULONG64)((PUCHAR)fakeProcess + seOffset));

	if (fakeNameInfo->Name.Length >= srcName->Length)
	{
		memset(fakeNameInfo->Name.Buffer, 0, fakeNameInfo->Name.MaximumLength);
		memcpy(fakeNameInfo->Name.Buffer, srcName->Buffer, srcName->Length);
	}
	else 
	{
		//申请一块内存
		SIZE_T size = srcName->MaximumLength + sizeof(UNICODE_STRING);
		PUNICODE_STRING uname = (PUNICODE_STRING)ExAllocatePoolWithTag(NonPagedPool, size, 'fIeS');

		if (uname)
		{
			uname->MaximumLength = srcName->MaximumLength;

			uname->Length = srcName->Length;

			uname->Buffer = (PWCH)((PUCHAR)uname + sizeof(UNICODE_STRING));

			memcpy(uname->Buffer, srcName->Buffer, srcName->Length);

			ExFreePool(fakeNameInfo);

			//_SE_AUDIT_PROCESS_CREATION_INFO 结构占8字节
			*((PULONG64)((PUCHAR)fakeProcess + seOffset)) = (ULONG64)uname;
		}
	}

	ExFreePool(srcName);
}

/**
 * 设置文件对象名字
 */
VOID resetProcessFileObjectName(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	PFILE_OBJECT srcFileObj = NULL;
	PFILE_OBJECT fakeFileObj = NULL;

	NTSTATUS status = PsReferenceProcessFilePointer(srcProcess, &srcFileObj);
	if (!NT_SUCCESS(status))
	{
		return;
	}

	status = PsReferenceProcessFilePointer(fakeProcess, &fakeFileObj);
	if (!NT_SUCCESS(status))
	{
		ObDereferenceObject(srcFileObj);
		return;
	}

	PUNICODE_STRING fakeName = &fakeFileObj->FileName;
	PUNICODE_STRING srcName = &srcFileObj->FileName;

	PWCH tmpFakeName = NULL;

	if (fakeName->Length >= srcName->Length)
	{
		memset(fakeName->Buffer, 0, fakeName->MaximumLength);
		memcpy(fakeName->Buffer, srcName->Buffer, srcName->Length);

		tmpFakeName = fakeName->Buffer;
	}
	else 
	{
		SIZE_T size = srcName->MaximumLength;
		tmpFakeName = (PWCH)ExAllocatePool(NonPagedPool, size);

		if (tmpFakeName == NULL)
		{
			ObDereferenceObject(srcFileObj);
			ObDereferenceObject(fakeFileObj);
			return;
		}

		memset(tmpFakeName, 0, size);
		memcpy(tmpFakeName, srcName->Buffer, srcName->Length);

		fakeName->Buffer = tmpFakeName;
	}

	fakeName->Length = srcName->Length;
	fakeName->MaximumLength = srcName->MaximumLength;

	ULONG64 fsContext2 = *(PULONG64)((PUCHAR)fakeFileObj + 0x20);

	if (MmIsAddressValid((PUCHAR)fsContext2))
	{
		PUNICODE_STRING unfsContextName = (PUNICODE_STRING)(fsContext2 + 0x10);

		if (unfsContextName->Length && unfsContextName->MaximumLength)
		{
			unfsContextName->Buffer = tmpFakeName;
			unfsContextName->Length = fakeName->Length;
			unfsContextName->MaximumLength = fakeName->MaximumLength;
		}
	}

	fakeFileObj->DeviceObject = srcFileObj->DeviceObject;
	fakeFileObj->Vpb = srcFileObj->Vpb;


	ObDereferenceObject(srcFileObj);
	ObDereferenceObject(fakeFileObj);
}

VOID resetProcessFileObjectNameWin10(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	RTL_OSVERSIONINFOW version = { 0 };

	RtlGetVersion(&version);

	if (version.dwMajorVersion < 10)
	{
		return;
	}

	ULONG fileOffset = GetProcessFileObjectOffset();

	PFILE_OBJECT fakeFileObj = (PFILE_OBJECT)*(PULONG64)((PUCHAR)fakeProcess + fileOffset);

	PFILE_OBJECT srcFileObj = (PFILE_OBJECT)*(PULONG64)((PUCHAR)srcProcess + fileOffset);

	OBJECT_NAME_INFORMATION srcFileName;

	PUNICODE_STRING srcName = &srcFileObj->FileName;

	PUNICODE_STRING fakeName = &fakeFileObj->FileName;

	PWCH tmpFakeName = NULL;

	if (fakeName->Length >= srcName->Length)
	{
		memset(fakeName->Buffer, 0, fakeName->MaximumLength);
		memcpy(fakeName->Buffer, srcName->Buffer, srcName->Length);

		tmpFakeName = fakeName->Buffer;
	}
	else
	{
		SIZE_T size = srcName->MaximumLength;
		tmpFakeName = (PWCH)ExAllocatePool(NonPagedPool, size);

		if (tmpFakeName == NULL)
		{
			ObDereferenceObject(srcFileObj);
			ObDereferenceObject(fakeFileObj);
			return;
		}

		memset(tmpFakeName, 0, size);
		memcpy(tmpFakeName, srcName->Buffer, srcName->Length);

		fakeName->Buffer = tmpFakeName;
	}

	fakeName->Length = srcName->Length;
	fakeName->MaximumLength = srcName->MaximumLength;

	ULONG64 fsContext2 = *(PULONG64)((PUCHAR)fakeFileObj + 0x20);

	if (MmIsAddressValid((PUCHAR)fsContext2))
	{
		PUNICODE_STRING unfsContextName = (PUNICODE_STRING)(fsContext2 + 0x10);

		if (unfsContextName->Length && unfsContextName->MaximumLength)
		{
			unfsContextName->Buffer = tmpFakeName;
			unfsContextName->Length = fakeName->Length;
			unfsContextName->MaximumLength = fakeName->MaximumLength;
		}
	}

	fakeFileObj->DeviceObject = srcFileObj->DeviceObject;
	fakeFileObj->Vpb = srcFileObj->Vpb;
}

/**
 * 设置进程TOKEN的UserAndGroups
 */
VOID resetProcessTokenGroup(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	ULONG64 fakeToken = PsReferencePrimaryToken(fakeProcess);
	ULONG64 srcToken = PsReferencePrimaryToken(srcProcess);

	PVOID fvt = GetTokenUserSidPointer((PVOID)fakeToken);
	PVOID svt = GetTokenUserSidPointer((PVOID)srcToken);

	if (fvt && svt)
	{
		memcpy(fvt, svt, 0x20);
	}

	ObDereferenceObject(srcToken);
	ObDereferenceObject(fakeToken);
}

VOID resetProcessPEB64Param(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	PPEB fakePEB = (PPEB)PsGetProcessPeb(fakeProcess);
	PPEB srcPEB = (PPEB)PsGetProcessPeb(srcProcess);

	if (!srcPEB || !fakePEB)
	{
		return;
	}

	KAPC_STATE fakeApc = { 0 };
	KAPC_STATE srcApc = { 0 };

	UNICODE_STRING ImagePathName = { 0 };
	UNICODE_STRING CommandLine = { 0 };
	UNICODE_STRING WindowTitle = { 0 };

	SIZE_T pro = NULL;

	KeStackAttachProcess(srcProcess, &srcApc);

	//防止隐藏驱动读R3内存蓝屏
	MmCopyVirtualMemory(srcProcess, srcPEB, srcProcess, srcPEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(srcProcess, srcPEB->ProcessParameters, srcProcess, srcPEB->ProcessParameters, 1, UserMode, &pro);

	if (srcPEB->ProcessParameters->ImagePathName.Length)
	{
		ImagePathName.Buffer = ExAllocatePool(NonPagedPool, srcPEB->ProcessParameters->ImagePathName.MaximumLength);
		memcpy(ImagePathName.Buffer, srcPEB->ProcessParameters->ImagePathName.Buffer, srcPEB->ProcessParameters->ImagePathName.Length);
		ImagePathName.Length = srcPEB->ProcessParameters->ImagePathName.Length;
		ImagePathName.MaximumLength = srcPEB->ProcessParameters->ImagePathName.MaximumLength;
	}


	if (srcPEB->ProcessParameters->CommandLine.Length)
	{
		CommandLine.Buffer = ExAllocatePool(NonPagedPool, srcPEB->ProcessParameters->CommandLine.MaximumLength);
		memcpy(CommandLine.Buffer, srcPEB->ProcessParameters->CommandLine.Buffer, srcPEB->ProcessParameters->CommandLine.Length);
		CommandLine.Length = srcPEB->ProcessParameters->CommandLine.Length;
		CommandLine.MaximumLength = srcPEB->ProcessParameters->CommandLine.MaximumLength;
	}


	if (srcPEB->ProcessParameters->WindowTitle.Length)
	{
		WindowTitle.Buffer = ExAllocatePool(NonPagedPool, srcPEB->ProcessParameters->WindowTitle.MaximumLength);
		memcpy(WindowTitle.Buffer, srcPEB->ProcessParameters->WindowTitle.Buffer, srcPEB->ProcessParameters->WindowTitle.Length);
		WindowTitle.Length = srcPEB->ProcessParameters->WindowTitle.Length;
		WindowTitle.MaximumLength = srcPEB->ProcessParameters->WindowTitle.MaximumLength;
	}

	KeUnstackDetachProcess(&srcApc);


	KeStackAttachProcess(fakeProcess, &fakeApc);

	MmCopyVirtualMemory(fakeProcess, fakePEB, fakeProcess, fakePEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, fakePEB->ProcessParameters, fakeProcess, fakePEB->ProcessParameters, 1, UserMode, &pro);

	PVOID BaseAddr = NULL;
	SIZE_T size = PAGE_SIZE;
	NTSTATUS status = ZwAllocateVirtualMemory(NtCurrentProcess(), &BaseAddr, 0, &size, MEM_COMMIT, PAGE_READWRITE);
	PUCHAR tempBase = BaseAddr;

	if (fakePEB->ProcessParameters->ImagePathName.Length && ImagePathName.Length)
	{
		if (fakePEB->ProcessParameters->ImagePathName.Length >= ImagePathName.Length)
		{
			memset(fakePEB->ProcessParameters->ImagePathName.Buffer, 0, fakePEB->ProcessParameters->ImagePathName.MaximumLength);

			memcpy(fakePEB->ProcessParameters->ImagePathName.Buffer, ImagePathName.Buffer, ImagePathName.Length);

			fakePEB->ProcessParameters->ImagePathName.Length = ImagePathName.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(fakePEB->ProcessParameters->ImagePathName.Buffer, 0, fakePEB->ProcessParameters->ImagePathName.MaximumLength);
				fakePEB->ProcessParameters->ImagePathName.Length = 0;
				fakePEB->ProcessParameters->ImagePathName.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, ImagePathName.Buffer, ImagePathName.Length);
				fakePEB->ProcessParameters->ImagePathName.Length = ImagePathName.Length;
				fakePEB->ProcessParameters->ImagePathName.MaximumLength = ImagePathName.MaximumLength;
				fakePEB->ProcessParameters->ImagePathName.Buffer = tempBase;
				tempBase += ImagePathName.MaximumLength;
			}
		}
	}

	if (fakePEB->ProcessParameters->CommandLine.Length && CommandLine.Length)
	{
		if (fakePEB->ProcessParameters->CommandLine.Length >= CommandLine.Length)
		{
			memset(fakePEB->ProcessParameters->CommandLine.Buffer, 0, fakePEB->ProcessParameters->CommandLine.MaximumLength);

			memcpy(fakePEB->ProcessParameters->CommandLine.Buffer, CommandLine.Buffer, CommandLine.Length);

			fakePEB->ProcessParameters->CommandLine.Length = CommandLine.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(fakePEB->ProcessParameters->CommandLine.Buffer, 0, fakePEB->ProcessParameters->CommandLine.MaximumLength);
				fakePEB->ProcessParameters->CommandLine.Length = 0;
				fakePEB->ProcessParameters->CommandLine.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, CommandLine.Buffer, CommandLine.Length);
				fakePEB->ProcessParameters->CommandLine.Length = CommandLine.Length;
				fakePEB->ProcessParameters->CommandLine.MaximumLength = CommandLine.MaximumLength;
				fakePEB->ProcessParameters->CommandLine.Buffer = tempBase;
				tempBase += CommandLine.MaximumLength;
			}
		}
	}


	if (fakePEB->ProcessParameters->WindowTitle.Length && WindowTitle.Length)
	{
		if (fakePEB->ProcessParameters->WindowTitle.Length >= WindowTitle.Length)
		{
			memset(fakePEB->ProcessParameters->WindowTitle.Buffer, 0, fakePEB->ProcessParameters->WindowTitle.MaximumLength);

			memcpy(fakePEB->ProcessParameters->WindowTitle.Buffer, WindowTitle.Buffer, WindowTitle.Length);

			fakePEB->ProcessParameters->WindowTitle.Length = WindowTitle.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(fakePEB->ProcessParameters->WindowTitle.Buffer, 0, fakePEB->ProcessParameters->WindowTitle.MaximumLength);
				fakePEB->ProcessParameters->WindowTitle.Length = 0;
				fakePEB->ProcessParameters->WindowTitle.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, WindowTitle.Buffer, WindowTitle.Length);
				fakePEB->ProcessParameters->WindowTitle.Length = WindowTitle.Length;
				fakePEB->ProcessParameters->WindowTitle.MaximumLength = WindowTitle.MaximumLength;
				fakePEB->ProcessParameters->WindowTitle.Buffer = tempBase;
			}
		}
	}
	else
	{
		memset(fakePEB->ProcessParameters->WindowTitle.Buffer, 0, fakePEB->ProcessParameters->WindowTitle.MaximumLength);
		fakePEB->ProcessParameters->WindowTitle.Length = 0;
		fakePEB->ProcessParameters->WindowTitle.MaximumLength = 0;
	}


	KeUnstackDetachProcess(&fakeApc);

	if (ImagePathName.Length) 
	{
		ExFreePool(ImagePathName.Buffer);
	}
	if (CommandLine.Length)
	{
		ExFreePool(CommandLine.Buffer);
	}
	if (WindowTitle.Length)
	{
		ExFreePool(WindowTitle.Buffer);
	}
}

VOID resetProcessPEB64Module(PEPROCESS fakeProcess, PEPROCESS srcProcess)
{
	PPEB fakePEB = (PPEB)PsGetProcessPeb(fakeProcess);

	PPEB srcPEB = (PPEB)PsGetProcessPeb(srcProcess);

	if (!srcPEB || !fakePEB)
	{
		return;
	}

	KAPC_STATE fakeApc = { 0 };

	KAPC_STATE srcApc = { 0 };

	UNICODE_STRING FullDllName = { 0 };
	ULONG baseLen = 0;

	KeStackAttachProcess(srcProcess, &srcApc);

	//防止隐藏驱动读R3内存蓝屏
	SIZE_T pro = NULL;
	MmCopyVirtualMemory(srcProcess, srcPEB, srcProcess, srcPEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(srcProcess, srcPEB->Ldr, srcProcess, srcPEB->Ldr, 1, UserMode, &pro);

	PMLDR_DATA_TABLE_ENTRY list = (PMLDR_DATA_TABLE_ENTRY)srcPEB->Ldr->InLoadOrderModuleList.Flink;

	if (list->FullDllName.Length)
	{
		FullDllName.Buffer = ExAllocatePool(NonPagedPool, list->FullDllName.MaximumLength);

		memcpy(FullDllName.Buffer, list->FullDllName.Buffer, list->FullDllName.Length);

		FullDllName.Length = list->FullDllName.Length;

		FullDllName.MaximumLength = list->FullDllName.MaximumLength;

		baseLen = (PUCHAR)list->BaseDllName.Buffer - (PUCHAR)list->FullDllName.Buffer;
	}

	KeUnstackDetachProcess(&srcApc);

	//附加源进程
	KeStackAttachProcess(fakeProcess, &fakeApc);

	//防止隐藏驱动读R3内存蓝屏
	MmCopyVirtualMemory(fakeProcess, fakePEB, fakeProcess, fakePEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, fakePEB->Ldr, fakeProcess, fakePEB->Ldr, 1, UserMode, &pro);

	PMLDR_DATA_TABLE_ENTRY fakeList = (PMLDR_DATA_TABLE_ENTRY)fakePEB->Ldr->InLoadOrderModuleList.Flink;

	if (fakeList->FullDllName.Length >= FullDllName.Length)
	{
		memset(fakeList->FullDllName.Buffer, 0, fakeList->FullDllName.MaximumLength);

		memcpy(fakeList->FullDllName.Buffer, FullDllName.Buffer, FullDllName.Length);

		fakeList->FullDllName.Length = FullDllName.Length;
	}
	else
	{
		PVOID BaseAddr = NULL;

		SIZE_T size = PAGE_SIZE;

		NTSTATUS status = ZwAllocateVirtualMemory(NtCurrentProcess(), &BaseAddr, 0, &size, MEM_COMMIT, PAGE_READWRITE);

		memcpy(BaseAddr, FullDllName.Buffer, FullDllName.Length);

		fakeList->FullDllName.Length = FullDllName.Length;

		fakeList->FullDllName.MaximumLength = FullDllName.MaximumLength;

		fakeList->FullDllName.Buffer = BaseAddr;
	}

	fakeList->BaseDllName.Buffer = (PUCHAR)fakeList->FullDllName.Buffer + baseLen;
	fakeList->BaseDllName.Length = fakeList->FullDllName.Length - baseLen;
	fakeList->BaseDllName.MaximumLength = baseLen + 2;

	KeUnstackDetachProcess(&fakeApc);

	if (FullDllName.Length)
	{
		ExFreePool(FullDllName.Buffer);
	}
}

VOID resetProcessPEB32Param(PEPROCESS fakeProcess)
{
	PPEB32 peb32 = (PPEB32)PsGetProcessWow64Process(fakeProcess);

	if (!peb32)
	{
		return;
	}

	PPEB fakePEB = (PPEB)PsGetProcessPeb(fakeProcess);

	if (!fakePEB)
	{
		return;
	}

	KAPC_STATE fakeApc = { 0 };

	KeStackAttachProcess(fakeProcess, &fakeApc);

	SIZE_T pro = NULL;
	MmCopyVirtualMemory(fakeProcess, fakePEB, fakeProcess, fakePEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, fakePEB->ProcessParameters, fakeProcess, fakePEB->ProcessParameters, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, peb32, fakeProcess, peb32, 1, UserMode, &pro);

	PRTL_USER_PROCESS_PARAMETERS32 param32 = (PRTL_USER_PROCESS_PARAMETERS32)ULongToPtr(peb32->ProcessParameters);

	MmCopyVirtualMemory(fakeProcess, param32, fakeProcess, param32, 1, UserMode, &pro);

	PVOID BaseAddr = NULL;
	SIZE_T size = PAGE_SIZE;
	NTSTATUS status = ZwAllocateVirtualMemory(NtCurrentProcess(), &BaseAddr, 0, &size, MEM_COMMIT, PAGE_READWRITE);
	PUCHAR tempBase = BaseAddr;

	if (fakePEB->ProcessParameters->ImagePathName.Length)
	{
		if (param32->ImagePathName.Length >= fakePEB->ProcessParameters->ImagePathName.Length)
		{
			memset(param32->ImagePathName.Buffer, 0, param32->ImagePathName.MaximumLength);

			memcpy(param32->ImagePathName.Buffer, fakePEB->ProcessParameters->ImagePathName.Buffer, fakePEB->ProcessParameters->ImagePathName.Length);

			param32->ImagePathName.Length = fakePEB->ProcessParameters->ImagePathName.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(param32->ImagePathName.Buffer, 0, param32->ImagePathName.MaximumLength);
				param32->ImagePathName.Length = 0;
				param32->ImagePathName.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, fakePEB->ProcessParameters->ImagePathName.Buffer, fakePEB->ProcessParameters->ImagePathName.Length);
				param32->ImagePathName.Length = fakePEB->ProcessParameters->ImagePathName.Length;
				param32->ImagePathName.MaximumLength = fakePEB->ProcessParameters->ImagePathName.MaximumLength;
				param32->ImagePathName.Buffer = tempBase;
				tempBase += param32->ImagePathName.MaximumLength;
			}
		}
	}

	if (fakePEB->ProcessParameters->CommandLine.Length)
	{
		if (param32->CommandLine.Length >= fakePEB->ProcessParameters->CommandLine.Length)
		{
			memset(param32->CommandLine.Buffer, 0, param32->CommandLine.MaximumLength);

			memcpy(param32->CommandLine.Buffer, fakePEB->ProcessParameters->CommandLine.Buffer, fakePEB->ProcessParameters->CommandLine.Length);

			param32->CommandLine.Length = fakePEB->ProcessParameters->CommandLine.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(param32->CommandLine.Buffer, 0, param32->CommandLine.MaximumLength);
				param32->CommandLine.Length = 0;
				param32->CommandLine.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, fakePEB->ProcessParameters->CommandLine.Buffer, fakePEB->ProcessParameters->CommandLine.Length);
				param32->CommandLine.Length = fakePEB->ProcessParameters->CommandLine.Length;
				param32->CommandLine.MaximumLength = fakePEB->ProcessParameters->CommandLine.MaximumLength;
				param32->CommandLine.Buffer = tempBase;
				tempBase += param32->CommandLine.MaximumLength;
			}
		}
	}

	if (fakePEB->ProcessParameters->WindowTitle.Length)
	{
		if (param32->WindowTitle.Length >= fakePEB->ProcessParameters->WindowTitle.Length)
		{
			memset(param32->WindowTitle.Buffer, 0, param32->WindowTitle.MaximumLength);

			memcpy(param32->WindowTitle.Buffer, fakePEB->ProcessParameters->WindowTitle.Buffer, fakePEB->ProcessParameters->WindowTitle.Length);

			param32->WindowTitle.Length = fakePEB->ProcessParameters->WindowTitle.Length;
		}
		else
		{
			if (!NT_SUCCESS(status))
			{
				memset(param32->WindowTitle.Buffer, 0, param32->WindowTitle.MaximumLength);
				param32->WindowTitle.Length = 0;
				param32->WindowTitle.MaximumLength = 0;
			}
			else
			{
				memcpy(tempBase, fakePEB->ProcessParameters->WindowTitle.Buffer, fakePEB->ProcessParameters->WindowTitle.Length);
				param32->WindowTitle.Length = fakePEB->ProcessParameters->WindowTitle.Length;
				param32->WindowTitle.MaximumLength = fakePEB->ProcessParameters->WindowTitle.MaximumLength;
				param32->WindowTitle.Buffer = tempBase;
				tempBase += param32->WindowTitle.MaximumLength;
			}
		}
	}
	else
	{
		memset(param32->WindowTitle.Buffer, 0, param32->WindowTitle.MaximumLength);
		param32->WindowTitle.Length = 0;
		param32->WindowTitle.MaximumLength = 0;
	}

	KeUnstackDetachProcess(&fakeApc);
}

VOID resetProcessPEB32Module(PEPROCESS fakeProcess)
{
	PPEB32 peb32 = (PPEB32)PsGetProcessWow64Process(fakeProcess);

	if (!peb32)
	{
		return;
	}

	PPEB fakePEB = (PPEB)PsGetProcessPeb(fakeProcess);

	if (!fakePEB)
	{
		return;
	}

	KAPC_STATE fakeApc = { 0 };

	ULONG baseLen = 0;

	//附加源进程
	KeStackAttachProcess(fakeProcess, &fakeApc);

	SIZE_T pro = NULL;

	//防止隐藏驱动读R3内存蓝屏
	MmCopyVirtualMemory(fakeProcess, fakePEB, fakeProcess, fakePEB, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, fakePEB->Ldr, fakeProcess, fakePEB->Ldr, 1, UserMode, &pro);

	MmCopyVirtualMemory(fakeProcess, peb32, fakeProcess, peb32, 1, UserMode, &pro);

	PPEB_LDR_DATA32 pldr32 = (PPEB_LDR_DATA32)ULongToPtr(peb32->Ldr);

	MmCopyVirtualMemory(fakeProcess, pldr32, fakeProcess, pldr32, 1, UserMode, &pro);


	PMLDR_DATA_TABLE_ENTRY fakeList = (PMLDR_DATA_TABLE_ENTRY)fakePEB->Ldr->InLoadOrderModuleList.Flink;
	PLDR_DATA_TABLE_ENTRY32 fakeList32 = (PLDR_DATA_TABLE_ENTRY32)ULongToPtr(pldr32->InLoadOrderModuleList.Flink);

	if (fakeList32->FullDllName.Length >= fakeList->FullDllName.Length)
	{
		memset(fakeList32->FullDllName.Buffer, 0, fakeList32->FullDllName.MaximumLength);

		memcpy(fakeList32->FullDllName.Buffer, fakeList->FullDllName.Buffer, fakeList->FullDllName.Length);

		fakeList32->FullDllName.Length = fakeList->FullDllName.Length;
	}
	else
	{
		PVOID BaseAddr = NULL;

		SIZE_T size = PAGE_SIZE;

		NTSTATUS status = ZwAllocateVirtualMemory(NtCurrentProcess(), &BaseAddr, 0, &size, MEM_COMMIT, PAGE_READWRITE);

		memcpy(BaseAddr, fakeList->FullDllName.Buffer, fakeList->FullDllName.Length);

		fakeList32->FullDllName.Length = fakeList->FullDllName.Length;

		fakeList32->FullDllName.MaximumLength = fakeList->FullDllName.MaximumLength;

		fakeList32->FullDllName.Buffer = BaseAddr;
	}

	fakeList32->BaseDllName.Buffer = (PUCHAR)fakeList->FullDllName.Buffer + baseLen;
	fakeList32->BaseDllName.Length = fakeList->FullDllName.Length - baseLen;
	fakeList32->BaseDllName.MaximumLength = baseLen + 2;

	KeUnstackDetachProcess(&fakeApc);
}

BOOLEAN FakeProcessByPid(HANDLE fakePid, HANDLE srcPid)
{
	PEPROCESS fakeProcess = NULL;

	NTSTATUS status = PsLookupProcessByProcessId(fakePid, &fakeProcess);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}

	PEPROCESS srcProcess = NULL;

    status = PsLookupProcessByProcessId(srcPid, &srcProcess);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}

	if (PsGetProcessExitStatus(srcProcess) != STATUS_PENDING)
	{
		ObDereferenceObject(srcProcess);
		return FALSE;
	}

	//1、替换名字   EPROCESS+0x2e0 的 imageName
	resetProcessImageName(fakeProcess, srcProcess);

	//2、替换全路径  EPROCESS+0x390 中 SeAuditProcessCreationInfo
	resetProcessFullName(fakeProcess, srcProcess);

	//3、替换文件对象名字 EPROCESS+0x268  中 SectionObject
	resetProcessFileObjectName(fakeProcess, srcProcess);

	//3、替换文件对象名字 WIN10 EPROCESS ImageFilePointer
	resetProcessFileObjectNameWin10(fakeProcess, srcProcess);

	//4、 EPROCESS+0x208 Token
	resetProcessTokenGroup(fakeProcess, srcProcess);

	//5、修改 PEB64
	resetProcessPEB64Param(fakeProcess, srcProcess);

	resetProcessPEB64Module(fakeProcess, srcProcess);

	//6、修改 PEB32
	resetProcessPEB32Param(fakeProcess);

	resetProcessPEB32Module(fakeProcess);

	return TRUE;
}