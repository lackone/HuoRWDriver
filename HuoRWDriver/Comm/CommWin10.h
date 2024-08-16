#pragma once
#include <ntifs.h>
#include "CommStruct.h"

NTSTATUS RegCommCallbackWin10(CommCallbackProc callback);

VOID UnRegCommCallbackWin10();