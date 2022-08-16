#pragma once

using PObpCallPreOperationCallbacks = NTSTATUS
(NTAPI*)(
	POBJECT_TYPE ObjectType,
	POB_PRE_OPERATION_INFORMATION Info,
	PLIST_ENTRY PostCallbackListHead
);
