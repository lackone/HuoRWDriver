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

        ULONG64 size = HRW_GetModule(2072, (CHAR*)"explorer.exe");
        printf("size = %x \r\n", size);
        
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

