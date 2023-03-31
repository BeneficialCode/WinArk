#include "pch.h"
#include "kDbgUtil.h"
#include "detours.h"
#include "Helpers.h"

extern POBJECT_TYPE* DbgkDebugObjectType;
extern PFAST_MUTEX g_pDbgkpProcessDebugPortMutex;
extern PULONG g_pDbgkpMaxModuleMsgs;

PEX_RUNDOWN_REF kDbgUtil::GetProcessRundownProtect(PEPROCESS Process) {
	return reinterpret_cast<PEX_RUNDOWN_REF>((char*)Process + _eprocessOffsets.RundownProtect);
}

PULONG kDbgUtil::GetProcessCrossThreadFlags(PEPROCESS Process) {
	return reinterpret_cast<PULONG>((char*)Process + _eprocessOffsets.CrossThreadFlags);
}

PPEB kDbgUtil::GetProcessPeb(PEPROCESS Process) {
	return *reinterpret_cast<PPEB*>((char*)Process + _eprocessOffsets.Peb);
}

PDEBUG_OBJECT* kDbgUtil::GetProcessDebugPort(PEPROCESS Process) {
	return reinterpret_cast<PDEBUG_OBJECT*>((char*)Process + _eprocessOffsets.DebugPort);
}

PWOW64_PROCESS kDbgUtil::GetProcessWow64Process(PEPROCESS Process) {
	return *reinterpret_cast<PWOW64_PROCESS*>((char*)Process + _eprocessOffsets.Wow64Process);
}

PULONG kDbgUtil::GetProcessFlags(PEPROCESS Process) {
	return reinterpret_cast<PULONG>((char*)Process + _eprocessOffsets.Flags);
}

VOID* kDbgUtil::GetProcessSectionBaseAddress(PEPROCESS Process) {
	return *reinterpret_cast<VOID**>((char*)Process + _eprocessOffsets.SectionBaseAddress);
}

VOID* kDbgUtil::GetProcessSectionObject(PEPROCESS Process) {
	return *reinterpret_cast<VOID**>((char*)Process + _eprocessOffsets.SectionObject);
}

VOID* kDbgUtil::GetProcessUniqueProcessId(PEPROCESS Process) {
	return *reinterpret_cast<VOID**>((char*)Process + _eprocessOffsets.UniqueProcessId);
}

VOID* kDbgUtil::GetThreadStartAddress(PETHREAD Thread) {
	return *reinterpret_cast<VOID**>((char*)Thread + _ethreadOffsets.StartAddress);
}

VOID* kDbgUtil::GetThreadWin32StartAddress(PETHREAD Thread) {
	return *reinterpret_cast<VOID**>((char*)Thread + _ethreadOffsets.Win32StartAddress);
}

PULONG kDbgUtil::GetThreadCrossThreadFlags(PETHREAD Ethread) {
	return reinterpret_cast<PULONG>((char*)Ethread + _ethreadOffsets.CrossThreadFlags);
}

PEX_RUNDOWN_REF kDbgUtil::GetThreadRundownProtect(PETHREAD Thread) {
	return reinterpret_cast<PEX_RUNDOWN_REF>((char*)Thread + _ethreadOffsets.RundownProtect);
}

PPEB_LDR_DATA kDbgUtil::GetPEBLdr(PPEB Peb) {
	return *reinterpret_cast<PPEB_LDR_DATA*>((char*)Peb + _pebOffsets.Ldr);
}

PBOOLEAN kDbgUtil::GetPEBBeingDebugged(PPEB Peb) {
	return reinterpret_cast<PBOOLEAN>((char*)Peb + _pebOffsets.BeingDebugged);
}

PKAPC_STATE kDbgUtil::GetThreadApcState(PETHREAD Thread) {
	return *reinterpret_cast<PKAPC_STATE*>((char*)Thread + _ethreadOffsets.ApcState);
}

UINT8 kDbgUtil::GetCurrentThreadApcStateIndex() {
	PETHREAD Thread = KeGetCurrentThread();
	return *reinterpret_cast<UINT8*>((char*)Thread + _ethreadOffsets.ApcStateIndex);
}

CLIENT_ID kDbgUtil::GetThreadCid(PETHREAD Thread) {
	CLIENT_ID cid{};
	cid.UniqueProcess = PsGetThreadProcessId(Thread);
	cid.UniqueThread = PsGetThreadId(Thread);
	return cid;
}

PLARGE_INTEGER kDbgUtil::GetProcessExitTime(PEPROCESS Process) {
	return reinterpret_cast<PLARGE_INTEGER>((char*)Process + _eprocessOffsets.ExitTime);
}

ULONG64 kDbgUtil::GetThreadClonedThread(PETHREAD Thread) {
	PUCHAR readAddr = (PUCHAR)Thread + _ethreadOffsets.ClonedThread;
	return Helpers::ReadBitField(readAddr, &_ethreadOffsets.ClonedThreadBitField);
}

bool kDbgUtil::HookDbgSys() {
	if (g_pNtCreateDebugObject) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtCreateDebugObject, NtCreateDebugObject);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}
	if (g_pNtDebugActiveProcess) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtDebugActiveProcess, NtDebugActiveProcess);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}
	
	if (g_pDbgkCreateThread) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkCreateThread, DbgkCreateThread);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkExitThread) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkExitThread, DbgkExitThread);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkExitProcess) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkExitProcess, DbgkExitProcess);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkMapViewOfSection) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkMapViewOfSection, DbgkMapViewOfSection);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkUnMapViewOfSection) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkUnMapViewOfSection, DbgkUnMapViewOfSection);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtWaitForDebugEvent) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtWaitForDebugEvent, NtWaitForDebugEvent);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtDebugContinue) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtDebugContinue, NtDebugContinue);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtRemoveProcessDebug) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtRemoveProcessDebug, NtRemoveProcessDebug);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkForwardException) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkForwardException, DbgkForwardException);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkCopyProcessDebugPort) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkCopyProcessDebugPort, DbgkCopyProcessDebugPort);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkClearProcessDebugObject) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pDbgkClearProcessDebugObject, DbgkClearProcessDebugObject);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtSetInformationDebugObject) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtSetInformationDebugObject, NtSetInformationDebugObject);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}



	return true;
}

bool kDbgUtil::UnhookDbgSys() {
	NTSTATUS status = DetourDetach((PVOID*)&g_pNtCreateDebugObject, NtCreateDebugObject);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	status = DetourDetach((PVOID*)&g_pNtDebugActiveProcess, NtDebugActiveProcess);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	if (g_pDbgkCreateThread) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkCreateThread, DbgkCreateThread);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkExitThread) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkExitThread, DbgkExitThread);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkExitProcess) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkExitProcess, DbgkExitProcess);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkMapViewOfSection) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkMapViewOfSection, DbgkMapViewOfSection);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkUnMapViewOfSection) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkUnMapViewOfSection, DbgkUnMapViewOfSection);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtWaitForDebugEvent) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pNtWaitForDebugEvent, NtWaitForDebugEvent);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtDebugContinue) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pNtDebugContinue, NtDebugContinue);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtRemoveProcessDebug) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pNtRemoveProcessDebug, NtRemoveProcessDebug);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkForwardException) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkForwardException, DbgkForwardException);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkCopyProcessDebugPort) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkCopyProcessDebugPort, DbgkCopyProcessDebugPort);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pDbgkClearProcessDebugObject) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pDbgkClearProcessDebugObject, DbgkClearProcessDebugObject);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	if (g_pNtSetInformationDebugObject) {
		NTSTATUS status = DetourDetach((PVOID*)&g_pNtSetInformationDebugObject, NtSetInformationDebugObject);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}

	return true;
}

bool kDbgUtil::InitDbgSys(DbgSysCoreInfo* info) {
	if (_first) {
		g_pNtCreateDebugObject = (PNtCreateDebugObject)info->NtCreateDebugObjectAddress;
		g_pZwProtectVirtualMemory = (PZwProtectVirtualMemory)info->ZwProtectVirtualMemory;
		DbgkDebugObjectType = (POBJECT_TYPE*)info->DbgkDebugObjectTypeAddress;
		g_pNtDebugActiveProcess = (PNtDebugActiveProcess)info->NtDebugActiveProcess;
		g_pDbgkpPostFakeProcessCreateMessages = (PDbgkpPostFakeProcessCreateMessages)info->DbgkpPostFakeProcessCreateMessages;
		g_pDbgkpSetProcessDebugObject = (PDbgkpSetProcessDebugObject)info->DbgkpSetProcessDebugObject;
		_eprocessOffsets.RundownProtect = info->EprocessOffsets.RundownProtect;
		g_pDbgkpPostFakeThreadMessages = (PDbgkpPostFakeThreadMessages)info->DbgkpPostFakeThreadMessages;
		g_pDbgkpPostModuleMessages = (PDbgkpPostModuleMessages)info->DbgkpPostModuleMessages;
		_ethreadOffsets.CrossThreadFlags = info->EthreadOffsets.CrossThreadFlags;
		_ethreadOffsets.RundownProtect = info->EthreadOffsets.RundownProtect;
		g_pPsGetNextProcessThread = (PPsGetNextProcessThread)info->PsGetNextProcessThread;
		_eprocessOffsets.DebugPort = info->EprocessOffsets.DebugPort;
		g_pDbgkpProcessDebugPortMutex = (PFAST_MUTEX)info->DbgkpProcessDebugPortMutex;
		g_pDbgkpMarkProcessPeb = (PDbgkpMarkProcessPeb)info->DbgkpMarkProcessPeb;
		g_pDbgkpWakeTarget = (PDbgkpWakeTarget)info->DbgkpWakeTarget;
		g_pDbgkpMaxModuleMsgs = (PULONG)info->DbgkpMaxModuleMsgs;
		_pebOffsets.Ldr = info->PebOffsets.Ldr;
		g_pMmGetFileNameForAddress = (PMmGetFileNameForAddress)info->MmGetFileNameForAddress;
		g_pDbgkpQueueMessage = (PDbgkpQueueMessage)info->DbgkpQueueMessage;
		g_pDbgkpSendApiMessage = (PDbgkpSendApiMessage)info->DbgkpSendApiMessage;
		g_pKeThawAllThreads = (PKeThawAllThreads)info->KeThawAllThreads;
		g_pDbgkpSectionToFileHandle = (PDbgkpSectionToFileHandle)info->DbgkpSectionToFileHandle;
		g_pPsResumeThread = (PPsResumeThread)info->PsResumeThread;
		g_pDbgkSendSystemDllMessages = (PDbgkSendSystemDllMessages)info->DbgkSendSystemDllMessages;
		g_pPsSuspendThread = (PPsSuspendThread)info->PsSuspendThread;
		g_pPsQuerySystemDllInfo = (PPsQuerySystemDllInfo)info->PsQuerySystemDllInfo;
		g_pPsCallImageNotifyRoutines = (PPsCallImageNotifyRoutines)info->PsCallImageNotifyRoutines;
		g_pObFastReferenceObject = (PObFastReferenceObject)info->ObFastReferenceObject;
		g_pExfAcquirePushLockShared = (PExfAcquirePushLockShared)info->ExfAcquirePushLockShared;
		g_pExfReleasePushLockShared = (PExfReleasePushLockShared)info->ExfReleasePushLockShared;
		g_pObFastReferenceObjectLocked = (PObFastReferenceObjectLocked)info->ObFastReferenceObjectLocked;
		g_pPsGetNextProcess = (PPsGetNextProcess)info->PsGetNextProcess;
		g_pDbgkpConvertKernelToUserStateChange = (PDbgkpConvertKernelToUserStateChange)info->DbgkpConvertKernelToUserStateChange;
		g_pDbgkpSendErrorMessage = (PDbgkpSendErrorMessage)info->DbgkpSendErrorMessage;
		g_pPsCaptureExceptionPort = (PPsCaptureExceptionPort)info->PsCaptureExceptionPort;
		g_pDbgkpSendApiMessageLpc = (PDbgkpSendApiMessageLpc)info->DbgkpSendApiMessageLpc;
		g_pDbgkpOpenHandles = (PDbgkpOpenHandles)info->DbgkpOpenHandles;
		g_pDbgkpSuppressDbgMsg = (PDbgkpSuppressDbgMsg)info->DbgkpSuppressDbgMsg;
		g_pObFastDereferenceObject = (PObFastDereferenceObject)info->ObFastDereferenceObject;
		g_pDbgkCreateThread = (PDbgkCreateThread)info->DbgkCreateThread;
		g_pDbgkExitThread = (PDbgkExitThread)info->DbgkExitThread;
		g_pDbgkExitProcess = (PDbgkExitProcess)info->DbgkExitProcess;
		g_pDbgkMapViewOfSection = (PDbgkMapViewOfSection)info->DbgkMapViewOfSection;
		g_pDbgkUnMapViewOfSection = (PDbgkUnMapViewOfSection)info->DbgkUnMapViewOfSection;
		g_pNtWaitForDebugEvent = (PNtWaitForDebugEvent)info->NtWaitForDebugEvent;
		g_pNtDebugContinue = (PNtDebugContinue)info->NtDebugContinue;
		g_pNtRemoveProcessDebug = (PNtRemoveProcessDebug)info->NtRemoveProcessDebug;
		g_pDbgkForwardException = (PDbgkForwardException)info->DbgkForwardException;
		g_pDbgkCopyProcessDebugPort = (PDbgkCopyProcessDebugPort)info->DbgkCopyProcessDebugPort;
		g_pDbgkClearProcessDebugObject = (PDbgkClearProcessDebugObject)info->DbgkClearProcessDebugObject;
		g_pNtSetInformationDebugObject = (PNtSetInformationDebugObject)info->NtSetInformationDebugObject;
		g_pPsTerminateProcess = (PPsTerminateProcess)info->PsTerminateProcess;
#ifdef _WIN64
		_eprocessOffsets.Wow64Process = info->EprocessOffsets.Wow64Process;
#endif // _WIN64
		_eprocessOffsets.Peb = info->EprocessOffsets.Peb;

		_first = false;
	}
	
	return HookDbgSys();
}

bool kDbgUtil::ExitDbgSys() {
	return UnhookDbgSys();
}