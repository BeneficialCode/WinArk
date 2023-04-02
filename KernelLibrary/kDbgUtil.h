#pragma once
#include "Common.h"
#include "khook.h"
#include "kDbgSys.h"
#include "Teb.h"




class kDbgUtil final {
public:
	static PEX_RUNDOWN_REF GetProcessRundownProtect(PEPROCESS Process);
	static PULONG GetProcessCrossThreadFlags(PEPROCESS Process);
	static PPEB GetProcessPeb(PEPROCESS Process);
	static PDEBUG_OBJECT* GetProcessDebugPort(PEPROCESS Process);
	static PWOW64_PROCESS GetProcessWow64Process(PEPROCESS Process);
	static PULONG GetProcessFlags(PEPROCESS Process);
	static VOID* GetProcessSectionBaseAddress(PEPROCESS Process);
	static VOID* GetProcessSectionObject(PEPROCESS Process);
	static VOID* GetProcessUniqueProcessId(PEPROCESS Process);
	static VOID* GetThreadStartAddress(PETHREAD Thread);
	static VOID* GetThreadWin32StartAddress(PETHREAD Thread);
	static PULONG GetThreadCrossThreadFlags(PETHREAD Ethread);
	static PEX_RUNDOWN_REF GetThreadRundownProtect(PETHREAD Thread);
	static PPEB_LDR_DATA GetPEBLdr(PPEB Peb);
	static CLIENT_ID GetThreadCid(PETHREAD Thread);
	static PKAPC_STATE GetThreadApcState(PETHREAD Thread);
	static UINT8 GetCurrentThreadApcStateIndex();
	static PBOOLEAN GetPEBBeingDebugged(PPEB Peb);
	static PLARGE_INTEGER GetProcessExitTime(PEPROCESS Process);
	static ULONG64 GetThreadClonedThread(PETHREAD Thread);

	// 初始化调试函数指针
	static bool InitDbgSys(DbgSysCoreInfo* info);
	static bool ExitDbgSys();
	static bool HookDbgSys();
	static bool UnhookDbgSys();

	static inline TcbOffsets _tcbOffsets;
	static inline EThreadOffsets _ethreadOffsets;
	static inline EProcessOffsets _eprocessOffsets;
	static inline KThreadOffsets _kthreadOffsets;
	static inline PebOffsets _pebOffsets;

	using PNtCreateDebugObject = decltype(&NewNtCreateDebugObject);
	static inline PNtCreateDebugObject g_pNtCreateDebugObject{ nullptr };
	static inline khook _hookNtCreateDebugObject;

	using PNtDebugActiveProcess = decltype(&NewNtDebugActiveProcess);
	static inline PNtDebugActiveProcess g_pNtDebugActiveProcess{ nullptr };

	using PDbgkpPostFakeProcessCreateMessages = decltype(&DbgkpPostFakeProcessCreateMessages);
	static inline PDbgkpPostFakeProcessCreateMessages g_pDbgkpPostFakeProcessCreateMessages{ nullptr };

	using PDbgkpSetProcessDebugObject = decltype(&DbgkpSetProcessDebugObject);
	static inline PDbgkpSetProcessDebugObject g_pDbgkpSetProcessDebugObject{ nullptr };

	using PDbgkpPostFakeThreadMessages = decltype(&DbgkpPostFakeThreadMessages);
	static inline PDbgkpPostFakeThreadMessages g_pDbgkpPostFakeThreadMessages{ nullptr };

	using PDbgkpPostModuleMessages = decltype(&DbgkpPostModuleMessages);
	static inline PDbgkpPostModuleMessages g_pDbgkpPostModuleMessages{ nullptr };


	using PPsGetNextProcessThread = PETHREAD(NTAPI*)(PEPROCESS Process, PETHREAD Thread);
	static inline PPsGetNextProcessThread g_pPsGetNextProcessThread{ nullptr };

	using PDbgkpWakeTarget = VOID(NTAPI*)(
		_In_ PDEBUG_EVENT DebugEvent
		);

	using PDbgkpConvertKernelToUserStateChange = VOID(NTAPI*)(
		PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
		PDEBUG_EVENT DebugEvent
		);

	using PPsCaptureExceptionPort = PVOID(NTAPI*)(
		_In_ PEPROCESS Process
		);

	using PDbgkpSendErrorMessage = NTSTATUS(NTAPI*)(
		_In_ PEXCEPTION_RECORD ExceptionRecord,
		_In_ BOOLEAN IsFilterMessage,
		_In_ PDBGKM_APIMSG DbgApiMsg
		);

	using PDbgkpSendApiMessageLpc = NTSTATUS(NTAPI*)(
		_Inout_ PDBGKM_APIMSG ApiMsg,
		_In_ PVOID Port,
		_In_ BOOLEAN SuspendProcess
		);

	using PDbgkpOpenHandles = VOID(NTAPI*)(
		_In_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
		_In_ PEPROCESS Process,
		_In_ PETHREAD Thread
		);

	using PDbgkpSuppressDbgMsg = BOOLEAN(NTAPI*)(
		_In_ PTEB Teb
		);



	static inline PDbgkpWakeTarget g_pDbgkpWakeTarget{ nullptr };
	using PDbgkpMarkProcessPeb = decltype(&DbgkpMarkProcessPeb);
	static inline PDbgkpMarkProcessPeb g_pDbgkpMarkProcessPeb{ nullptr };

	using PMmGetFileNameForAddress = NTSTATUS(NTAPI*)(PVOID ProcessVa, PUNICODE_STRING FileName);
	static inline PMmGetFileNameForAddress g_pMmGetFileNameForAddress{ nullptr };

	using PDbgkpQueueMessage = decltype(&DbgkpQueueMessage);
	static inline PDbgkpQueueMessage g_pDbgkpQueueMessage{ nullptr };

	using PDbgkpSendApiMessage = decltype(&DbgkpSendApiMessage);
	static inline PDbgkpSendApiMessage g_pDbgkpSendApiMessage{ nullptr };

	static inline PDbgkpSuspendProcess g_pDbgkpSuspendProcess{ nullptr };

	static inline PKeThawAllThreads g_pKeThawAllThreads{ nullptr };

	static inline PDbgkpSectionToFileHandle g_pDbgkpSectionToFileHandle{ nullptr };

	static inline PPsResumeThread g_pPsResumeThread{ nullptr };

	using PDbgkSendSystemDllMessages = decltype(&DbgkSendSystemDllMessages);
	static inline PDbgkSendSystemDllMessages g_pDbgkSendSystemDllMessages{ nullptr };


	static inline PPsSuspendThread g_pPsSuspendThread{ nullptr };

	static inline PPsQuerySystemDllInfo g_pPsQuerySystemDllInfo{ nullptr };


	static inline PObFastReferenceObject g_pObFastReferenceObject{ nullptr };

	static inline PExfAcquirePushLockShared g_pExfAcquirePushLockShared{ nullptr };

	static inline PExfReleasePushLockShared g_pExfReleasePushLockShared{ nullptr };

	static inline PObFastReferenceObjectLocked g_pObFastReferenceObjectLocked{ nullptr };

	static inline PDbgkpConvertKernelToUserStateChange g_pDbgkpConvertKernelToUserStateChange{ nullptr };

	static inline PPsGetNextProcess g_pPsGetNextProcess{ nullptr };

	static inline PPsCaptureExceptionPort g_pPsCaptureExceptionPort{ nullptr };

	static inline PDbgkpSendErrorMessage g_pDbgkpSendErrorMessage{ nullptr };

	static inline PDbgkpSendApiMessageLpc g_pDbgkpSendApiMessageLpc{ nullptr };

	static inline PDbgkpOpenHandles g_pDbgkpOpenHandles{ nullptr };

	static inline PDbgkpSuppressDbgMsg g_pDbgkpSuppressDbgMsg{ nullptr };

	static inline PObFastDereferenceObject g_pObFastDereferenceObject{ nullptr };

	using PDbgkCreateThread = decltype(&NewDbgkCreateThread);
	static inline PDbgkCreateThread g_pDbgkCreateThread{ nullptr };

	using PDbgkExitThread = decltype(&NewDbgkExitThread);
	static inline PDbgkExitThread g_pDbgkExitThread{ nullptr };

	using PDbgkExitProcess = decltype(&NewDbgkExitProcess);
	static inline PDbgkExitProcess g_pDbgkExitProcess{ nullptr };

	using PDbgkMapViewOfSection = decltype(&NewDbgkMapViewOfSection);
	static inline PDbgkMapViewOfSection g_pDbgkMapViewOfSection{ nullptr };

	using PDbgkUnMapViewOfSection = decltype(&NewDbgkUnMapViewOfSection);
	static inline PDbgkUnMapViewOfSection g_pDbgkUnMapViewOfSection{ nullptr };

	using PNtWaitForDebugEvent = decltype(&NewNtWaitForDebugEvent);
	static inline PNtWaitForDebugEvent g_pNtWaitForDebugEvent{ nullptr };

	using PNtDebugContinue = decltype(&NewNtDebugContinue);
	static inline PNtDebugContinue g_pNtDebugContinue{ nullptr };

	using PNtRemoveProcessDebug = decltype(&NewNtRemoveProcessDebug);
	static inline PNtRemoveProcessDebug g_pNtRemoveProcessDebug{ nullptr };

	using PDbgkForwardException = decltype(&NewDbgkForwardException);
	static inline PDbgkForwardException g_pDbgkForwardException{ nullptr };

	using PDbgkCopyProcessDebugPort = decltype(&NewDbgkCopyProcessDebugPort);
	static inline PDbgkCopyProcessDebugPort g_pDbgkCopyProcessDebugPort{ nullptr };

	using PDbgkClearProcessDebugObject = decltype(&NewDbgkClearProcessDebugObject);
	static inline PDbgkClearProcessDebugObject g_pDbgkClearProcessDebugObject{ nullptr };

	using PNtSetInformationDebugObject = decltype(&NewNtSetInformationDebugObject);
	static inline PNtSetInformationDebugObject g_pNtSetInformationDebugObject{ nullptr };

	using PPsTerminateProcess = NTSTATUS (NTAPI*)(
		PEPROCESS Process,
		NTSTATUS Status
	);
	static inline PPsTerminateProcess g_pPsTerminateProcess{ nullptr };

	using PMiSectionControlArea = PVOID(NTAPI*)(PVOID Section);
	static inline PMiSectionControlArea g_pMiSectionControlArea{ nullptr };

	using PMiReferenceControlAreaFile = PFILE_OBJECT (NTAPI*)(PVOID SectionControlArea);
	static inline PMiReferenceControlAreaFile g_pMiReferenceControlAreaFile{ nullptr };

	static inline bool _first = true;
};