#pragma once
#include <ntifs.h>
#include "Comm/CommStruct.h"

ULONG_PTR GetModuleR3(HANDLE pid, PCHAR moduleName, ULONG_PTR* sizeOfImage);

NTSTATUS QueryMemory(HANDLE pid, ULONG64 addr, PMyMEMORY_BASIC_INFORMATION info);