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

        /*
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

        HRW_ProcessProtect(1988);
        
        HMODULE hm = LoadLibraryA("user32.dll");
        ULONG_PTR msg = (ULONG_PTR)GetProcAddress(hm, "MessageBoxA");

        char code[] = {
            0x31, 0xC9,
            0x31, 0xD2,
            0x4D, 0x31, 0xC0,
            0x4D, 0x31, 0xC9,
            0x48, 0xB8, 0x99, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00,
            0x48, 0x81, 0xEC, 0xA8, 0x00, 0x00, 0x00,
            0xFF, 0xD0,
            0x48, 0x81, 0xC4, 0xA8, 0x00, 0x00, 0x00,
            0xC3
        };

		*(PULONG64)&code[12] = msg;

		HRW_RemoteCall(944, code, sizeof(code));
        */

        HRW_ProcessFake(944, 416);
        
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

