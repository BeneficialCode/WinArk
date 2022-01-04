#include "pch.h"
#include "SysMon.h"
#include "SysMonCommon.h"
#include "AutoLock.h"
#include "PEParser.h"
#include "AutoEnter.h"
#include "Logging.h"


ULONG	PspNotifyEnableMask;
EX_CALLBACK* PspLoadImageNotifyRoutine;
SysMonGlobals g_SysMonGlobals;


PUCHAR GetProcessNameByProcessId(HANDLE ProcessId) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process = nullptr;
	PUCHAR name = nullptr;
	status = PsLookupProcessByProcessId(ProcessId, &Process);
	if (NT_SUCCESS(status)) {
		name = PsGetProcessImageFileName(Process);
		ObDereferenceObject(Process);
	}
	return name;
}

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {
	//UNREFERENCED_PARAMETER(Process);

	if (CreateInfo) {
		// process create
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		if (CreateInfo->CommandLine) {
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}
		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, SYSMON_TAG);
		if (info == nullptr) {
			KdPrint((SYSMON_PREFIX "failed allocation\n"));
			return;
		}

		auto& item = info->Data;
		//KeQuerySystemTimePrecise(&item.Time);// Available starting with Windows 8.
		item.Type = ItemType::ProcessCreate;
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize;
		item.ProcessId = HandleToUlong(ProcessId);
		item.ParentProcessId = HandleToUlong(CreateInfo->ParentProcessId);

		if (commandLineSize > 0) {
			::memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer,
				commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}
		else {
			item.CommandLineLength = 0;
		}
		PushItem(&info->Entry);

		KdPrint(("[Library] %s [%ld] 创建进程%wZ\n", GetProcessNameByProcessId(CreateInfo->ParentProcessId),
			CreateInfo->ParentProcessId, CreateInfo->ImageFileName));
		PUCHAR name = PsGetProcessImageFileName(Process);
		if (!_stricmp(reinterpret_cast<const char*>(name), "calc.exe")) {
			KdPrint(("禁止创建计算器进程"));
			CreateInfo->CreationStatus = STATUS_UNSUCCESSFUL;// 返回失败
		}
	}
	else {
		// process exit
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool,
			sizeof(FullItem<ProcessExitInfo>), SYSMON_TAG);
		if (info == nullptr) {
			KdPrint((SYSMON_PREFIX "failed allocation\n"));
			return;
		}

		auto& item = info->Data;
		//KeQuerySystemTimePrecise(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);

		PushItem(&info->Entry);

		KdPrint(("进程退出：%s\n", PsGetProcessImageFileName(Process)));
	}
}

void OnThreadNotify(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create) {
	auto size = sizeof(FullItem<ThreadCreateExitInfo>);
	auto info = (FullItem<ThreadCreateExitInfo>*)ExAllocatePoolWithTag(PagedPool, size, SYSMON_TAG);
	if (info == nullptr) {
		KdPrint((SYSMON_PREFIX "Failed to allocate memory\n"));
		return;
	}
	auto& item = info->Data;
	//KeQuerySystemTimePrecise(&item.Time);
	item.Size = sizeof(item);
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToUlong(ThreadId);

	PushItem(&info->Entry);

	if (Create) {
		KdPrint(("线程创建: PID= %ld,TID= %ld\n", ProcessId, ThreadId));
	}
	else {
		KdPrint(("线程退出: PID= %ld,TID= %ld\n", ProcessId, ThreadId));
	}
}

void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
	if (ProcessId == nullptr) { // kernel image
		//system image , ignore
		PEParser parser(ImageInfo->ImageBase);
		auto entryPoint = parser.GetAddressEntryPoint();
		if (FullImageName)
			KdPrint(("[Library] Driver Load %wZ AddressEntryPoint 0x%p\n", FullImageName,entryPoint));
		else
			KdPrint(("[Library] Unknown Driver Load AddressEntryPoint: 0x%p\n",entryPoint));
		// 判断驱动名，修改入口点代码，达到禁止加载驱动。
		
		// do something...

		if (ImageInfo->ExtendedInfoPresent) {
			auto exinfo = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);
			// access FileObject
			
		}
		return;
	}
	// exe or dll image file
	auto size = sizeof(FullItem<ImageLoadInfo>);
	auto info = (FullItem<ImageLoadInfo>*)ExAllocatePoolWithTag(PagedPool, size, SYSMON_TAG);
	if (info == nullptr) {
		KdPrint((SYSMON_PREFIX "Failed to allocate memory\n"));
		return;
	}

	::memset(info, 0, size);

	auto& item = info->Data;
	//KeQuerySystemTimePrecise(&item.Time);
	item.Size = sizeof(item);
	item.Type = ItemType::ImageLoad;
	item.ProcessId = HandleToULong(ProcessId);
	item.ImageSize = ImageInfo->ImageSize;
	item.LoadAddress = ImageInfo->ImageBase;

	if (FullImageName) {
		::memcpy(item.ImageFileName, FullImageName->Buffer, min(FullImageName->Length,
			MaxImageFileSize * sizeof(WCHAR)));
	}
	else {
		::wcscpy_s(item.ImageFileName, L"(unknown)");
	}

	PushItem(&info->Entry);
}

NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2) {
	UNREFERENCED_PARAMETER(context);

	static const WCHAR machine[] = L"\\REGISTRY\\MACHINE\\";

	switch ((REG_NOTIFY_CLASS)(ULONG_PTR)arg1)
	{
	case RegNtPostSetValueKey:
		//...
		auto args = (REG_POST_OPERATION_INFORMATION*)arg2;
		if (!NT_SUCCESS(args->Status))
			break;

		PCUNICODE_STRING name;
		//if (NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&g_SysMonGlobals.RegCookie, args->Object,
		//	nullptr, &name, 0))) { // Available starting with Windows 8.
		//	// filter out none-HKLM writes
		//	if (::wcsncmp(name->Buffer, machine, ARRAYSIZE(machine) - 1) == 0) {
		//		auto preInfo = (REG_SET_VALUE_KEY_INFORMATION*)args->PreInformation;
		//		NT_ASSERT(preInfo);

		//		auto size = sizeof(FullItem<RegistrySetValueInfo>);
		//		auto info = (FullItem<RegistrySetValueInfo>*)ExAllocatePoolWithTag(PagedPool,
		//			size, SYSMON_TAG);
		//		if (info == nullptr)
		//			break;

		//		// zero out struture to make sure strings are null-terminated when copied
		//		RtlZeroMemory(info, size);

		//		// fill standard data
		//		auto& item = info->Data;
		//		KeQuerySystemTimePrecise(&item.Time);
		//		item.Size = sizeof(item);
		//		item.Type = ItemType::RegistrySetValue;

		//		// get client PID/TID (this is our caller)
		//		item.ProcessId = HandleToUlong(PsGetCurrentProcessId());
		//		item.ThreadId = HandleToUlong(PsGetCurrentThreadId());

		//		// get specific key/value data
		//		::wcsncpy_s(item.KeyName, name->Buffer, name->Length / sizeof(WCHAR) - 1);
		//		::wcsncpy_s(item.ValueName, preInfo->ValueName->Buffer,
		//			preInfo->ValueName->Length / sizeof(WCHAR) - 1);
		//		item.DataType = preInfo->Type;
		//		item.DataSize = preInfo->DataSize;
		//		::memcpy(item.Data, preInfo->Data, min(item.DataSize, sizeof(item.Data)));

		//		PushItem(&info->Entry);
		//	}

		//	CmCallbackReleaseKeyObjectIDEx(name);
		//}
		break;
	//case RegNtPreCreateKeyEx:
		/*auto args = (REG_CREATE_KEY_INFORMATION*)arg2;
		if (!NT_SUCCESS(args->Status))
			break;*/

		/*PUNICODE_STRING name;
		if (NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&g_SysMonGlobals.RegCookie, args->Object,
			nullptr, &name, 0))) {
			if (::wcsncmp(name->Buffer, machine, ARRAYSIZE(machine) - 1) == 0) {
				auto preInfo = (REG_CREATE_KEY_INFORMATION*)args->PreInformation;
				NT_ASSERT(preInfo);
				KdPrint(("%wZ", preInfo->CompleteName));
			}
			CmCallbackReleaseKeyObjectIDEx(name);
		}*/
		//break;
	}
	return STATUS_SUCCESS;
}

void PushItem(LIST_ENTRY* entry) {
	AutoLock<FastMutex> lock(g_SysMonGlobals.Mutex);
	if (g_SysMonGlobals.ItemCount > 1024) {
		// too many items,remove oldest one
		auto head = RemoveHeadList(&g_SysMonGlobals.ItemHead);
		g_SysMonGlobals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}
	InsertTailList(&g_SysMonGlobals.ItemHead, entry);
	g_SysMonGlobals.ItemCount++;
}

BOOLEAN ExFastRefCanBeReferenced(PEX_FAST_REF_S ref)
{
	return ref->RefCnt != 0;
}

BOOLEAN ExFastRefObjectNull(PEX_FAST_REF_S ref)
{
	return (BOOLEAN)(ref->Value == 0);
}

PVOID ExFastRefGetObject(PEX_FAST_REF_S ref){
	return (PVOID)(ref->Value & ~MAX_FAST_REFS);
}

PEX_CALLBACK_ROUTINE_BLOCK ExReferenceCallBackBlock(PEX_FAST_REF_S ref)
{
	if (ExFastRefObjectNull(ref)) {
		return NULL;
	}

	if (!ExFastRefCanBeReferenced(ref)) {
		return NULL;
	}

	return (PEX_CALLBACK_ROUTINE_BLOCK)ExFastRefGetObject(ref);
}

LOGICAL ExFastRefDereference(
	_Inout_ PEX_FAST_REF_S FastRef,
	_In_ PVOID Object
) {
	EX_FAST_REF_S OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);
		if ((OldRef.Value ^ (ULONG_PTR)Object) >= MAX_FAST_REFS) {
			return FALSE;
		}

		NewRef.Value = OldRef.Value + 1;
		NewRef.Object = InterlockedCompareExchangePointerRelease(
			&FastRef->Object,
			NewRef.Object,
			OldRef.Object
		);
		if (NewRef.Object != OldRef.Object) {
			continue;
		}
		break;
	}
	return TRUE;
}

VOID ExDereferenceCallBackBlock(
	_Inout_ PEX_CALLBACK Callback,
	_In_ PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock
) {
	if (!ExFastRefDereference(&Callback->RoutineBlock, CallbackBlock)) {
		ExReleaseRundownProtection(&CallbackBlock->RundownProtect);
	}
}



VOID PsCallImageNotifyRoutines(
	_In_ PUNICODE_STRING ImageName,
	_In_ HANDLE ProcessId,
	_In_ PVOID FileObject,
	_In_ PIMAGE_INFO_EX ImageInfoEx
) {
	PLOAD_IMAGE_NOTIFY_ROUTINE Routine;
	PEX_CALLBACK_ROUTINE_BLOCK Callback;

	CriticalRegion critical;
	AutoEnter<CriticalRegion> enter(critical);

	if (PspNotifyEnableMask & 1) {
		ImageInfoEx->Size = sizeof(IMAGE_INFO_EX);
		ImageInfoEx->ImageInfo.ExtendedInfoPresent = TRUE;
		ImageInfoEx->FileObject = (FILE_OBJECT*)FileObject;

		for (ULONG i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
			Callback = ExReferenceCallBackBlock(&PspLoadImageNotifyRoutine[i].RoutineBlock);
			if (Callback != nullptr) {
				Routine = (PLOAD_IMAGE_NOTIFY_ROUTINE)Callback->Function;
				Routine(ImageName,
					ProcessId, &ImageInfoEx->ImageInfo);
				ExDereferenceCallBackBlock(&PspLoadImageNotifyRoutine[i], Callback);
			}
		}
	}
}

bool EnumSystemNotify(PEX_CALLBACK callback,ULONG count,KernelCallbackInfo* info) {
	if (!callback) 
		return false;

	KdPrint(("Count: %d\n", count));

#ifdef _WIN64
	static ULONG Max = 64;
#else
	static ULONG Max = 8;
#endif

	int j = 0;
	for (ULONG i = 0; i < Max; i++) {
		if (j == count)
			break;
		if (!MmIsAddressValid(callback)) 
			break;
		auto block = ExReferenceCallBackBlock(&callback->RoutineBlock);
		if (block != nullptr) {
			KdPrint(("SystemNotifyFunc: 0x%p\n", (ULONG64)block->Function));
			info->Address[j] = block->Function;
			ExDereferenceCallBackBlock(callback, block);
			++j;
		}
		callback++;
	}
	return true;
}



bool EnumRegistryNotify(PEXT_CALLBACK callback) {
	if (!callback)
		return false;
	
}

bool EnumObCallbackNotify(POBJECT_TYPE objectType,ULONG callbackListOffset,ObCallbackInfo* info) {
	PLIST_ENTRY callbackListHead = nullptr;
	PLIST_ENTRY nextEntry = nullptr;
	POB_CALLBACK_ENTRY callbackEntry = nullptr;
	ULONG count = 0;

	if (!objectType) {
		return false;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)objectType + callbackListOffset);
	nextEntry = callbackListHead->Flink;
	int i = 0;
	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, EntryItemList);
		LogInfo("PreOperation %p, PostOperation: %p\n", callbackEntry->PreOperation, callbackEntry->PostOperation);
		if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_CREATE))
			LogInfo("Protect handle from creating\n");
		if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_DUPLICATE))
			LogInfo("Protect handle from duplicating\n");
		info[i].PostOperation = callbackEntry->PostOperation;
		info[i].PreOperation = callbackEntry->PreOperation;
		i++;
		nextEntry = nextEntry->Flink;
	}

	return false;
}

LONG GetObCallbackCount(POBJECT_TYPE objectType, ULONG callbackListOffset) {
	PLIST_ENTRY callbackListHead = nullptr;
	PLIST_ENTRY nextEntry = nullptr;
	POB_CALLBACK_ENTRY callbackEntry = nullptr;
	volatile LONG count = 0;

	if (!objectType) {
		return count;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)objectType + callbackListOffset);
	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, EntryItemList);
		InterlockedIncrement(&count);
		nextEntry = nextEntry->Flink;
	}

	return count;
}

