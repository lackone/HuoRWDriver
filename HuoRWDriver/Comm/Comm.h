#pragma once
#include <ntifs.h>
#include "CommStruct.h"

/**
 * ע��ص�
 */
NTSTATUS RegCommCallback(CommCallbackProc callback);

/**
 * ж�ػص�
 */
VOID UnRegCommCallback();