#pragma once

#ifndef _WINDOWS_
#include <ntifs.h>
#else
#include <windows.h>
#endif


typedef struct _CommPackage
{
	ULONG64 id;
	ULONG64 cmd;
	ULONG64 inData;
	ULONG64 inSize;
	ULONG64 retStatus;
} CommPackage, * PCommPackage;

typedef enum _CMD
{
	CMD_TEST, //��ֹ��������֮���ظ�����

	CMD_GET_MODULE, // ��ȡģ��

	CMD_READ_MEMORY, // ���ڴ�

	CMD_WRITE_MEMORY, //д�ڴ�

	CMD_QUERY_MEMORY, //��ѯ�ڴ�
} CMD;

typedef struct _ModuleInfo
{
	ULONG64 pid;
	ULONG64 moduleName;
	ULONG64 moduleBase;
	ULONG64 moduleSize;
} ModuleInfo, * PModuleInfo;

typedef struct _ReadWriteInfo
{
	ULONG64 pid;
	ULONG64 baseAddr;
	ULONG64 buf;
	ULONG64 size;

} ReadWriteInfo, * PReadWriteInfo;

typedef struct _MyMEMORY_BASIC_INFORMATION {
	ULONG64 BaseAddress;
	ULONG64 AllocationBase;
	ULONG64 AllocationProtect;
	ULONG64 RegionSize;
	ULONG64 State;
	ULONG64 Protect;
	ULONG64 Type;
} MyMEMORY_BASIC_INFORMATION, * PMyMEMORY_BASIC_INFORMATION;

typedef struct _QueryMemoryInfo
{
	ULONG64 pid;
	ULONG64 baseAddr;
	MyMEMORY_BASIC_INFORMATION basicInfo;
} QueryMemoryInfo, * PQueryMemoryInfo;

typedef ULONG(NTAPI* CommCallbackProc)(PCommPackage package);