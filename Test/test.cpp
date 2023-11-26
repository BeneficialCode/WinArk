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

struct FullItem {
	ULONG_PTR Value;
	HASH_ENTRY Entry;
};

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
	HashTableChangeTable(&g_Table, PAGE_SIZE / sizeof(SINGLE_LIST_ENTRY), pBuckets);

	do
	{
		auto item = (FullItem*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem), 'meti');
		if (!item)
			break;

		RtlZeroMemory(item, sizeof(FullItem));

		UINT64 value = 0x1000;
		UINT64 key = HashUlongPtr(value);
		item->Entry.HashVaue = key;
		item->Value = 0x7fff0;


		HashTableInsert(&g_Table, &item->Entry.Link);

		PHASH_BUCKET pBucket = NULL;
		FullItem* pData = NULL;
		pBucket = HashTableFindNext(&g_Table, key, pBucket);
		if (pBucket != NULL) {
			pData = CONTAINING_RECORD(pBucket, FullItem, Entry);
			KdPrint(("Value: %p\n", pData->Value));
		}

		HASH_TABLE_ITERATOR iter;
		HashTableIterInit(&iter, &g_Table);
		while (HashTableIterGetNext(&iter)) {
			pData = CONTAINING_RECORD(iter.Bucket, FullItem, Entry);
			HashTableIterRemove(&iter);
			KdPrint(("Value: %p\n", pData->Value));
			ExFreePoolWithTag(pData, 'meti');
		}

	} while (FALSE);

	

	PHASH_BUCKET p = HashTableCleanup(&g_Table);
	if (p) {
		ExFreePoolWithTag(p, 'tset');
	}

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
