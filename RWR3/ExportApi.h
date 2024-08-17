#pragma once
#include <windows.h>

EXTERN_C BOOLEAN WINAPI HRW_DriverLoad();

EXTERN_C VOID WINAPI HRW_UnDriverLoad();

EXTERN_C ULONG64 WINAPI HRW_GetModule(DWORD pid, CHAR* moduleName);