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
	CMD_TEST, //防止驱动隐藏之后，重复加载

	CMD_GET_MODULE, // 获取模块

	CMD_READ_MEMORY, // 读内存

	CMD_WRITE_MEMORY, //写内存

	CMD_QUERY_MEMORY, //查询内存

	CMD_PROCESS_PROTECT, //保护进程

	CMD_REMOTE_CALL, //远程CALL

	CMD_PROCESS_FAKE, //进程伪装
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

typedef struct _ProcessProtectInfo
{
	ULONG64 pid;
} ProcessProtectInfo, * PProcessProtectInfo;

typedef struct _RemoteCallInfo
{
	ULONG64 pid;
	ULONG64 shellCode;
	ULONG64 shellCodeSize;

} RemoteCallInfo, * PRemoteCallInfo;

typedef struct _ProcessFakeInfo
{
	ULONG64 fakePid;
	ULONG64 srcPid;
} ProcessFakeInfo, * PProcessFakeInfo;

typedef ULONG(NTAPI* CommCallbackProc)(PCommPackage package);