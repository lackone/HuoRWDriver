#include <ntifs.h>
#include "Tools.h"
#include "Module.h"
#include "Comm/Comm.h"
#include "Comm/CommStruct.h"
#include "RWMemory.h"
#include "Export.h"

HANDLE regHandle = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	Log("DriverUnload");

	if (regHandle)
	{
		ObUnRegisterCallbacks(regHandle);
	}

	UnRegCommCallback();
}

ULONG NTAPI DispatchComm(PCommPackage package)
{
	PVOID data = package->inData;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	Log("DispatchComm id %llx cmd %llx data %llx %llx", package->id, package->cmd, package->inData, *(PULONG64)package->inData);

	switch (package->cmd)
	{
	case CMD_TEST:
	{
		status = STATUS_SUCCESS;
	}
	break;
	case CMD_GET_MODULE:
	{
		PModuleInfo info = (PModuleInfo)data;
		if (info)
		{
			ULONG64 size = 0;
			info->moduleBase = GetModuleR3(info->pid, info->moduleName, &size);
			info->moduleSize = size;
		}

		status = STATUS_SUCCESS;
	}
	break;
	case CMD_READ_MEMORY:
	{
		PReadWriteInfo info = (PReadWriteInfo)data;

		if (info)
		{
			status = ReadMemory2(info->pid, info->baseAddr, info->buf, info->size);
		}
	}
	break;
	case CMD_WRITE_MEMORY:
	{
		PReadWriteInfo info = (PReadWriteInfo)data;

		if (info)
		{
			status = WriteMemory(info->pid, info->baseAddr, info->buf, info->size);
		}
	}
	break;
	case CMD_QUERY_MEMORY:
	{
		PQueryMemoryInfo info = (PQueryMemoryInfo)data;

		if (info)
		{
			status = QueryMemory(info->pid, info->baseAddr, &info->basicInfo);
		}
	}
	break;
	default:
		status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

OB_PREOP_CALLBACK_STATUS preCallback(
	_In_ PVOID RegistrationContext,
	_Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
	OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = PROCESS_ALL_ACCESS;
	OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess = PROCESS_ALL_ACCESS;
	OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = PROCESS_ALL_ACCESS;
	OperationInformation->Parameters->DuplicateHandleInformation.OriginalDesiredAccess = PROCESS_ALL_ACCESS;

	return OB_PREOP_SUCCESS;
}

VOID postCallback(
	_In_ PVOID RegistrationContext,
	_In_ POB_POST_OPERATION_INFORMATION OperationInformation
)
{
	OperationInformation->Parameters->CreateHandleInformation.GrantedAccess = PROCESS_ALL_ACCESS;
	OperationInformation->Parameters->DuplicateHandleInformation.GrantedAccess = PROCESS_ALL_ACCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{
	RegCommCallback(DispatchComm);

	/*
	OB_OPERATION_REGISTRATION obOp = { 0 };
	obOp.ObjectType = PsProcessType;
	obOp.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	obOp.PostOperation = postCallback;
	obOp.PreOperation = preCallback;

	OB_CALLBACK_REGISTRATION obCallReg = { 0 };
	obCallReg.Version = ObGetFilterVersion();
	obCallReg.OperationRegistrationCount = 1;
	obCallReg.RegistrationContext = NULL;
	obCallReg.OperationRegistration = &obOp;

	UNICODE_STRING altitude = { 0 };;
	RtlInitUnicodeString(&altitude, L"999999");

	obCallReg.Altitude = altitude;

	PKLDR_DATA_TABLE_ENTRY ldr = (PKLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
	ldr->Flags |= 0x20;

	ObRegisterCallbacks(&obCallReg, &regHandle);
	*/

	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}