// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "CommR3.h"
#include "ExportApi.h"

int main()
{
    BOOLEAN ret = HRW_DriverLoad();
    if (ret)
    {
        printf("驱动加载成功\r\n");

        ULONG64 module = HRW_GetModule(2072, (CHAR*)"explorer.exe");

        MMEMORY_BASIC_INFORMATION info = { 0 };

        HRW_QueryMemory(2072, module, &info);

        printf("AllocationBase %llx \r\n", info.AllocationBase);
        printf("AllocationProtect %llx \r\n", info.AllocationProtect);
        printf("BaseAddress %llx \r\n", info.BaseAddress);
        printf("Protect %llx \r\n", info.Protect);
        printf("RegionSize %llx \r\n", info.RegionSize);
        printf("State %llx \r\n", info.State);
        printf("Type %llx \r\n", info.Type);
        
        system("pause");
        HRW_UnDriverLoad();
    }
    else
    {
        printf("驱动加载失败\r\n");
    }

    system("pause");
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

