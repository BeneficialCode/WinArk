#include <ntifs.h>
#include <fltKernel.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include "..\KernelLibrary\reflector.h"
#include "..\KernelLibrary\detours.h"
#include "..\KernelLibrary\khook.h"
#include "..\KernelLibrary\HashTable.h"


DRIVER_UNLOAD DriverUnload;

PUCHAR g_pNewKiClearLastBranchRecordStack;
PUCHAR g_pNewNtUserGetForegroundWindow;
// u nt!KiClearLastBranchRecordStack
// u win32k!NtUserGetForegroundWindow
PUCHAR g_pAddr = (PUCHAR)0xfffff96000082744;

HASH_TABLE g_Table;



extern "C" 
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	HashTableInitialize(&g_Table, 0, 0, nullptr);

	PHASH_BUCKET pBuckets = (PHASH_BUCKET)ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'tset');
	if (!pBuckets) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	HashTableChangeTable(&g_Table, PAGE_SIZE / 8, pBuckets);

	do
	{
		auto item = (HASH_BUCKET*)ExAllocatePoolWithTag(PagedPool, sizeof(HASH_BUCKET), 'meti');
		if (!item)
			break;

		RtlZeroMemory(item, sizeof(HASH_BUCKET));
		
		item->HashValue = 0x1000;
		HashTableInsert(&g_Table, item);

	} while (FALSE);




	HASH_TABLE_ITERATOR iter;
	HashTableIterInit(&iter, &g_Table);
	while (HashTableIterGetNext(&iter)) {
		HashTableIterRemove(&iter);
	}
	PHASH_BUCKET p = HashTableCleanup(&g_Table);
	if (p) {
		ExFreePoolWithTag(p, 'tset');
	}

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
