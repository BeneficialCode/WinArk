#include <ntifs.h>
#include <fltKernel.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include "..\KernelLibrary\reflector.h"
#include "..\KernelLibrary\detours.h"
#include "..\KernelLibrary\khook.h"


DRIVER_UNLOAD DriverUnload;

PUCHAR g_pNewKiClearLastBranchRecordStack;
PUCHAR g_pNewNtUserGetForegroundWindow;
// u nt!KiClearLastBranchRecordStack
// u win32k!NtUserGetForegroundWindow
PUCHAR g_pAddr = (PUCHAR)0xfffff96000082744;

extern "C" 
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	bool success = khook::SearchSessionProcess();
	if (!success)
		return STATUS_UNSUCCESSFUL;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(khook::_pid, &Process);
	if (!NT_SUCCESS(status))
		return STATUS_UNSUCCESSFUL;

	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);

	g_pNewNtUserGetForegroundWindow = (PUCHAR)Reflector(g_pAddr, 0x30, NULL);
	KdPrint(("pNewNtUserGetForegroundWindow: %p\n", g_pNewNtUserGetForegroundWindow));
	status = DetourAttach((PVOID*)&g_pAddr, g_pNewNtUserGetForegroundWindow);
	if (g_pNewNtUserGetForegroundWindow != nullptr)
		ExFreePool(g_pNewNtUserGetForegroundWindow);
	if (!NT_SUCCESS(status))
		return status;
	
	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
