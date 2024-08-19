#pragma once
#include <ntifs.h>

NTSTATUS RemoteCall(HANDLE pid, PVOID shellCode, SIZE_T shellCodeSize);

