#include "ProcessProtect.h"
#include "Export.h"
#include "SearchCode.h"

extern POBJECT_TYPE* IoDriverObjectType;

HANDLE regHandle = NULL;
HANDLE protectPid = NULL;

NTSTATUS SetProtectPid(HANDLE pid)
{
	protectPid = pid;
	return STATUS_SUCCESS;
}

OB_PREOP_CALLBACK_STATUS PreCallback(
	_In_ PVOID RegistrationContext,
	_Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
	PEPROCESS process = OperationInformation->Object;
	if (!process)
	{
		return OB_PREOP_SUCCESS;
	}

	//��ǰ����ID
	HANDLE curPid = PsGetCurrentProcessId();
	//Ŀ�����ID
	HANDLE targetPid = PsGetProcessId(process);

	if (curPid == protectPid)
	{
		return OB_PREOP_SUCCESS;
	}

	if (targetPid != protectPid)
	{
		return OB_PREOP_SUCCESS;
	}

	if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
	{
		OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
		OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess = 0;
	}
	else
	{
		OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;
		OperationInformation->Parameters->DuplicateHandleInformation.OriginalDesiredAccess = 0;
	}

	return OB_PREOP_SUCCESS;
}

/**
 * ��ȡ��������
 */
PDRIVER_OBJECT GetDriverObjectByName(PWCH ObjName)
{
	UNICODE_STRING driverName = { 0 };
	RtlInitUnicodeString(&driverName, ObjName);

	PDRIVER_OBJECT Driver = NULL;

	NTSTATUS status = ObReferenceObjectByName(&driverName, FILE_ALL_ACCESS, 0, 0, *IoDriverObjectType, KernelMode, NULL, &Driver);

	if (NT_SUCCESS(status))
	{
		ObDereferenceObject(Driver);
	}

	return Driver;
}

VOID DestoryObRegister()
{
	if (regHandle)
	{
		ObUnRegisterCallbacks(regHandle);
	}
}

NTSTATUS InitObRegister()
{
	//�õ���������
	PDRIVER_OBJECT pDriver = GetDriverObjectByName(L"\\Driver\\WMIxWDM");

	if (!pDriver)
	{
		return STATUS_UNSUCCESSFUL;
	}

	ULONG64 jmpRcx = searchCode("ntoskrnl.exe", ".text", "FFE1", 0);

	if (!jmpRcx)
	{
		return STATUS_UNSUCCESSFUL;
	}

	OB_OPERATION_REGISTRATION obOp = { 0 };
	obOp.ObjectType = PsProcessType;
	obOp.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	//JMP RCX, 64λ�£���һ��������ͨ�� RCX ����
	obOp.PreOperation = jmpRcx;

	OB_CALLBACK_REGISTRATION obCallReg = { 0 };
	obCallReg.Version = ObGetFilterVersion();
	obCallReg.OperationRegistrationCount = 1;
	//�������õ�һ������RegistrationContextΪPreCallback
	obCallReg.RegistrationContext = PreCallback;
	obCallReg.OperationRegistration = &obOp;

	UNICODE_STRING altitude = { 0 };;
	RtlInitUnicodeString(&altitude, L"678456");
	obCallReg.Altitude = altitude;

	//PKLDR_DATA_TABLE_ENTRY ldr = (PKLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
	//ldr->Flags |= 0x20;

	//��ʼ�򲹶�
	RTL_OSVERSIONINFOEXW version = { 0 };
	RtlGetVersion(&version);
	PUCHAR findFunc = NULL;

	//WIN7
	if (version.dwBuildNumber == 7600 || version.dwBuildNumber == 7601)
	{
		PUCHAR func = (PUCHAR)ObRegisterCallbacks;
		for (int i = 0; i < 0x500; i++)
		{
			//�ҵ�MmVerifyCallbackFunction
			if (func[i] == 0x74 && func[i + 2] == 0xe8 && func[i + 7] == 0x3b && func[i + 8] == 0xc3)
			{
				LARGE_INTEGER larger;
				larger.QuadPart = func + i + 7;
				larger.LowPart += *(PULONG)(func + i + 3);
				findFunc = larger.QuadPart;
				break;
			}
		}
	}
	else
	{
		PUCHAR func = (PUCHAR)ObRegisterCallbacks;
		for (int i = 0; i < 0x500; i++)
		{
			//�ҵ�MmVerifyCallbackFunction
			if (func[i] == 0xBA && func[i + 5] == 0xe8 && func[i + 10] == 0x85 && func[i + 11] == 0xc0)
			{
				LARGE_INTEGER larger;
				larger.QuadPart = func + i + 10;
				larger.LowPart += *(PULONG)(func + i + 6);
				findFunc = larger.QuadPart;
				break;
			}
		}
	}

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (findFunc)
	{
		//��ȡ���������ַ
		PHYSICAL_ADDRESS phy = MmGetPhysicalAddress(findFunc);
		//
		PVOID mem = MmMapIoSpace(phy, 10, MmNonCached);
		if (mem)
		{
			UCHAR bufCode[10] = { 0 };
			//����1
			UCHAR patch[] = { 0xb0, 0x1, 0xc3 };
			memcpy(bufCode, mem, 10);
			memcpy(mem, patch, sizeof(patch));
			status = ObRegisterCallbacks(&obCallReg, &regHandle);
			memcpy(mem, bufCode, 10);
		}
	}

	return status;
}