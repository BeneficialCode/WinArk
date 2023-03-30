#include "pch.h"
#include "kDbgUtil.h"
#include "detours.h"

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