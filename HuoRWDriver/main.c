#include <ntifs.h>
#include "Tools.h"
#include "Module.h"
#include "Comm/Comm.h"
#include "Comm/CommStruct.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	Log("DriverUnload");

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
	default:
		status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{
	RegCommCallback(DispatchComm);

	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}