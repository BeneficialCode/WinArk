#include "pch.h"
#include "SysMon.h"
#include "SysMonCommon.h"
#include "AutoLock.h"
#include "PEParser.h"
#include "AutoEnter.h"
#include "Logging.h"
#include "FileManager.h"
#include "Memory.h"
#include "khook.h"



ULONG	PspNotifyEnableMask;
EX_CALLBACK* PspLoadImageNotifyRoutine;
SysMonGlobals g_SysMonGlobals;
UNICODE_STRING g_BackupDir;

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
		HANDLE pid = PsGetCurrentProcessId();
		LogInfo("pid = %d\n", pid);
		//system image , ignore
		PEParser parser(ImageInfo->ImageBase);
		auto entryPoint = parser.GetAddressEntryPoint();
		if (FullImageName) {
			KdPrint(("[Library] Driver Load %wZ AddressEntryPoint 0x%p\n", FullImageName, entryPoint));
			BackupFile(FullImageName);
		}
		else
			KdPrint(("[Library] Unknown Driver Load AddressEntryPoint: 0x%p\n",entryPoint));
		// 判断驱动名，修改入口点代码，达到禁止加载驱动。
		
		// do something...
		
		if (ImageInfo->ExtendedInfoPresent) {
			auto exinfo = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);
			// access FileObject
			PFLT_FILE_NAME_INFORMATION nameInfo;
			if (NT_SUCCESS(FltGetFileNameInformationUnsafe(exinfo->FileObject, nullptr,
				FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo))) {
				LogInfo("FileNameInfo %wZ\n", &nameInfo->Name);
				FltReleaseFileNameInformation(nameInfo);
			}
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

LOGICAL
ExFastRefAddAdditionalReferenceCounts(
	_Inout_ PEX_FAST_REF_S FastRef,
	_In_ PVOID Object,
	_In_ ULONG RefsToAdd
) {
	EX_FAST_REF_S OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);

		if (OldRef.RefCnt + RefsToAdd > MAX_FAST_REFS ||
			(ULONG_PTR)Object != (OldRef.Value & ~MAX_FAST_REFS)) {
			return FALSE;
		}

		NewRef.Value = OldRef.Value + RefsToAdd;
		NewRef.Object = InterlockedCompareExchangePointerAcquire(&FastRef->Object,
			NewRef.Object, OldRef.Object);

		if (NewRef.Object != OldRef.Object) {
			continue;
		}
		break;
	}
	return TRUE;
}

PEX_CALLBACK_ROUTINE_BLOCK ExReferenceCallBackBlock(
	_Inout_ PEX_CALLBACK Callback
) {
	EX_FAST_REF_S OldRef;
	PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;


	if (Callback->RoutineBlock.RefCnt & MAX_FAST_REFS) {
		OldRef = ExFastReference(&Callback->RoutineBlock);
		if (OldRef.Value == 0)
			return nullptr;
	}

	if (OldRef.RefCnt == 0)
		return nullptr;

	if (!(OldRef.RefCnt & MAX_FAST_REFS)) {

		KeEnterCriticalRegion();

		CallbackBlock = (PEX_CALLBACK_ROUTINE_BLOCK)(Callback->RoutineBlock.Value & ~MAX_FAST_REFS);
		if (CallbackBlock && !ExAcquireRundownProtection(&CallbackBlock->RundownProtect)) {
			CallbackBlock = nullptr;
		}

		KeLeaveCriticalRegion();

		if (CallbackBlock == nullptr) {
			return nullptr;
		}
	}
	else {
		CallbackBlock = (PEX_CALLBACK_ROUTINE_BLOCK)(OldRef.Value & ~MAX_FAST_REFS);

		if (OldRef.RefCnt == 1 &&
			ExAcquireRundownProtectionEx(&CallbackBlock->RundownProtect, MAX_FAST_REFS)) {

			if (!ExFastRefAddAdditionalReferenceCounts(&Callback->RoutineBlock,
				CallbackBlock, MAX_FAST_REFS)) {
				ExReleaseRundownProtectionEx(&CallbackBlock->RundownProtect, MAX_FAST_REFS);
			}
		}
	}

	return CallbackBlock;
}

EX_FAST_REF_S ExFastReference(
	_Inout_ PEX_FAST_REF_S FastRef
) {
	EX_FAST_REF_S OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);

		if (OldRef.RefCnt != 0) {
			NewRef.Value = OldRef.Value - 1;
			NewRef.Object = InterlockedCompareExchangePointerAcquire(
				&FastRef->Object,
				NewRef.Object,
				OldRef.Object);
			if (NewRef.Object != OldRef.Object) {
				continue;
			}
		}
		break;
	}

	return OldRef;
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
			Callback = ExReferenceCallBackBlock(&PspLoadImageNotifyRoutine[i]);
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
		auto block = ExReferenceCallBackBlock(callback);
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



bool EnumRegistryNotify(PLIST_ENTRY pListHead,CmCallbackInfo* info) {
	if (!pListHead)
		return false;

	PLIST_ENTRY callbackListHead = pListHead;
	PLIST_ENTRY nextEntry = callbackListHead->Flink;
	PCM_CALLBACK_CONTEXT_BLOCKEX callbackEntry = nullptr;
	int i = 0;
	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, CM_CALLBACK_CONTEXT_BLOCKEX, ListEntry);
		LogInfo("Cookie %p, Function: %p\n", callbackEntry->Cookie, callbackEntry->Function);
		info[i].Address = callbackEntry->Function;
		info[i].Cookie = callbackEntry->Cookie;
		++i;
		nextEntry = nextEntry->Flink;
	}

	return true;
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
		info[i].RegistrationHandle = callbackEntry->RegistrationHandle;
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

NTSTATUS BackupFile(_In_ PUNICODE_STRING FileName) {
	NTSTATUS status = STATUS_SUCCESS;
	FileManager mgrS,mgrT;
	void* buffer = nullptr;
	IO_STATUS_BLOCK ioStatus;

	do
	{
		// open source file
		status = mgrS.Open(FileName, FileAccessMask::Read | FileAccessMask::Synchronize);
		if (!NT_SUCCESS(status))
			break;

		// get source file size
		LARGE_INTEGER fileSize;
		status = mgrS.GetFileSize(&fileSize);
		if (!NT_SUCCESS(status) || fileSize.QuadPart == 0)
			break;

		// open target file
		UNICODE_STRING targetFileName;
		auto sysName = wcsrchr(FileName->Buffer, L'\\') + 1;
		if (sysName == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		USHORT len = wcslen(sysName);
		const WCHAR backupStream[] = L"_backup.sys";
		
		targetFileName.MaximumLength = g_BackupDir.Length + len * sizeof(WCHAR)+ sizeof(backupStream);
		targetFileName.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, targetFileName.MaximumLength, 'kuab');
		if (targetFileName.Buffer == nullptr){
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlCopyUnicodeString(&targetFileName, &g_BackupDir);
		RtlAppendUnicodeToString(&targetFileName, sysName);
		RtlAppendUnicodeToString(&targetFileName, backupStream);

		status = mgrT.Open(&targetFileName, FileAccessMask::Write | FileAccessMask::Synchronize);
		ExFreePool(targetFileName.Buffer);

		if (!NT_SUCCESS(status))
			break;

		// allocate buffer for copying purpose
		ULONG size = 1 << 21; // 2 MB
		buffer = ExAllocatePoolWithTag(PagedPool, size, 'kcab');
		if (!buffer) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		// loop - read from source,write to target
		LARGE_INTEGER offset = { 0 }; // read
		LARGE_INTEGER writeOffset = { 0 }; // write

		ULONG bytes;
		auto saveSize = fileSize;
		while (fileSize.QuadPart > 0) {
			status = mgrS.ReadFile(buffer, (ULONG)min((ULONGLONG)size, fileSize.QuadPart), &ioStatus, &offset);
			if (!NT_SUCCESS(status))
				break;

			bytes = (ULONG)ioStatus.Information;

			// write to target file
			status = mgrT.WriteFile(buffer, bytes, &ioStatus, &writeOffset);

			if (!NT_SUCCESS(status))
				break;

			// update byte count and offsets
			offset.QuadPart += bytes;
			writeOffset.QuadPart += bytes;
			fileSize.QuadPart -= bytes;
		}

		FILE_END_OF_FILE_INFORMATION info;
		info.EndOfFile = saveSize;
		NT_VERIFY(NT_SUCCESS(mgrT.SetInformationFile(&ioStatus, &info, sizeof(info), FileEndOfFileInformation)));
	} while (false);

	if (buffer)
		ExFreePool(buffer);
	
	return status;
}


void RemoveImageNotify(_In_ PVOID context) {
	NTSTATUS status = PsRemoveLoadImageNotifyRoutine(OnImageLoadNotify);
	if (!NT_SUCCESS(status)) {
		LogError("failed to remove image load callbacks (status=%08X)\n", status);
	}
	// free the buckup dir memory
	ExFreePool(g_BackupDir.Buffer);

	// free remaining items
	while (!IsListEmpty(&g_SysMonGlobals.ItemHead)) {
		auto entry = RemoveHeadList(&g_SysMonGlobals.ItemHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
	PsTerminateSystemThread(status);
}

NTSTATUS RemoveSystemNotify(_In_ PVOID context) {
	auto notify = (NotifyData*)context;
	NTSTATUS status = STATUS_SUCCESS;
	switch (notify->Type)
	{	
		case NotifyType::LoadImageNotify:
		{
			status = PsRemoveLoadImageNotifyRoutine(reinterpret_cast<PLOAD_IMAGE_NOTIFY_ROUTINE>(notify->Address));
			if (!NT_SUCCESS(status)) {
				LogError("failed to remove image load callbacks (status=%08X)\n", status);
			}
			break;
		}
			
		case NotifyType::CreateProcessNotify:
		{
			
			status = PsSetCreateProcessNotifyRoutineEx(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>(notify->Address), TRUE);
			if (status == STATUS_PROCEDURE_NOT_FOUND) {
				status = PsSetCreateProcessNotifyRoutine(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE>(notify->Address), TRUE);
			}
			if (status == STATUS_PROCEDURE_NOT_FOUND) {
				// PsSetCreateProcessNotifyRoutineEx2
				PPsSetCreateProcessNotifyRoutineEx2 pPsSetCreateProcessNotifyRoutineEx2 = nullptr;
				pPsSetCreateProcessNotifyRoutineEx2 = (PPsSetCreateProcessNotifyRoutineEx2)khook::GetApiAddress(L"PsSetCreateProcessNotifyRoutineEx2");
				if(nullptr != pPsSetCreateProcessNotifyRoutineEx2)
					status = pPsSetCreateProcessNotifyRoutineEx2(0, notify->Address, TRUE);
			}
			break;
		}

		case NotifyType::CreateThreadNotify:
		{
			status = PsRemoveCreateThreadNotifyRoutine(reinterpret_cast<PCREATE_THREAD_NOTIFY_ROUTINE>(notify->Address));
			break;
		}
		case NotifyType::ThreadObjectNotify:
		case NotifyType::ProcessObjectNotify:
		{
			ObUnRegisterCallbacks(notify->Address);
		}

		case NotifyType::RegistryNotify:
		{
			status = CmUnRegisterCallback(notify->Cookie);
			break;
		}
		default:
			break;
	}
	
	return status;
}