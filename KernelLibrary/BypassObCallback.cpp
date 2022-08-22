#include "pch.h"
#include "BypassObCallback.h"
#include "detours.h"

PObpCallPreOperationCallbacks g_pObpCallPreOperationCallbacks = nullptr;

extern "C" {
	NTKERNELAPI UCHAR* NTAPI PsGetProcessImageFileName(_In_ PEPROCESS Process);
}

NTSTATUS NTAPI
ObpCallPreOperationCallbacks(POBJECT_TYPE ObjectType,
	POB_PRE_OPERATION_INFORMATION Info,
	PLIST_ENTRY PostCallbackListHead) {
	if (!strstr((char*)PsGetProcessImageFileName(PsGetCurrentProcess()), "WinArk")) {
		return STATUS_SUCCESS;
	}
	return g_pObpCallPreOperationCallbacks(ObjectType, Info, PostCallbackListHead);
}

bool BypassObjectCallback::Bypass(void* address) {
	g_pObpCallPreOperationCallbacks = (PObpCallPreOperationCallbacks)address;
	if (g_pObpCallPreOperationCallbacks == nullptr)
		return false;
	NTSTATUS status = DetourAttach((PVOID*)&g_pObpCallPreOperationCallbacks, ObpCallPreOperationCallbacks);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	return true;
}

bool BypassObjectCallback::Unbypass() {
	NTSTATUS status = DetourDetach((PVOID*)&g_pObpCallPreOperationCallbacks, ObpCallPreOperationCallbacks);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	return true;
}