#include "pch.h"
#include "ExportApi.h"
#include "LoadDriver.h"
#include <time.h>
#include "../HuoRWDriver/Comm/CommStruct.h"
#include "CommR3.h"

//��ĸ��
CHAR gLetterTable[62] = { 0 };

VOID initLetterTable()
{
	if (gLetterTable[0] != 0)
	{
		return;
	}
	int ix = 0;
	for (CHAR i = 'A'; i <= 'Z'; i++, ix++)
	{
		gLetterTable[ix] = i;
	}
	for (CHAR i = 'a'; i <= 'z'; i++, ix++)
	{
		gLetterTable[ix] = i;
	}
	for (CHAR i = '0'; i <= '9'; i++, ix++)
	{
		gLetterTable[ix] = i;
	}
}

/**
 * �����������
 */
CHAR* GetRandDriverName()
{
	static CHAR* driverName = NULL;
	if (driverName)
	{
		return driverName;
	}

	initLetterTable();

	SIZE_T size = sizeof(CHAR) * 20;
	driverName = (CHAR*)malloc(size);
	if (driverName == NULL)
	{
		return NULL;
	}
	memset(driverName, 0, size);

	int len = (rand() % 10) + 5;

	for (int i = 0; i < len; i++)
	{
		int ix = rand() % sizeof(gLetterTable);

		driverName[i] = gLetterTable[ix];
	}

	strcat_s(driverName, size, ".sys");

	return driverName;
}

/**
 * �����������
 */
CHAR* GetRandServiceName()
{
	static CHAR* serviceName = NULL;
	if (serviceName)
	{
		return serviceName;
	}

	initLetterTable();

	SIZE_T size = sizeof(CHAR) * 10;
	serviceName = (CHAR*)malloc(size);
	if (serviceName == NULL)
	{
		return NULL;
	}
	memset(serviceName, 0, size);

	int len = (rand() % 4) + 5;

	for (int i = 0; i < len; i++)
	{
		int ix = rand() % sizeof(gLetterTable);

		serviceName[i] = gLetterTable[ix];
	}

	return serviceName;
}

EXTERN_C BOOLEAN WINAPI HRW_DriverLoad()
{
	if (HRW_test())
	{
		return TRUE;
	}

	srand(time(NULL));

	LoadDriver load;

	CHAR tmpPath[MAX_PATH] = { 0 };
	GetTempPathA(MAX_PATH, tmpPath);

	CHAR* driverName = GetRandDriverName();
	CHAR* serviceName = GetRandServiceName();

	printf("%s %s \r\n", driverName, serviceName);

	strcat_s(tmpPath, MAX_PATH, driverName);

	printf("%s \r\n", tmpPath);

	load.installDriver(tmpPath, serviceName);

	//�Ѳ���Ϊ׼��������û�ӳɹ�
	return HRW_test();
}

EXTERN_C VOID WINAPI HRW_UnDriverLoad()
{
	LoadDriver load;

	CHAR* serviceName = GetRandServiceName();

	load.unload(serviceName);
}

EXTERN_C BOOLEAN WINAPI HRW_test()
{
	ULONG64 test = 0;
	return DriverComm(CMD_TEST, &test, sizeof(test));
}

EXTERN_C ULONG64 WINAPI HRW_GetModule(DWORD pid, CHAR* moduleName)
{
	ModuleInfo mi = { 0 };
	mi.moduleName = (ULONG64)moduleName;
	mi.pid = pid;
	DriverComm(CMD_GET_MODULE, &mi, sizeof(ModuleInfo));

	return mi.moduleBase;
}

EXTERN_C BOOLEAN WINAPI HRW_ReadMemory(DWORD pid, ULONG64 baseAddr, PVOID buf, ULONG size)
{
	ReadWriteInfo info = { 0 };
	info.pid = pid;
	info.baseAddr = baseAddr;
	info.buf = (ULONG64)buf;
	info.size = size;

	return DriverComm(CMD_READ_MEMORY, &info, sizeof(ReadWriteInfo));
}

EXTERN_C BOOLEAN WINAPI HRW_WriteMemory(DWORD pid, ULONG64 baseAddr, PVOID buf, ULONG size)
{
	ReadWriteInfo info = { 0 };
	info.pid = pid;
	info.baseAddr = baseAddr;
	info.buf = (ULONG64)buf;
	info.size = size;

	return DriverComm(CMD_WRITE_MEMORY, &info, sizeof(ReadWriteInfo));
}

EXTERN_C BOOLEAN WINAPI HRW_QueryMemory(DWORD pid, ULONG64 baseAddr, PMMEMORY_BASIC_INFORMATION basicInfo)
{
	QueryMemoryInfo info = { 0 };
	info.pid = pid;
	info.baseAddr = baseAddr;

	BOOLEAN ret = DriverComm(CMD_QUERY_MEMORY, &info, sizeof(QueryMemoryInfo));

	memcpy(basicInfo, &info.basicInfo, sizeof(MMEMORY_BASIC_INFORMATION));

	return ret;
}

EXTERN_C BOOLEAN WINAPI HRW_ProcessProtect(DWORD pid)
{
	ProcessProtectInfo info = { 0 };
	info.pid = pid;

	return DriverComm(CMD_PROCESS_PROTECT, &info, sizeof(ProcessProtectInfo));
}

EXTERN_C BOOLEAN WINAPI HRW_RemoteCall(DWORD pid, PVOID shellCode, DWORD shellCodeSize)
{
	RemoteCallInfo info = { 0 };

	info.pid = pid;
	info.shellCode = (ULONG64)shellCode;
	info.shellCodeSize = shellCodeSize;

	return DriverComm(CMD_REMOTE_CALL, &info, sizeof(RemoteCallInfo));
}

EXTERN_C BOOLEAN WINAPI HRW_ProcessFake(DWORD fakePid, DWORD srcPid)
{
	ProcessFakeInfo info = { 0 };
	info.fakePid = fakePid;
	info.srcPid = srcPid;

	return DriverComm(CMD_PROCESS_FAKE, &info, sizeof(ProcessFakeInfo));
}