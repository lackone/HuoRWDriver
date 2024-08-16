#pragma once
#include <ntifs.h>
#include "CommStruct.h"

typedef NTSTATUS(*FileCallback)(HANDLE fileHandle, PVOID info, PVOID p1, PVOID p2);

typedef struct _RegisterCallback
{
	FileCallback QueryFileCallback;
	FileCallback SetFileCallback;
} RegisterCallback, * PRegisterCallback;

typedef NTSTATUS(*ExRegisterAttributeInformationCallbackProc)(PRegisterCallback callbacks);

NTSTATUS RegCommCallbackWin7(CommCallbackProc callback);

VOID UnRegCommCallbackWin7();