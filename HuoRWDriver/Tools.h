#pragma once
#include <ntifs.h>

/**
 * ��ӡ��־
 */
#define Log(Format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[HuoRWDriver] " Format "\n", ##__VA_ARGS__)