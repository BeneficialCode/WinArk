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
		item->bucket.HashValue = hash;
		
		item2->age = 19;
		item2->height = 182;
		item2->name = "ShuaiGeCunZhenDeShuai";
		UINT64 hash2 = HashUlongPtr((UINT64)item2->name);
		item2->bucket.HashValue = hash2;

		
		HashTableInsert(&g_Table, &item->bucket);
		HashTableInsert(&g_Table, &item2->bucket);

		PHASH_BUCKET pBucket = NULL;
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

	

	PHASH_BUCKET p = HashTableCleanup(&g_Table);
	if (p) {
		ExFreePoolWithTag(p, 'tset');
	}

	DbgBreakPoint();
	HASH_TABLE_ITERATOR iter;
	HashTableIterInit(&iter, (PHASH_TABLE)(0xfffff80510af0000 + 0x20AF0));
	while (HashTableIterGetNext(&iter))
	{
		KdPrint(("result %p", iter.HashEntry));
		HashTableIterRemove(&iter);
	}
	// 好像不是test.sys蓝的，肯定必现，你没结构体大小 不好验证。
	// 没问题吧？？？？ 每 怎么验证正确性：） 结构体 你需要它的结构体大小 还有bucket所在的位置
	// // -0x10 + 0x30 = name 你试一下继续下个段
	// 不看以下新版本的吗 一样
	// 代码要动吗 不动 断点断了以后 给我操作 我看看 // 结构体没对上 -0x10 + 0x30不对 // 确实是+0x38 我刚刚看错了 意思就是遍历对了
	// 按错了 md 重新来一下 我看看为什么蓝屏 这个iter貌似是要配合Remove一起用的
	// 这样试试？这样我估计会蓝 因为毕竟你是在遍历别人的链表 是没有锁的 别人在你遍历的过程里面 删一个你就炸了 还有可能是这个原因。

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT) {
	KdPrint(("Driver unload success!"));
}
