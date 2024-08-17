#pragma once
#include <ntifs.h>

//Ö±½Ó¸½¼Ó memcpy
NTSTATUS ReadMemory1(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// MmCopyVirtualMemory
NTSTATUS ReadMemory2(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// MDL
NTSTATUS ReadMemory3(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);

// ÇÐCR3
NTSTATUS ReadMemory4(HANDLE pid, PVOID targetAddr, PVOID buf, SIZE_T size);