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
#include "Helpers.h"
#include "Section.h"
#include "SysMon.h"
#include "detours.h"
#include "kstring.h"

#define LIMIT_INJECTION_TO_PROC L"notepad.exe"	// Process to limit injection to (only in Debugger builds)


SysMonGlobals g_SysMonGlobals;
UNICODE_STRING g_BackupDir;

Section g_sec;		// native section object

#ifdef _WIN64
Section g_secWow;	// Wow64 section object
#endif // _WIN64

/*
	PspNotifyEnableMask是一个系统通知回调是否产生的一个标记
	0位——标记是否产生模块回调。
	3位——标记是否产生线程回调。
	其他位有待分析......
*/
PULONG	g_pPspNotifyEnableMask;
PEX_CALLBACK g_pPspLoadImageNotifyRoutine;

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

		LogInfo("%s [%ld] Create Process %wZ\n", GetProcessNameByProcessId(CreateInfo->ParentProcessId),
			CreateInfo->ParentProcessId, CreateInfo->ImageFileName);
		PUCHAR name = PsGetProcessImageFileName(Process);
		if (!_stricmp(reinterpret_cast<const char*>(name), "calc.exe")) {
			LogInfo("Disable create calc.exe!");
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
		bool sameProcess = ProcessId == PsGetCurrentProcessId();
		LogInfo("Thread Create: PID= %ld,TID= %ld Same process: %d\n", 
			ProcessId, ThreadId, sameProcess);
	}
	else {
		LogInfo("Thread Exit: PID= %ld,TID= %ld\n", ProcessId, ThreadId);
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
	ASSERT(ImageInfo);

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

	static UNICODE_STRING kernel32 = RTL_CONSTANT_STRING(L"\\kernel32.dll");
	// We are looking for kernel32.dll only - skip the rest
	if (!ImageInfo->SystemModeImage && // skip anything mapped into kernel
		ProcessId == PsGetCurrentProcessId() &&
		Helpers::IsSuffixedUnicodeString(FullImageName, &kernel32) && // Need kernel32.dll only
		Helpers::IsMappedByLdrLoadDll(&kernel32)		// Make sure that it's a call from the LdrLoadDll() function
#if defined(DBG) && defined(LIMIT_INJECTION_TO_PROC)
		&& Helpers::IsSpecificProcess(ProcessId, LIMIT_INJECTION_TO_PROC, FALSE) // for debug build limit to specific process only (for debug purpose)
#endif
		) {
#ifdef _WIN64
		// Is it a 32-bit process running in a 64-bit OS
		bool wowProc = IoIs32bitProcess(nullptr);
#else
		// Cannot be a WOW64 process on a 32-bit OS
		bool wowProc = false;
#endif // _WIN64

		// Now we can prceed with our injection
		LogDebug("Image load for (WOW=%d) PID=%u: \"%wZ\"", wowProc, HandleToUlong(ProcessId), FullImageName);

		// Get our (DLL) section to inject
		DllStats* pDllState;
		NTSTATUS status = g_sec.GetSection(&pDllState);
		if (NT_SUCCESS(status)) {
			// Add inject now

		}
		else {
			LogError("Error: 0x%x g_sec.GetSection PID=%u\n", status, HandleToUlong(ProcessId));
		}

		// The following only applies to a 64-bit build
		// INFO: We need to inject our dll into a 32 bit process too...
#ifdef _WIN64
		if (wowProc) {
			status = g_secWow.GetSection(&pDllState);
			if (NT_SUCCESS(status)) {
				// Add inject now
			}
			else {
				// Error
				LogError("Error: 0x%x g_secWow.GetSection PID=%u\n", status, HandleToUlong(ProcessId));
			}
		}
#endif
	}
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

BOOLEAN
ExFastRefObjectNull(
	_In_ EX_FAST_REF_S FastRef
) {
	return (BOOLEAN)(FastRef.Value == 0);
}

PVOID
ExFastRefGetObject(
	_In_ EX_FAST_REF_S FastRef
) {
	return (PVOID)(FastRef.Value & ~MAX_FAST_REFS);
}

LOGICAL
ExFastRefIsLastReference(
	_In_ EX_FAST_REF_S FastRef
) {
	return FastRef.RefCnt == 1;
}

PEX_CALLBACK_ROUTINE_BLOCK ExReferenceCallBackBlock(
	_Inout_ PEX_CALLBACK Callback
) {
	EX_FAST_REF_S OldRef;
	PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;


	OldRef = ExFastReference(&Callback->RoutineBlock);
	if (OldRef.Value == 0)
		return nullptr;
	
	if (ExFastRefObjectNull(OldRef))
		return nullptr;

	if (!(OldRef.RefCnt & MAX_FAST_REFS)) {

		KeEnterCriticalRegion();
		
		CallbackBlock = (PEX_CALLBACK_ROUTINE_BLOCK)ExFastRefGetObject(Callback->RoutineBlock);
		if (CallbackBlock && !ExAcquireRundownProtection(&CallbackBlock->RundownProtect)) {
			CallbackBlock = nullptr;
		}

		KeLeaveCriticalRegion();

		if (CallbackBlock == nullptr) {
			return nullptr;
		}
	}
	else {
		CallbackBlock = (PEX_CALLBACK_ROUTINE_BLOCK)ExFastRefGetObject(OldRef);

		if (ExFastRefIsLastReference(OldRef) &&
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

bool EnumObCallbackNotify(ULONG callbackListOffset,ObCallbackInfo* info) {
	PLIST_ENTRY callbackListHead = nullptr;
	PLIST_ENTRY nextEntry = nullptr;
	POB_CALLBACK_ENTRY callbackEntry = nullptr;
	ULONG count = 0;

	if (callbackListOffset == -1) {
		return false;
	}

	int i = 0;

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*PsThreadType + callbackListOffset);
	nextEntry = callbackListHead->Flink;
	
	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
		if (ExAcquireRundownProtection(&callbackEntry->RundownProtect)) {
			KdPrint(("PreOperation %p, PostOperation: %p\n", callbackEntry->PreOperation, callbackEntry->PostOperation));
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_CREATE))
				KdPrint(("Protect handle from creating\n"));
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_DUPLICATE))
				KdPrint(("Protect handle from duplicating\n"));
			info[i].Type = ObjectCallbackType::Thread;
			info[i].Operations = callbackEntry->Operations;
			info[i].CallbackEntry = callbackEntry;
			info[i].PostOperation = callbackEntry->PostOperation;
			info[i].PreOperation = callbackEntry->PreOperation;
			info[i].RegistrationHandle = callbackEntry->RegistrationHandle;
			info[i].Enabled = callbackEntry->Enabled;
			i++;
			ExReleaseRundownProtection(&callbackEntry->RundownProtect);
		}
		nextEntry = nextEntry->Flink;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*PsProcessType + callbackListOffset);
	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
		if (ExAcquireRundownProtection(&callbackEntry->RundownProtect)) {
			KdPrint(("PreOperation %p, PostOperation: %p\n", callbackEntry->PreOperation, callbackEntry->PostOperation));
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_CREATE))
				KdPrint(("Protect handle from creating\n"));
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_DUPLICATE))
				KdPrint(("Protect handle from duplicating\n"));
			info[i].Type = ObjectCallbackType::Process;
			info[i].Operations = callbackEntry->Operations;
			info[i].CallbackEntry = callbackEntry;
			info[i].PostOperation = callbackEntry->PostOperation;
			info[i].PreOperation = callbackEntry->PreOperation;
			info[i].RegistrationHandle = callbackEntry->RegistrationHandle;
			info[i].Enabled = callbackEntry->Enabled;
			i++;
			ExReleaseRundownProtection(&callbackEntry->RundownProtect);
		}
		nextEntry = nextEntry->Flink;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*ExDesktopObjectType + callbackListOffset);
	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
		if (ExAcquireRundownProtection(&callbackEntry->RundownProtect)) {
			LogInfo("PreOperation %p, PostOperation: %p\n", callbackEntry->PreOperation, callbackEntry->PostOperation);
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_CREATE))
				LogInfo("Protect handle from creating\n");
			if (FlagOn(callbackEntry->Operations, OB_OPERATION_HANDLE_DUPLICATE))
				LogInfo("Protect handle from duplicating\n");
			info[i].Type = ObjectCallbackType::Desktop;
			info[i].Operations = callbackEntry->Operations;
			info[i].CallbackEntry = callbackEntry;
			info[i].PostOperation = callbackEntry->PostOperation;
			info[i].PreOperation = callbackEntry->PreOperation;
			info[i].RegistrationHandle = callbackEntry->RegistrationHandle;
			info[i].Enabled = callbackEntry->Enabled;
			i++;
			ExReleaseRundownProtection(&callbackEntry->RundownProtect);
		}
		nextEntry = nextEntry->Flink;
	}

	return true;
}

LONG GetObCallbackCount(ULONG callbackListOffset) {
	PLIST_ENTRY callbackListHead = nullptr;
	PLIST_ENTRY nextEntry = nullptr;
	POB_CALLBACK_ENTRY callbackEntry = nullptr;
	volatile LONG count = 0;

	if (callbackListOffset == -1) {
		return count;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*PsThreadType + callbackListOffset);

	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
		InterlockedIncrement(&count);
		nextEntry = nextEntry->Flink;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*PsProcessType + callbackListOffset);

	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
		InterlockedIncrement(&count);
		nextEntry = nextEntry->Flink;
	}

	callbackListHead = (PLIST_ENTRY)((PUCHAR)*ExDesktopObjectType + callbackListOffset);

	nextEntry = callbackListHead->Flink;

	while (nextEntry != callbackListHead) {
		callbackEntry = CONTAINING_RECORD(nextEntry, OB_CALLBACK_ENTRY, CallbackList);
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
	status = g_sec.FreeSection();
	if (!NT_SUCCESS(status)) {
		LogDebug("free section failed 0x%x\n", status);
	}
#ifdef _WIN64
	status = g_secWow.FreeSection();
	if (!NT_SUCCESS(status)) {
		LogDebug("free wow section failed 0x%x\n", status);
	}
#endif // 

	PsTerminateSystemThread(status);
}

void RemoveFilter(_In_ PVOID context) {
	NTSTATUS status = STATUS_SUCCESS;
	PFLT_FILTER pFilter = (PFLT_FILTER)context;

	FltUnregisterFilter(pFilter);

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
			// PsSetCreateProcessNotifyRoutineEx2
			PPsSetCreateProcessNotifyRoutineEx2 pPsSetCreateProcessNotifyRoutineEx2 = nullptr;
			pPsSetCreateProcessNotifyRoutineEx2 = (PPsSetCreateProcessNotifyRoutineEx2)khook::GetApiAddress(L"PsSetCreateProcessNotifyRoutineEx2");
			if (nullptr != pPsSetCreateProcessNotifyRoutineEx2) {
				status = pPsSetCreateProcessNotifyRoutineEx2(0, notify->Address, TRUE);
			}
			else {
				status = STATUS_PROCEDURE_NOT_FOUND;
			}
			if (status == STATUS_PROCEDURE_NOT_FOUND||!NT_SUCCESS(status)) {
				status = PsSetCreateProcessNotifyRoutineEx(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>(notify->Address), TRUE);
			}
			if (status == STATUS_PROCEDURE_NOT_FOUND) {
				status = PsSetCreateProcessNotifyRoutine(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE>(notify->Address), TRUE);
			}
			break;
		}

		case NotifyType::CreateThreadNotify:
		{
			status = PsRemoveCreateThreadNotifyRoutine(reinterpret_cast<PCREATE_THREAD_NOTIFY_ROUTINE>(notify->Address));
			break;
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

bool RemoveObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry){
	if (!pCallbackEntry) {
		return false;
	}
	pCallbackEntry->RegistrationHandle->Count = 0;
	bool ret =  RemoveEntryList(&pCallbackEntry->CallbackList);
	if (!ret) {
		ObUnRegisterCallbacks(pCallbackEntry->RegistrationHandle);
	}
	return ret;
}

NTSTATUS EnumMiniFilterOperations(MiniFilterData* pData, OperationInfo* pInfo) {
	NTSTATUS status = STATUS_SUCCESS;

	WCHAR name[128] = { 0 };
	WCHAR filterName[128] = { 0 };
	if (pData->Length < sizeof(filterName) / sizeof(WCHAR)) {
		RtlCopyMemory(name, pData->Name, pData->Length * sizeof(WCHAR));
		name[pData->Length] = L'\0';
	}
	ULONG offset = pData->OperationsOffset;
	ULONG filtersCount = { 0 };
	ULONG size = 0;
	PFILTER_FULL_INFORMATION pFullInfo = nullptr;
	PFLT_FILTER* ppFltList = nullptr;
	status = FltEnumerateFilters(nullptr, 0, &filtersCount);
	if ((status == STATUS_BUFFER_TOO_SMALL) && filtersCount) {
		size = sizeof(PFLT_FILTER) * filtersCount;
		ppFltList = (PFLT_FILTER*)ExAllocatePoolWithTag(NonPagedPool, size, 'tsil');
		if (ppFltList) {
			status = FltEnumerateFilters(ppFltList, size, &filtersCount);
			for (decltype(filtersCount) i = 0; NT_SUCCESS(status) && (i < filtersCount); i++) {
				status = FltGetFilterInformation(ppFltList[i], FilterFullInformation, nullptr, 0, &size);
				if ((status == STATUS_BUFFER_TOO_SMALL) && size) {
					pFullInfo = (PFILTER_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, size, 'ofni');
					if (pFullInfo) {
						status = FltGetFilterInformation(ppFltList[i], FilterFullInformation, pFullInfo, size, &size);
						if (NT_SUCCESS(status)) {
							if (pFullInfo->FilterNameLength < sizeof(filterName)/sizeof(WCHAR)) {
								RtlCopyMemory(filterName, pFullInfo->FilterNameBuffer,
									pFullInfo->FilterNameLength);
								filterName[pFullInfo->FilterNameLength/sizeof(WCHAR)] = L'\0';
							}
							if (!_wcsicmp(filterName, name)) {
								PFLT_OPERATION_REGISTRATION* ppOperationReg = (PFLT_OPERATION_REGISTRATION*)((PUCHAR)ppFltList[i] + offset);
								PFLT_OPERATION_REGISTRATION pOperationReg = *ppOperationReg;
								int j = 0;
								for (j = 0; pOperationReg->MajorFunction != IRP_MJ_OPERATION_END; j++,pOperationReg++) {
									pInfo[j].FilterHandle = ppFltList[i];
									pInfo[j].Flags = pOperationReg->Flags;
									pInfo[j].MajorFunction = pOperationReg->MajorFunction;
									pInfo[j].PostOperation = pOperationReg->PostOperation;
									pInfo[j].PreOperation = pOperationReg->PreOperation;
								}
								pInfo[j].MajorFunction = pOperationReg->MajorFunction;
								break;
							}
						}
						ExFreePool(pFullInfo);
						pFullInfo = nullptr;
					}
				}
			}
			ExFreePoolWithTag(ppFltList, 'tsil');
			ppFltList = nullptr;
		}
	}
	if(pFullInfo!=nullptr)
		ExFreePool(pFullInfo);

	if(ppFltList!=nullptr)
		ExFreePoolWithTag(ppFltList, 'tsil');


	return status;
}



NTSTATUS RemoveMiniFilter(MiniFilterData* pData) {
	NTSTATUS status = STATUS_SUCCESS;

	WCHAR name[128] = { 0 };
	WCHAR filterName[128] = { 0 };
	if (pData->Length < sizeof(filterName) / sizeof(WCHAR)) {
		RtlCopyMemory(name, pData->Name, pData->Length * sizeof(WCHAR));
		name[pData->Length] = L'\0';
	}

	ULONG filtersCount = 0;
	ULONG size = 0;
	ULONG offset = pData->RundownRefOffset;
	PFILTER_FULL_INFORMATION pFullInfo = nullptr;
	PFLT_FILTER* ppFltList = nullptr;
	status = FltEnumerateFilters(nullptr, 0, &filtersCount);
	if ((status == STATUS_BUFFER_TOO_SMALL) && filtersCount) {
		size = sizeof(PFLT_FILTER) * filtersCount;
		ppFltList = (PFLT_FILTER*)ExAllocatePoolWithTag(NonPagedPool, size, 'tsil');
		if (ppFltList) {
			status = FltEnumerateFilters(ppFltList, size, &filtersCount);
			for (decltype(filtersCount) i = 0; NT_SUCCESS(status) && (i < filtersCount); i++) {
				status = FltGetFilterInformation(ppFltList[i], FilterFullInformation, nullptr, 0, &size);
				if ((status == STATUS_BUFFER_TOO_SMALL) && size) {
					pFullInfo = (PFILTER_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, size, 'ofni');
					if (pFullInfo) {
						status = FltGetFilterInformation(ppFltList[i], FilterFullInformation, pFullInfo, size,
							&size);
						if (NT_SUCCESS(status)) {
							if (pFullInfo->FilterNameLength < sizeof(filterName) / sizeof(WCHAR)) {
								RtlCopyMemory(filterName, pFullInfo->FilterNameBuffer, pFullInfo->FilterNameLength);
								filterName[pFullInfo->FilterNameLength / sizeof(WCHAR)] = L'\0';
							}
							if (!_wcsicmp(filterName, name)) {
								HANDLE hThread;
								status = PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, RemoveFilter, ppFltList[i]);
								if (!NT_SUCCESS(status)) {
									LogError("PsCreateSystemThread failed!\n");
									break;
								}
								status = ZwClose(hThread);
								break;
							}
						}
						ExFreePool(pFullInfo);
						pFullInfo = nullptr;
					}
				}
			}
			ExFreePoolWithTag(ppFltList, 'tsil');
			ppFltList = nullptr;
		}
	}
	if (pFullInfo != nullptr)
		ExFreePool(pFullInfo);

	if (ppFltList != nullptr)
		ExFreePoolWithTag(ppFltList, 'tsil');

	return status;
}

VOID PsCallImageNotifyRoutines(
	_In_ PUNICODE_STRING ImageName,
	_In_ HANDLE ProcessId,
	_Inout_ PIMAGE_INFO_EX ImageInfoEx,
	_In_ PVOID FileObject
) {
	PLOAD_IMAGE_NOTIFY_ROUTINE Rtn;
	ULONG i;

	PEX_CALLBACK_ROUTINE_BLOCK CallBack;

	KeEnterCriticalRegion();

	if (*g_pPspNotifyEnableMask & 0x1) {
		ImageInfoEx->Size = sizeof(IMAGE_INFO_EX);
		ImageInfoEx->ImageInfo.ExtendedInfoPresent = TRUE;
		ImageInfoEx->FileObject = (FILE_OBJECT*)FileObject;

#ifdef _WIN64
		static ULONG Max = 64;
#else
		static ULONG Max = 8;
#endif
		PEX_CALLBACK callback = g_pPspLoadImageNotifyRoutine;
		int j = 0;
		for (ULONG i = 0; i < Max; i++) {
			if (!MmIsAddressValid(callback))
				break;
			auto block = ExReferenceCallBackBlock(callback);
			if (block != nullptr) {
				Rtn = (PLOAD_IMAGE_NOTIFY_ROUTINE)block->Function;
				Rtn(ImageName, ProcessId, &ImageInfoEx->ImageInfo);
				ExDereferenceCallBackBlock(callback, block);
				++j;
			}
			callback++;
		}
	}

	KeLeaveCriticalRegion();
}

using PMinCryptIsFileRevoked = NTSTATUS(NTAPI*) (
	PVOID Arg0,
	PVOID Arg1,
	ULONG Len
	);

PMinCryptIsFileRevoked g_pMinCryptIsFileRevoked = nullptr;


NTSTATUS NTAPI HookMinCryptIsFileRevoked(PVOID Arg0, PVOID Arg1, ULONG Len) {
	LogInfo("Arg0: %p,Arg1: %p,Size: %d\n", Arg0, Arg1, Len);
	return STATUS_IMAGE_CERT_REVOKED;
}

bool DisableDriverLoad(CiSymbols* pSym) {
	g_pMinCryptIsFileRevoked = (PMinCryptIsFileRevoked)pSym->MinCryptIsFileRevoked;
	if (g_pMinCryptIsFileRevoked) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pMinCryptIsFileRevoked, HookMinCryptIsFileRevoked);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}
	return true;
}

bool EnableDriverLoad() {
	NTSTATUS status = DetourDetach((PVOID*)&g_pMinCryptIsFileRevoked, HookMinCryptIsFileRevoked);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;

	return true;
}

using PI_MinCryptHashSearchCompare = int(NTAPI*) (
	size_t Size,
	PUCHAR Arg1,
	PUCHAR Arg2
	);

PI_MinCryptHashSearchCompare g_pI_MinCryptHashSearchCompare = nullptr;

int NTAPI HookI_MinCryptHashSearchCompare(size_t Size, PUCHAR Arg1, PUCHAR Arg2) {
	do
	{
		LogInfo("Pid: %d\n", PsGetCurrentProcessId());
		PCHAR hash = (PCHAR)ExAllocatePool(PagedPool, Size * 2 + 1);
		if (hash == nullptr) {
			break;
		}
		hash[0] = '\0';
		CHAR hex[3];
		for (int i = 0; i < Size; i++) {
			sprintf_s(hex, "%02X", Arg1[i]);
			strcat(hash, hex);
		}
		LogInfo(hash);
		hash[0] = '\0';
		for (int i = 0; i < Size; i++) {
			sprintf_s(hex, "%02X", Arg2[i]);
			strcat(hash, hex);
		}
		LogInfo(hash);
		if (hash != nullptr) {
			ExFreePool(hash);
		}
	} while (FALSE);
	
	return memcmp(Arg1, Arg2, Size);
}

bool StartLogDriverHash(CiSymbols* pSym) {
	g_pI_MinCryptHashSearchCompare = (PI_MinCryptHashSearchCompare)pSym->I_MinCryptHashSearchCompare;
	if (g_pI_MinCryptHashSearchCompare) {
		NTSTATUS status = DetourAttach((PVOID*)&g_pI_MinCryptHashSearchCompare, HookI_MinCryptHashSearchCompare);
		if (!NT_SUCCESS(status))
			return false;
		status = DetourTransactionCommit();
		if (!NT_SUCCESS(status))
			return false;
	}
	return true;
}

bool StopLogDriverHash() {
	NTSTATUS status = DetourDetach((PVOID*)&g_pI_MinCryptHashSearchCompare, HookI_MinCryptHashSearchCompare);
	if (!NT_SUCCESS(status))
		return false;

	status = DetourTransactionCommit();
	if (!NT_SUCCESS(status))
		return false;
	
	return true;
}

void DisableObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry) {
	if (!pCallbackEntry)
		return;
	pCallbackEntry->Enabled = FALSE;
}

void EnableObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry) {
	if (!pCallbackEntry)
		return;
	pCallbackEntry->Enabled = TRUE;
}