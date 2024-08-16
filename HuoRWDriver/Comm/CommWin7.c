#include "CommWin7.h"
#include "CommStruct.h"
#include "../Tools.h"

CommCallbackProc g_CommCallbackWin7 = NULL;

FileCallback oldExpDisQueryAttributeInformationFunc = NULL;
FileCallback oldExpDisSetAttributeInformationFunc = NULL;
PULONG64 UnExpDisQueryAttributeInformation = NULL;

NTSTATUS QueryFileCallback(HANDLE fileHandle, PVOID info, PVOID p1, PVOID p2)
{
	Log("QueryFileCallback");

	if (MmIsAddressValid(info))
	{
		PCommPackage package = (PCommPackage)info;

		if (package->id == 0x12345678)
		{
			package->retStatus = g_CommCallbackWin7(package);
		}
		else
		{
			if (oldExpDisQueryAttributeInformationFunc)
			{
				oldExpDisQueryAttributeInformationFunc(fileHandle, info, p1, p2);
			}
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS SetFileCallback(HANDLE fileHandle, PVOID info, PVOID p1, PVOID p2)
{
	Log("SetFileCallback");

	if (MmIsAddressValid(info))
	{
		PCommPackage package = (PCommPackage)info;

		if (package->id == 0x12345678)
		{
			package->retStatus = g_CommCallbackWin7(package);
		}
		else
		{
			if (oldExpDisSetAttributeInformationFunc)
			{
				return oldExpDisSetAttributeInformationFunc(fileHandle, info, p1, p2);
			}
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS RegCommCallbackWin7(CommCallbackProc callback)
{
	UNICODE_STRING uName = { 0 };
	RtlInitUnicodeString(&uName, L"ExRegisterAttributeInformationCallback");
	PUCHAR func = MmGetSystemRoutineAddress(&uName);

	//64λ�£�ȫ�ֱ������� RIP+Offset
	//ע������Ҫ���з��ŵ�
	ULONG64	offset = *(PLONG)(func + 0xD + 3);
	PULONG64 ExpDisQueryAttributeInformation = (PULONG64)((func + 0xD + 7) + offset);

	//������ֵ
	oldExpDisQueryAttributeInformationFunc = (FileCallback)ExpDisQueryAttributeInformation[0];
	oldExpDisSetAttributeInformationFunc = (FileCallback)ExpDisQueryAttributeInformation[1];

	//���
	ExpDisQueryAttributeInformation[0] = 0;
	//��8�ֽڣ����� ExpDisSetAttributeInformation
	ExpDisQueryAttributeInformation[1] = 0;

	//��������
	RegisterCallback reg = { 0 };
	reg.QueryFileCallback = QueryFileCallback;
	reg.SetFileCallback = SetFileCallback;

	//���ú���
	ExRegisterAttributeInformationCallbackProc callFunc = (ExRegisterAttributeInformationCallbackProc)func;
	NTSTATUS status = callFunc(&reg);

	if (NT_SUCCESS(status))
	{
		g_CommCallbackWin7 = callback;
		UnExpDisQueryAttributeInformation = ExpDisQueryAttributeInformation;
	}

	return status;
}

VOID UnRegCommCallbackWin7()
{
	if (UnExpDisQueryAttributeInformation)
	{
		UnExpDisQueryAttributeInformation[0] = oldExpDisQueryAttributeInformationFunc;
		UnExpDisQueryAttributeInformation[1] = oldExpDisSetAttributeInformationFunc;
	}
}