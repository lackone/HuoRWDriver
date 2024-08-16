#pragma once
#include <ntifs.h>

ULONG_PTR GetModuleR3(HANDLE pid, PCHAR moduleName, ULONG_PTR* sizeOfImage);