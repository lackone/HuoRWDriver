#include <ntifs.h>
#include "Tools.h"
#include "Module.h"
#include "Comm/Comm.h"
#include "Comm/CommStruct.h"
#include "RWMemory.h"
#include "Export.h"
#include "ProcessProtect.h"
#include "RemoteCall.h"
#include "ProcessFake.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	Log("DriverUnload");

	UnRegCommCallback();

	DestoryObRegister();
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
	case CMD_PROCESS_PROTECT:
	{
		PProcessProtectInfo info = (PProcessProtectInfo)data;

		if (info)
		{
			status = SetProtectPid(info->pid);
		}
	}
	break;
	case CMD_REMOTE_CALL:
	{
		PRemoteCallInfo info = (PRemoteCallInfo)data;

		if (info)
		{
			status = RemoteCall(info->pid, info->shellCode, info->shellCodeSize);
		}
	}
	break;
	case CMD_PROCESS_FAKE:
	{
		PProcessFakeInfo info = (PProcessFakeInfo)data;

		if (info)
		{
			FakeProcessByPid(info->fakePid, info->srcPid);
			status = STATUS_SUCCESS;
		}
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

	InitObRegister();

	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}