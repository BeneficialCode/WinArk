#pragma once

using PObpCallPreOperationCallbacks = NTSTATUS
(NTAPI*)(
	POBJECT_TYPE ObjectType,
	POB_PRE_OPERATION_INFORMATION Info,
	PLIST_ENTRY PostCallbackListHead
	);

NTSTATUS NTAPI
ObpCallPreOperationCallbacks(POBJECT_TYPE ObjectType,
	POB_PRE_OPERATION_INFORMATION Info,
	PLIST_ENTRY PostCallbackListHead);

struct BypassObjectCallback {
	static bool Bypass(void* address);
	static bool Unbypass();
};
