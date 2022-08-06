#include "pch.h"
#include "kDbgUtil.h"

extern POBJECT_TYPE* DbgkDebugObjectType;

PEX_RUNDOWN_REF kDbgUtil::GetProcessRundownProtect(PEPROCESS Process) {
	return reinterpret_cast<PEX_RUNDOWN_REF>((char*)Process + _eprocessOffsets.RundownProtect);
}

bool kDbgUtil::HookDbgSys() {
	bool success = false;
	g_pNtCreateDebugObject = (PNtCreateDebugObject)_info.NtCreateDebugObjectAddress;
	if(g_pNtCreateDebugObject)
		success = _hookNtCreateDebugObject.HookKernelApi(g_pNtCreateDebugObject, NtCreateDebugObject, true);
	return success;
}

bool kDbgUtil::UnhookDbgSys() {
	bool success = false;
	if (_hookNtCreateDebugObject._success)
		success = _hookNtCreateDebugObject.UnhookKernelApi(true);

	return success;
}

bool kDbgUtil::InitDbgSys(DbgSysCoreInfo* info) {
	if (_first) {
		_info.NtCreateDebugObjectAddress = info->NtCreateDebugObjectAddress;
		_info.DbgkDebugObjectTypeAddress = info->DbgkDebugObjectTypeAddress;
		_first = false;
	}
	DbgkDebugObjectType = (POBJECT_TYPE*)_info.DbgkDebugObjectTypeAddress;
	return HookDbgSys();
}

bool kDbgUtil::ExitDbgSys() {
	return UnhookDbgSys();
}