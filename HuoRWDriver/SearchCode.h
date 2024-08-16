#pragma once
#include <ntifs.h>

typedef struct _FindCode
{
	UCHAR code[0x200];
	ULONG len;
	int offset;
	ULONG lastAddressOffset;
} FindCode, * PFindCode;

VOID initFindCodeStruct(PFindCode findCode, PCHAR code, ULONG_PTR offset, ULONG_PTR lastAddrOffset);

ULONG_PTR findAddressByCode(ULONG_PTR beginAddr, ULONG_PTR endAddr, PFindCode findCode, ULONG numbers);

ULONG_PTR QuerySysModule(PCHAR moduleName, ULONG_PTR* moduleBase);

ULONG_PTR searchNtCode(PCHAR code, INT offset);

ULONG_PTR searchCode(PCHAR moduleName, PCHAR segmentName, PCHAR code, INT offset);