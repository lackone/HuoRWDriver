#include "pch.h"
#include "CommR3.h"
#include <intrin.h>
#include <tchar.h>
#include "../HuoRWDriver/Comm/CommStruct.h"

typedef struct _IO_STATUS_BLOCK {
	union {
		ULONG Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef ULONG(NTAPI* NtQueryInformationFileProc)(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__out_bcount(Length) PVOID FileInformation,
	__in ULONG Length,
	__in ULONG FileInformationClass
	);

typedef ULONG(WINAPI* NtConvertBetweenAuxiliaryCounterAndPerformanceCounterProc)(char a1, PVOID a2, PVOID a3, PVOID a4);

HANDLE g_file = NULL;
NtQueryInformationFileProc g_NtQueryInformationFile = NULL;

NtConvertBetweenAuxiliaryCounterAndPerformanceCounterProc g_NtConvertBetweenAuxiliaryCounterAndPerformanceCounter = NULL;

/**
 * 获取版本
 */
USHORT MyGetVersion()
{
	USHORT OSBuildNumber = 0;

#ifndef _WIN64
	//fs:30的位置就是PEB
	DWORD peb = __readfsdword(0x30);
	OSBuildNumber = *(PUSHORT)(peb + 0xAC);
#else
	//gs:60的位置是PEB
	ULONG64 peb = __readgsqword(0x60);
	OSBuildNumber = *(PUSHORT)(peb + 0x120);
#endif

	return OSBuildNumber;
}

/**
 * 通信初始化
 */
BOOLEAN DriverCommInit()
{
	if (g_NtQueryInformationFile || g_NtConvertBetweenAuxiliaryCounterAndPerformanceCounter)
	{
		return TRUE;
	}

	USHORT buildNumber = MyGetVersion();

	HMODULE h = GetModuleHandle(TEXT("ntdll.dll"));

	if (buildNumber == 7600 || buildNumber == 7601)
	{
		g_NtQueryInformationFile = (NtQueryInformationFileProc)GetProcAddress(h, "NtQueryInformationFile");

		TCHAR tmpPath[MAX_PATH] = { 0 };
		GetTempPath(MAX_PATH, tmpPath);

		_tcscat_s(tmpPath, MAX_PATH, TEXT("\\1.txt"));

		g_file = CreateFile(tmpPath, FILE_ALL_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (g_file == NULL || g_file == INVALID_HANDLE_VALUE)
		{
			MessageBox(NULL, TEXT("通信初始化失败"), TEXT("错误"), 0);
			return FALSE;
		}
	}
	else
	{
		g_NtConvertBetweenAuxiliaryCounterAndPerformanceCounter = (NtConvertBetweenAuxiliaryCounterAndPerformanceCounterProc)GetProcAddress(h, "NtConvertBetweenAuxiliaryCounterAndPerformanceCounter");
	}

	return TRUE;
}

/*
	//win7通信
	NtQueryInformationFileProc NtQueryInformationFile = (NtQueryInformationFileProc)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationFile");
	HANDLE handle = CreateFile("C:\\1.txt", FILE_ALL_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	printf("%p \r\n", handle);
	char buf[0xE0] = { 0 };

	PCommPackage package = (PCommPackage)buf;
	package->id = 0x12345678;
	package->cmd = 1;

	IO_STATUS_BLOCK io = { 0 };
	system("pause");
	NtQueryInformationFile(handle, &io, buf, sizeof(buf), 0x34);
	CloseHandle(handle);
	system("pause");

	//win10通信
	NtConvertBetweenAuxiliaryCounterAndPerformanceCounterProc func = (NtConvertBetweenAuxiliaryCounterAndPerformanceCounterProc)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtConvertBetweenAuxiliaryCounterAndPerformanceCounter");
	char buf[0xE0] = { 0 };
	PCommPackage package = (PCommPackage)buf;
	package->id = 0x12345678;
	package->cmd = 1;
	ULONG64 xx = 0;
	//注意，第2个参数是二级指针
	func(1, (PVOID)&package, (PVOID)&xx, NULL);
*/

BOOLEAN DriverCommWin7(ULONG cmd, PVOID inData, SIZE_T inSize)
{
	char buf[0xE0] = { 0 };

	PCommPackage package = (PCommPackage)buf;
	package->id = 0x12345678;
	package->cmd = cmd;
	package->inSize = inSize;
	package->inData = (ULONG64)inData;
	package->retStatus = -1;

	IO_STATUS_BLOCK io = { 0 };

	g_NtQueryInformationFile(g_file, &io, buf, sizeof(buf), 0x34);

	return package->retStatus == 0;
}

BOOLEAN DriverCommWin10(ULONG cmd, PVOID inData, SIZE_T inSize)
{
	CommPackage package = { 0 };
	package.id = 0x12345678;
	package.cmd = cmd;
	package.inData = (ULONG64)inData;
	package.inSize = inSize;
	package.retStatus = -1;

	ULONG64 xxx = 0;
	PCommPackage data = &package;

	//注意，第2个参数是二级指针
	g_NtConvertBetweenAuxiliaryCounterAndPerformanceCounter(1, (PVOID)&data, (PVOID)&xxx, NULL);

	return package.retStatus == 0;
}

/**
 * 通信
 */
BOOLEAN DriverComm(ULONG cmd, PVOID inData, SIZE_T inSize)
{
	if (DriverCommInit())
	{
		USHORT buildNumber = MyGetVersion();

		if (buildNumber == 7600 || buildNumber == 7601)
		{
			return DriverCommWin7(cmd, inData, inSize);
		}
		else
		{
			return DriverCommWin10(cmd, inData, inSize);
		}
	}

	return FALSE;
}