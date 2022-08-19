#include "pch.h"
#include "kDbgUtil.h"
#include "detours.h"

extern POBJECT_TYPE* DbgkDebugObjectType;

PEX_RUNDOWN_REF kDbgUtil::GetProcessRundownProtect(PEPROCESS Process) {
	return reinterpret_cast<PEX_RUNDOWN_REF>((char*)Process + _eprocessOffsets.RundownProtect);
}

bool kDbgUtil::HookDbgSys() {
	bool success = false;
	g_pNtCreateDebugObject = (PNtCreateDebugObject)_info.NtCreateDebugObjectAddress;
	if (g_pNtCreateDebugObject) {
		NTSTATUS status = DetourTransactionBegin();
		if (!NT_SUCCESS(status))
			return false;
		status = DetourUpdateThread(ZwCurrentThread());
		if (!NT_SUCCESS(status))
			return false;
		status = DetourAttach((PVOID*)&g_pNtCreateDebugObject, NtCreateDebugObject);
		if (!NT_SUCCESS(status))
			return false;
	}
	// success = _hookNtCreateDebugObject.HookKernelApi(g_pNtCreateDebugObject, NtCreateDebugObject, true);
	return success;
}

bool kDbgUtil::UnhookDbgSys() {
	bool success = false;
	NTSTATUS status = DetourTransactionBegin();
	if (!NT_SUCCESS(status))
		return false;
	status = DetourUpdateThread(ZwCurrentThread());
	if (!NT_SUCCESS(status))
		return false;

	status = DetourDetach((PVOID*)&g_pNtCreateDebugObject, NtCreateDebugObject);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	return success;
}

bool kDbgUtil::InitDbgSys(DbgSysCoreInfo* info) {
	if (_first) {
		_info.NtCreateDebugObjectAddress = info->NtCreateDebugObjectAddress;
		_info.DbgkDebugObjectTypeAddress = info->DbgkDebugObjectTypeAddress;
		_info.ZwProtectVirtualMemory = info->ZwProtectVirtualMemory;
		pZwProtectVirtualMemory = (PZwProtectVirtualMemory)_info.ZwProtectVirtualMemory;
		_first = false;
	}
	DbgkDebugObjectType = (POBJECT_TYPE*)_info.DbgkDebugObjectTypeAddress;
	return HookDbgSys();
}

bool kDbgUtil::ExitDbgSys() {
	return UnhookDbgSys();
}