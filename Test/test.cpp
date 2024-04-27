#include <ntifs.h>
#include <fltKernel.h>
#include <minwindef.h>
#include <ntstrsafe.h>
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


struct FZG
{
	int height;
	int age;
	HASH_BUCKET bucket;
	char* name;
};


extern "C" 
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	HashTableInitialize(&g_Table, 0, 0, nullptr);


	int AllocCount = 100;


	PHASH_BUCKET pBuckets = (PHASH_BUCKET)ExAllocatePoolWithTag(PagedPool, AllocCount * sizeof(PSINGLE_LIST_ENTRY), 'tset');
	if (!pBuckets) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	HashTableChangeTable(&g_Table, AllocCount, (PSINGLE_LIST_ENTRY)pBuckets);

	do
	{
		auto item = (FZG*)ExAllocatePoolWithTag(PagedPool, sizeof(FZG), 'meti');
		auto item2 = (FZG*)ExAllocatePoolWithTag(PagedPool, sizeof(FZG), 'meti');
		RtlZeroMemory(item, sizeof(FZG));
		RtlZeroMemory(item2, sizeof(FZG));

		

		item->age = 18;
		item->height = 183;
		item->name = "ShuaiGeZengZhenDeShuai";
		UINT64 hash = HashUlongPtr((UINT64)item->name);
		item->bucket.Key = hash;
		
		item2->age = 19;
		item2->height = 182;
		item2->name = "ShuaiGeCunZhenDeShuai";
		UINT64 hash2 = HashUlongPtr((UINT64)item2->name);
		item2->bucket.Key = hash2;

		
		HashTableInsert(&g_Table, &item->bucket);
		HashTableInsert(&g_Table, &item2->bucket);

		PSINGLE_LIST_ENTRY pBucket = NULL;
		while (TRUE) {
			pBucket = HashTableFindNext(&g_Table, hash, pBucket);
			if (!pBucket) {
				KdPrint(("Not Found!"));
				break;
			}

			FZG* result = CONTAINING_RECORD(pBucket, FZG, bucket);
			KdPrint(("%d %d %s", result->age, result->height, result->name));
		}


		DbgBreakPoint();
		HASH_TABLE_ITERATOR Iterator;
		HashTableIterInit(&Iterator, &g_Table);
		while (HashTableIterGetNext(&Iterator))
		{
			FZG* result = CONTAINING_RECORD(Iterator.HashEntry, FZG, bucket);
			KdPrint(("result %p", result));
			HashTableIterRemove(&Iterator);
			//KdPrint(("Iterator %d %d %s", result->age, result->height, result->name));
			//ExFreePoolWithTag(result, 'meti');
			
		}
	} while (FALSE);

	

	PSINGLE_LIST_ENTRY p = HashTableCleanup(&g_Table);
	if (p) {
		ExFreePoolWithTag(p, 'tset');
	}

	DbgBreakPoint();
	HASH_TABLE_ITERATOR iter;
	// WdBoot.sys	0xFFFFF80167830000	0xA000	25	Microsoft Corporation	C:\Windows\system32\drivers\wd\WdBoot.sys
	HashTableIterInit(&iter, (PHASH_TABLE)(0xFFFFF80167830000 + 0x20AF0));
	while (HashTableIterGetNext(&iter))
	{
		KdPrint(("result %p", iter.HashEntry));
	}

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
