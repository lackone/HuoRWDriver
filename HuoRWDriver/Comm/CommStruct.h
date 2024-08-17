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
} CMD;

typedef struct _ModuleInfo
{
	ULONG64 pid;
	ULONG64 moduleName;
	ULONG64 moduleBase;
	ULONG64 moduleSize;
} ModuleInfo, *PModuleInfo;

typedef struct _ReadWriteInfo
{
	ULONG64 pid;
	ULONG64 baseAddr;
	ULONG64 buf;
	ULONG64 size;

} ReadWriteInfo, *PReadWriteInfo;

typedef ULONG (NTAPI* CommCallbackProc)(PCommPackage package);