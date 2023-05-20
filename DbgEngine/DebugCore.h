#pragma once

typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

typedef struct _DBGKM_EXCEPTION
{
	EXCEPTION_RECORD ExceptionRecord;
	ULONG FirstChance;
} DBGKM_EXCEPTION, * PDBGKM_EXCEPTION;

typedef struct _DBGKM_CREATE_THREAD
{
	ULONG SubSystemKey;
	PVOID StartAddress;
} DBGKM_CREATE_THREAD, * PDBGKM_CREATE_THREAD;

typedef struct _DBGKM_CREATE_PROCESS
{
	ULONG SubSystemKey;
	HANDLE FileHandle;
	PVOID BaseOfImage;
	ULONG DebugInfoFileOffset;
	ULONG DebugInfoSize;
	DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, * PDBGKM_CREATE_PROCESS;

typedef struct _DBGKM_EXIT_THREAD
{
	NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, * PDBGKM_EXIT_THREAD;

typedef struct _DBGKM_EXIT_PROCESS
{
	NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, * PDBGKM_EXIT_PROCESS;

typedef struct _DBGKM_LOAD_DLL
{
	HANDLE FileHandle;
	PVOID BaseOfDll;
	ULONG DebugInfoFileOffset;
	ULONG DebugInfoSize;
	PVOID NamePointer;
} DBGKM_LOAD_DLL, * PDBGKM_LOAD_DLL;

typedef struct _DBGKM_UNLOAD_DLL
{
	PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, * PDBGKM_UNLOAD_DLL;

typedef enum _DBG_STATE
{
	DbgIdle,
	DbgReplyPending,
	DbgCreateThreadStateChange,
	DbgCreateProcessStateChange,
	DbgExitThreadStateChange,
	DbgExitProcessStateChange,
	DbgExceptionStateChange,
	DbgBreakpointStateChange,
	DbgSingleStepStateChange,
	DbgLoadDllStateChange,
	DbgUnloadDllStateChange
} DBG_STATE, * PDBG_STATE;

typedef struct _DBGUI_CREATE_THREAD
{
	HANDLE HandleToThread;
	DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, * PDBGUI_CREATE_THREAD;

typedef struct _DBGUI_CREATE_PROCESS
{
	HANDLE HandleToProcess;
	HANDLE HandleToThread;
	DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, * PDBGUI_CREATE_PROCESS;

typedef struct _DBGUI_WAIT_STATE_CHANGE
{
	DBG_STATE NewState;
	CLIENT_ID AppClientId;
	union
	{
		DBGKM_EXCEPTION Exception;
		DBGUI_CREATE_THREAD CreateThread;
		DBGUI_CREATE_PROCESS CreateProcessInfo;
		DBGKM_EXIT_THREAD ExitThread;
		DBGKM_EXIT_PROCESS ExitProcess;
		DBGKM_LOAD_DLL LoadDll;
		DBGKM_UNLOAD_DLL UnloadDll;
	} StateInfo;
} DBGUI_WAIT_STATE_CHANGE, * PDBGUI_WAIT_STATE_CHANGE;


NTSTATUS
NTAPI
DetourNtWaitForDebugEvent(
	_In_ HANDLE DebugObjectHandle,
	_In_ BOOLEAN Alertable,
	_In_opt_ PLARGE_INTEGER Timeout,
	_Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
);

NTSTATUS
NTAPI
DetourNtCreateDebugObject(
	_In_ HANDLE DebugObjectHandle,
	_In_ BOOLEAN Alertable,
	_In_opt_ PLARGE_INTEGER Timeout,
	_Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
);

NTSTATUS
NTAPI
DetourDbgUiWaitStateChange(
	_Out_ PDBGUI_WAIT_STATE_CHANGE StateChange,
	_In_opt_ PLARGE_INTEGER Timeout
);

BOOL
WINAPI
DetourDebugSetProcessKillOnExit(
	_In_ BOOL KillOnExit
);

NTSTATUS
NTAPI
DetourDbgUiIssueRemoteBreakin(
	_In_ HANDLE Process
);

NTSTATUS
NTAPI
DetourDbgUiContinue(
	_In_ PCLIENT_ID AppClientId,
	_In_ NTSTATUS ContinueStatus
);

NTSTATUS
NTAPI
DetourNtRemoveProcessDebug(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
);



using PNtWaitForDebugEvent = decltype(&DetourNtWaitForDebugEvent);
extern PNtWaitForDebugEvent g_pNtWaitForDebugEvent;

using PNtCreateDebugObject = decltype(&DetourNtCreateDebugObject);
extern PNtCreateDebugObject g_pNtCreateDebugObject;

using PDbgUiWaitStateChange = decltype(&DetourDbgUiWaitStateChange);
extern PDbgUiWaitStateChange g_pDbgUiWaitStateChange;

using PDebugSetProcessKillOnExit = decltype(&DebugSetProcessKillOnExit);
extern PDebugSetProcessKillOnExit g_pDebugSetProcessKillOnExit;

using PDetourDbgUiContinue = decltype(&DetourDbgUiContinue);
extern PDetourDbgUiContinue g_pDbgUiContinue;

using PDbgUiIssueRemoteBreakin = decltype(&DetourDbgUiIssueRemoteBreakin);
extern PDbgUiIssueRemoteBreakin g_pDbgUiIssueRemoteBreakin;

using PNtRemoveProcessDebug = decltype(&DetourNtRemoveProcessDebug);
extern PNtRemoveProcessDebug g_pNtRemoveProcessDebug;