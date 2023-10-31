#include <ntifs.h>
#include <fltKernel.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include "..\KernelLibrary\reflector.h"


DRIVER_UNLOAD DriverUnload;


extern "C" 
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;
	// u nt!KiClearLastBranchRecordStack
	PUCHAR pNewKiClearLastBranchRecordStack = (PUCHAR)Reflector((PUCHAR)0xfffff8071ba54920, 0x30, NULL);
	
	KdPrint(("pNewKiClearLastBranchRecordStack: %p\n", pNewKiClearLastBranchRecordStack));
	
	if (pNewKiClearLastBranchRecordStack != NULL) {
		ExFreePool(pNewKiClearLastBranchRecordStack);
	}

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
