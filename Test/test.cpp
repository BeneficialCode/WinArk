#include <ntifs.h>
#include <fltKernel.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include "..\KernelLibrary\reflector.h"
#include "..\KernelLibrary\detours.h"


DRIVER_UNLOAD DriverUnload;

PUCHAR g_pNewKiClearLastBranchRecordStack;
// u nt!KiClearLastBranchRecordStack
PUCHAR g_pAddr = (PUCHAR)0xfffff80209a73cd0;

extern "C" 
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;
	g_pNewKiClearLastBranchRecordStack = (PUCHAR)Reflector(g_pAddr, 0x30, NULL);
	KdPrint(("pNewKiClearLastBranchRecordStack: %p\n", g_pNewKiClearLastBranchRecordStack));
	NTSTATUS status = DetourAttach((PVOID*)&g_pAddr, g_pNewKiClearLastBranchRecordStack);
	if (!NT_SUCCESS(status))
		return status;
	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return status;

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	NTSTATUS status = DetourDetach((PVOID*)&g_pAddr, g_pNewKiClearLastBranchRecordStack);
	status = DetourTransactionCommit();
	if (g_pNewKiClearLastBranchRecordStack != NULL) {
		ExFreePool(g_pNewKiClearLastBranchRecordStack);
	}
	KdPrint(("Driver unload success!"));
}
