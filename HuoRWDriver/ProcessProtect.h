#pragma once
#include <ntifs.h>

NTSTATUS SetProtectPid(HANDLE pid);

VOID DestoryObRegister();

NTSTATUS InitObRegister();