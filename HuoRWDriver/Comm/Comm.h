#pragma once
#include <ntifs.h>
#include "CommStruct.h"

/**
 * 注册回调
 */
NTSTATUS RegCommCallback(CommCallbackProc callback);

/**
 * 卸载回调
 */
VOID UnRegCommCallback();