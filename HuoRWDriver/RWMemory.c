#include "RWMemory.h"
#include "Export.h"
#include <intrin.h>
#include "Tools.h"

PVOID MdlMapMemory(PMDL* mdl, PVOID targetAddr, SIZE_T size, MODE preMode)
{
	//ֻ�����MDL�ṹ
	PMDL pmdl = IoAllocateMdl(targetAddr, size, FALSE, FALSE, NULL);
	PVOID mapAddr = NULL;

	if (!pmdl)
	{
		return NULL;
	}

	BOOLEAN isLock = FALSE;
	__try
	{
		//���ڴ棬���ܻ���쳣
		MmProbeAndLockPages(pmdl, preMode, IoReadAccess);
		isLock = TRUE;
		//MmMapLockedPages() ���APIӳ��ʧ�ܻ�����
		mapAddr = MmMapLockedPagesSpecifyCache(pmdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
	}
	__except (1)
	{
		if (isLock)
		{
			MmUnlockPages(pmdl);
		}
		IoFreeMdl(pmdl);

		return NULL;
	}

	*mdl = pmdl;
	
	return mapAddr;
}

VOID MdlUnMapMemory(PMDL mdl, PVOID mapBase)
{
	__try
	{
		MmUnmapLockedPages(mapBase, mdl);

		MmUnlockPages(mdl);

		IoFreeMdl(mdl);
	}
	__except (1)
	{
		return;
	}
}

NTSTATUS ReadMemory1(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size)
{
	if (((ULONG64)targetAddr >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size < (ULONG64)targetAddr))
	{
		return STATUS_ACCESS_VIOLATION;
	}

	if (buf == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_INVALID_PARAMETER_1;
	}

	PVOID mem = ExAllocatePool(NonPagedPool, size);
	if (mem == NULL)
	{
		ObDereferenceObject(process);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	memset(mem, 0, size);
	KAPC_STATE apc = { 0 };
	
	//���ӽ���
	KeStackAttachProcess(process, &apc);

	status = STATUS_UNSUCCESSFUL;

	//�жϵ�ַ�Ƿ���Ч
	if (MmIsAddressValid(targetAddr) && MmIsAddressValid((PVOID)((ULONG64)targetAddr + size)))
	{
		memcpy(mem, targetAddr, size);
		status = STATUS_SUCCESS;
	}

	KeUnstackDetachProcess(&apc);

	if (NT_SUCCESS(status))
	{
		memcpy(buf, mem, size);
	}

	ExFreePool(mem);

	ObDereferenceObject(process);

	return status;
}

NTSTATUS ReadMemory2(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size)
{
	if (((ULONG64)targetAddr >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size < (ULONG64)targetAddr))
	{
		return STATUS_ACCESS_VIOLATION;
	}

	if (buf == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_INVALID_PARAMETER_1;
	}

	SIZE_T retSize = 0;
	status = MmCopyVirtualMemory(process, targetAddr, IoGetCurrentProcess(), buf, size, UserMode, &retSize);

	ObDereferenceObject(process);

	return status;
}

NTSTATUS ReadMemory3(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size)
{
	if (((ULONG64)targetAddr >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size < (ULONG64)targetAddr))
	{
		return STATUS_ACCESS_VIOLATION;
	}

	if (buf == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_INVALID_PARAMETER_1;
	}

	PVOID mem = ExAllocatePool(NonPagedPool, size);
	if (mem == NULL)
	{
		ObDereferenceObject(process);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	memset(mem, 0, size);

	KAPC_STATE apc = { 0 };

	//���ӽ���
	KeStackAttachProcess(process, &apc);

	status = STATUS_UNSUCCESSFUL;

	//�жϵ�ַ�Ƿ���Ч
	if (MmIsAddressValid(targetAddr) && MmIsAddressValid((PVOID)((ULONG64)targetAddr + size)))
	{
		PMDL mdl = NULL;
		PVOID map = MdlMapMemory(&mdl, targetAddr, size, UserMode);
		if (map)
		{
			memcpy(mem, map, size);
			MdlUnMapMemory(mdl, map);
		}

		status = STATUS_SUCCESS;
	}

	KeUnstackDetachProcess(&apc);

	if (NT_SUCCESS(status))
	{
		memcpy(buf, mem, size);
	}

	ExFreePool(mem);

	ObDereferenceObject(process);

	return status;
}

NTSTATUS ReadMemory4(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size)
{
	if (((ULONG64)targetAddr >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size < (ULONG64)targetAddr))
	{
		return STATUS_ACCESS_VIOLATION;
	}

	if (buf == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_INVALID_PARAMETER_1;
	}

	PVOID mem = ExAllocatePool(NonPagedPool, size);
	if (mem == NULL)
	{
		ObDereferenceObject(process);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	memset(mem, 0, size);

	status = STATUS_UNSUCCESSFUL;


	ULONG64 newCR3 = *(PULONG64)((PUCHAR)process + 0x28);
	ULONG64 oldCR3 = __readcr3();

	//�ص�APC
	KeEnterCriticalRegion();

	_disable();
	__writecr3(newCR3);

	//�жϵ�ַ�Ƿ���Ч
	if (MmIsAddressValid(targetAddr) && MmIsAddressValid((PVOID)((ULONG64)targetAddr + size)))
	{
		memcpy(mem, targetAddr, size);
		status = STATUS_SUCCESS;
	}

	_enable();
	__writecr3(oldCR3);

	//�ָ�APC
	KeLeaveCriticalRegion();

	if (NT_SUCCESS(status))
	{
		memcpy(buf, mem, size);
	}

	ExFreePool(mem);

	ObDereferenceObject(process);

	return status;
}


NTSTATUS WriteMemory(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size)
{
	if (((ULONG64)targetAddr >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size >= MmHighestUserAddress) ||
		((ULONG64)targetAddr + size < (ULONG64)targetAddr))
	{
		return STATUS_ACCESS_VIOLATION;
	}

	if (buf == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_INVALID_PARAMETER_1;
	}

	//��һ�γ���д��
	SIZE_T retSize = 0;
	status = MmCopyVirtualMemory(IoGetCurrentProcess(), buf, process, targetAddr, size, UserMode, retSize);

	//����ɹ�
	if (NT_SUCCESS(status))
	{
		ObDereferenceObject(process);
		return status;
	}

	KAPC_STATE apc = { 0 };
	PEPROCESS srcProcess = IoGetCurrentProcess();

	KeStackAttachProcess(process, &apc);

	PVOID addr = targetAddr;
	SIZE_T tmpSize = size;
	ULONG oldProtect = 0;

	//�޸�ҳ����
	status = MyProtectVirtualMemory(NtCurrentProcess(), &addr, &tmpSize, PAGE_EXECUTE_READWRITE, &oldProtect);

	if (NT_SUCCESS(status))
	{
		retSize = 0;
		//�ٴ�д��
		status = MmCopyVirtualMemory(srcProcess, buf, process, targetAddr, size, UserMode, retSize);

		MyProtectVirtualMemory(NtCurrentProcess(), &addr, &tmpSize, oldProtect, &oldProtect);
	}

	KeUnstackDetachProcess(&apc);

	if (!NT_SUCCESS(status))
	{
		ULONG64 cr0 = wpOff();

		retSize = 0;
		//�ٴ�д��
		status = MmCopyVirtualMemory(srcProcess, buf, process, targetAddr, size, UserMode, retSize);

		wpOn(cr0);
	}

	ObDereferenceObject(process);

	return status;
}