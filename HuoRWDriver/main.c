#include <ntifs.h>
#include "Tools.h"
#include "Module.h"
#include "Comm/Comm.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	Log("DriverUnload");

	UnRegCommCallback();
}

ULONG NTAPI DispatchComm(PCommPackage package)
{
	Log("DispatchComm id %llx cmd %llx data %llx %llx", package->id, package->cmd, package->inData, *(PULONG64)package->inData);

	return 0;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{
	RegCommCallback(DispatchComm);

	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}