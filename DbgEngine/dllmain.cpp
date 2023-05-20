// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "DebugCore.h"
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

    if (MH_CreateHookApi(L"ntdll", "NtWaitForDebugEvent", &DetourNtWaitForDebugEvent,
        reinterpret_cast<void**>(&g_pNtWaitForDebugEvent))) {
        OutputDebugString(L"Create NtWaitForDebugEvent hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"ntdll", "NtCreateDebugObject", &DetourNtCreateDebugObject,
        reinterpret_cast<void**>(&g_pNtCreateDebugObject))) {
        OutputDebugString(L"Create NtCreateDebugObject hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"ntdll", "DbgUiWaitStateChange", &DetourDbgUiWaitStateChange,
        reinterpret_cast<void**>(&g_pDbgUiWaitStateChange))) {
        OutputDebugString(L"Create DbgUiWaitStateChange hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"kernel32", "DebugSetProcessKillOnExit", &DetourDebugSetProcessKillOnExit,
        reinterpret_cast<void**>(&g_pDebugSetProcessKillOnExit))) {
        OutputDebugString(L"Create DebugSetProcessKillOnExit hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"ntdll", "DbgUiContinue", &DetourDbgUiContinue,
        reinterpret_cast<void**>(&g_pDbgUiContinue))) {
        OutputDebugString(L"Create DbgUiContinue hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"ntdll", "DbgUiIssueRemoteBreakin", &DetourDbgUiIssueRemoteBreakin,
        reinterpret_cast<void**>(&g_pDbgUiIssueRemoteBreakin))) {
        OutputDebugString(L"Create DbgUiIssueRemoteBreakin hook failed!");
        return -1;
    }

    if (MH_CreateHookApi(L"ntdll", "NtRemoveProcessDebug", &DetourNtRemoveProcessDebug,
        reinterpret_cast<void**>(&g_pNtRemoveProcessDebug))) {
        OutputDebugString(L"Create NtRemoveProcessDebug hook failed!");
        return -1;
    }

    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}
