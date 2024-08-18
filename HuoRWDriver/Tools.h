#pragma once
#include <ntifs.h>

/**
 * 打印日志
 */
#define Log(Format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[HuoRWDriver] " Format "\n", ##__VA_ARGS__)

/**
 * 修改内存属性
 */
NTSTATUS NTAPI MyProtectVirtualMemory(
	__in HANDLE ProcessHandle,
	__inout PVOID* BaseAddress,
	__inout PSIZE_T RegionSize,
	__in ULONG NewProtect,
	__out PULONG OldProtect
);

ULONG64 wpOff();

VOID wpOn(ULONG64 cr0);