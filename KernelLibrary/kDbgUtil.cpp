#include "pch.h"
#include "kDbgUtil.h"
#include "detours.h"

extern POBJECT_TYPE* DbgkDebugObjectType;

PEX_RUNDOWN_REF kDbgUtil::GetProcessRundownProtect(PEPROCESS Process) {
	return reinterpret_cast<PEX_RUNDOWN_REF>((char*)Process + _eprocessOffsets.RundownProtect);
}

ULONG kDbgUtil::GetProcessCrossThreadFlags(PEPROCESS Process) {
	return *reinterpret_cast<PULONG>((char*)Process + _eprocessOffsets.CrossThreadFlags);
}

PPEB kDbgUtil::GetProcessPeb(PEPROCESS Process) {
	return *reinterpret_cast<PPEB*>((char*)Process + _eprocessOffsets.Peb);
}

PDEBUG_OBJECT kDbgUtil::GetProcessDebugPort(PEPROCESS Process) {
	return *reinterpret_cast<PDEBUG_OBJECT*>((char*)Process + _eprocessOffsets.DebugPort);
}

VOID* kDbgUtil::GetProcessWow64Process(PEPROCESS Process) {
	return *reinterpret_cast<VOID**>((char*)Process + _eprocessOffsets.Wow64Process);
}

ULONG kDbgUtil::GetProcessFlags(PEPROCESS Process) {
	return *reinterpret_cast<PULONG>((char*)Process + _eprocessOffsets.Flags);
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
		_first = false;
	}
	
	return HookDbgSys();
}

bool kDbgUtil::ExitDbgSys() {
	return UnhookDbgSys();
}