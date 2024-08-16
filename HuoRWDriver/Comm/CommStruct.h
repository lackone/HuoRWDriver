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

typedef ULONG (NTAPI* CommCallbackProc)(PCommPackage package);