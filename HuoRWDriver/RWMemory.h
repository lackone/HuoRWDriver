#pragma once
#include <ntifs.h>

//直接附加 memcpy
NTSTATUS ReadMemory1(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// MmCopyVirtualMemory
NTSTATUS ReadMemory2(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// MDL
NTSTATUS ReadMemory3(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// 切CR3
NTSTATUS ReadMemory4(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// 写内存
NTSTATUS WriteMemory(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);