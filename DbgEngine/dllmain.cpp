// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#pragma comment(lib,"ntdllp.lib")



DWORD WINAPI MainThread(void* params);


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
    {
        MH_Uninitialize();
        break;
    }
    }
    return TRUE;
}


DWORD WINAPI MainThread(void* params) {
    OutputDebugString(L"DbgEngine initialize...\n");
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        OutputDebugString(L"Minhook init failed");
        return -1;
    }
    return 0;
}
