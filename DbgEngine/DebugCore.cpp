#include "pch.h"
#include "DebugCore.h"

PNtWaitForDebugEvent g_pNtWaitForDebugEvent = nullptr;
PNtCreateDebugObject g_pNtCreateDebugObject = nullptr;
PDbgUiWaitStateChange g_pDbgUiWaitStateChange = nullptr;
PDebugSetProcessKillOnExit g_pDebugSetProcessKillOnExit = nullptr;
PDetourDbgUiContinue g_pDbgUiContinue = nullptr;
PDbgUiIssueRemoteBreakin g_pDbgUiIssueRemoteBreakin = nullptr;
PNtRemoveProcessDebug g_pNtRemoveProcessDebug = nullptr;

NTSTATUS
NTAPI
DetourNtWaitForDebugEvent(
    _In_ HANDLE DebugObjectHandle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
) {
    OutputDebugString(L"NtWaitForDebugEvent hooked!");
    return g_pNtWaitForDebugEvent(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
}

NTSTATUS
NTAPI
DetourNtCreateDebugObject(
    _In_ HANDLE DebugObjectHandle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
) {
    OutputDebugString(L"NtCreateDebugObject hooked!");
    return g_pNtCreateDebugObject(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
}

NTSTATUS
NTAPI
DetourDbgUiWaitStateChange(
    _Out_ PDBGUI_WAIT_STATE_CHANGE StateChange,
    _In_opt_ PLARGE_INTEGER Timeout
) {
    OutputDebugString(L"DbgUiWaitStateChange hooked!");
    return g_pDbgUiWaitStateChange(StateChange, Timeout);
}

BOOL
WINAPI
DetourDebugSetProcessKillOnExit(
    _In_ BOOL KillOnExit
) {
    OutputDebugString(L"DebugSetProcessKillOnExit hooked!");
    return g_pDebugSetProcessKillOnExit(KillOnExit);
}

NTSTATUS
NTAPI
DetourDbgUiContinue(
    _In_ PCLIENT_ID AppClientId,
    _In_ NTSTATUS ContinueStatus
) {
    OutputDebugString(L"DbgUiContinue hooked!");
    return g_pDbgUiContinue(AppClientId,ContinueStatus);
}

NTSTATUS
NTAPI
DetourDbgUiIssueRemoteBreakin(
    _In_ HANDLE Process
) {
    OutputDebugString(L"DbgUiIssueRemoteBreakin hooked!");
    return g_pDbgUiIssueRemoteBreakin(Process);
}

NTSTATUS
NTAPI
DetourNtRemoveProcessDebug(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE DebugObjectHandle
) {
    OutputDebugString(L"NtRemoveProcessDebug hooked!");
    return g_pNtRemoveProcessDebug(ProcessHandle,DebugObjectHandle);
}