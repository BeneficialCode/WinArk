#include "pch.h"
#include "BypassAntiKernelDbg.h"
#include "detours.h"

PVOID BypassAntiKernelDbg::GetExportSymbolAddress(PCWSTR symbolName) {
	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, symbolName);
	return MmGetSystemRoutineAddress(&uname);
}

NTSTATUS NTAPI HookNtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_ PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
) {
	NTSTATUS status = BypassAntiKernelDbg::g_pNtQuerySystemInformation(SystemInformationClass, 
		SystemInformation, 
		SystemInformationLength,
		ReturnLength);
	if (NT_SUCCESS(status) && SystemInformation) {
		if (SystemInformationClass == SystemKernelDebuggerInformation) {
			typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION
			{
				BOOLEAN DebuggerEnabled;
				BOOLEAN DebuggerNotPresent;
			} SYSTEM_KERNEL_DEBUGGER_INFORMATION, * PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

			SYSTEM_KERNEL_DEBUGGER_INFORMATION* info = (SYSTEM_KERNEL_DEBUGGER_INFORMATION*)SystemInformation;
			__try {
				info->DebuggerEnabled = false;
				info->DebuggerNotPresent = true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				status = STATUS_ACCESS_VIOLATION;
			}
		}
	}

	return status;
}

NTSTATUS BypassAntiKernelDbg::Bypass() {
	g_pNtQuerySystemInformation = (PNtQuerySystemInformation)GetExportSymbolAddress(L"NtQuerySystemInformation");
	if (g_pNtQuerySystemInformation) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pNtQuerySystemInformation, HookNtQuerySystemInformation);
		if (!NT_SUCCESS(status))
			return status;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return status;
	}
	return STATUS_SUCCESS;

}

NTSTATUS BypassAntiKernelDbg::Unbypass() {
	NTSTATUS status = DetourDetach((PVOID*)&g_pNtQuerySystemInformation, HookNtQuerySystemInformation);
	if (!NT_SUCCESS(status))
		return status;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return status;

	return STATUS_SUCCESS;
}