#include "pch.h"
#include "kDbgCore.h"
#include "kDbgSys.h"
#include <ntimage.h>


// 调试对象类型
POBJECT_TYPE* DbgkDebugObjectType;

// 保护对EPROCESS的DebugPort的访问
FAST_MUTEX	DbgkpProcessDebugPortMutex;

PSYSTEM_DLL* PspSystemDlls;

NTSTATUS NtCreateDebugObject(
	_Out_ PHANDLE DebugObjectHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ ULONG Flags
) {
	NTSTATUS status;
	HANDLE handle;
	PDEBUG_OBJECT DebugObject;
	KPROCESSOR_MODE PreviousMode;

	PreviousMode = ExGetPreviousMode();
	
	// 判断用户层句柄地址是否合法
	__try {
		if (PreviousMode != KernelMode) {
			ProbeForWrite(DebugObjectHandle,sizeof(PHANDLE),TYPE_ALIGNMENT(PHANDLE));
		}
		*DebugObjectHandle = nullptr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return GetExceptionCode();
	}

	if (Flags & ~DEBUG_KILL_ON_CLOSE) {
		return STATUS_INVALID_PARAMETER;
	}

	// 创建调试对象
	status = ObCreateObject(
		PreviousMode,
		*DbgkDebugObjectType,
		ObjectAttributes,
		PreviousMode,
		nullptr,
		sizeof(DEBUG_OBJECT),
		0,
		0,
		(PVOID*)&DebugObject);

	if (!NT_SUCCESS(status)) {
		return status;
	}
	// 初始化调试对象
	ExInitializeFastMutex(&DebugObject->Mutex);
	InitializeListHead(&DebugObject->EventList);
	KeInitializeEvent(&DebugObject->EventsPresent, NotificationEvent, FALSE);
	
	if (Flags & DEBUG_KILL_ON_CLOSE) {
		DebugObject->Flags = DEBUG_OBJECT_KILL_ON_CLOSE;
	}
	else {
		DebugObject->Flags = 0;
	}

	// 调试对象插入句柄表
	status = ObInsertObject(
		DebugObject,
		nullptr,
		DesiredAccess,
		0,
		nullptr,
		&handle
	);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	__try {
		*DebugObjectHandle = handle;
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		status = GetExceptionCode();
	}

	return status;
}

NTSTATUS NtDebugActiveProcess(
	_In_ HANDLE ProcessHandle, 
	_In_ HANDLE DebugObjectHandle
){
	NTSTATUS status;
	KPROCESSOR_MODE PreviousMode;
	PDEBUG_OBJECT DebugObject;
	PEPROCESS Process, CurrentProcess;
	PETHREAD LastThread;

	PreviousMode = ExGetPreviousMode();
	// 得到被调试进程的eprocess
	status = ObReferenceObjectByHandle(ProcessHandle,
		PROCESS_SUSPEND_RESUME,
		*PsProcessType,
		PreviousMode,
		(PVOID*)&Process,
		nullptr);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	CurrentProcess = PsGetCurrentProcess();

	//判断被调试进程是否自己或者被调试进程是否PsInitialSystemProcess进程，是的话退出
	if (Process == CurrentProcess || Process == PsInitialSystemProcess) {
		ObDereferenceObject(Process);
		return STATUS_ACCESS_DENIED;
	}
	
	if (PreviousMode == UserMode) {
		ObDereferenceObject(Process);
		return STATUS_PROCESS_IS_PROTECTED;
	}

	// 得到调试句柄关联的调试对象DebugObject
	status = ObReferenceObjectByHandle(DebugObjectHandle,
		DEBUG_OBJECT_ADD_REMOVE_PROCESS,
		*DbgkDebugObjectType,
		PreviousMode,
		(PVOID*)&DebugObject,
		nullptr);

	if (NT_SUCCESS(status)) {
		// 避免进程退出，出错
		//if (ExAcquireRundownProtection(&Process->RundownProtect)) {
		//	// 发送虚假的进程创建消息
		//	status = DbgkpPostFakeProcessCreateMessages((PEPROCESS)Process, DebugObject, &LastThread);
		//	// 设置调试对象给被调试进程
		//	status = DbgkpSetProcessDebugObject((PEPROCESS)Process, DebugObject, status, LastThread);

		//	//ExReleaseRundownProtection(&Process->RundownProtect);
		//}
		//else {
		//	status = STATUS_PROCESS_IS_TERMINATING;
		//}
		
		ObDereferenceObject(DebugObject);
	}
	ObDereferenceObject(Process);

	return status;
}

NTSTATUS DbgkpPostFakeProcessCreateMessages(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ PETHREAD* pLastThread
) {
	NTSTATUS status;
	KAPC_STATE ApcState;
	PETHREAD StartThread, Thread;
	PETHREAD LastThread;

	// 收集所有线程创建的消息
	StartThread = nullptr;
	status = DbgkpPostFakeThreadMessages(
		Process,
		StartThread,
		DebugObject,
		&Thread,
		&LastThread
	);

	if (NT_SUCCESS(status)) {
		KeStackAttachProcess(Process, &ApcState);

		// 收集模块创建的消息
		DbgkpPostModuleMessages(Process, Thread, DebugObject);

		KeUnstackDetachProcess(&ApcState);

		ObDereferenceObject(Thread);
	}
	else {
		LastThread = nullptr;
	}

	*pLastThread = LastThread;
	return status;
}

VOID DbgkpFreeDebugEvent(
	_In_ PDEBUG_EVENT DebugEvent
) {
	NTSTATUS status;

	switch (DebugEvent->ApiMsg.ApiNumber){
	case DbgKmCreateProcessApi:
		if (DebugEvent->ApiMsg.u.CreateProcess.FileHandle != nullptr) {
			status = ObCloseHandle(DebugEvent->ApiMsg.u.CreateProcess.FileHandle, KernelMode);
		}
		break;
	case DbgKmCreateThreadApi:
		if (DebugEvent->ApiMsg.u.LoadDll.FileHandle != nullptr) {
			status = ObCloseHandle(DebugEvent->ApiMsg.u.LoadDll.FileHandle, KernelMode);
		}
		break;
	}
	ObDereferenceObject(DebugEvent->Process);
	ObDereferenceObject(DebugEvent->Thread);
	ExFreePool(DebugEvent);
}

NTSTATUS PsResumeThread(
	_In_ PETHREAD Thread,
	_Out_opt_ PULONG PreviousSuspendCount
) {
	if (PreviousSuspendCount)
		*PreviousSuspendCount = 0;
	return STATUS_SUCCESS;
}

VOID DbgkpWakeTarget(
	_In_ PDEBUG_EVENT DebugEvent
) {
	PETHREAD Thread;

	Thread = (PETHREAD)DebugEvent->Thread;

	if ((DebugEvent->Flags & DEBUG_EVENT_SUSPEND) != 0) {
		PsResumeThread(Thread, nullptr);
	}

	if (DebugEvent->Flags & DEBUG_EVENT_RELEASE) {
		//ExReleaseRundownProtection(&Thread->RundownProtect);
	}

	if ((DebugEvent->Flags & DEBUG_EVENT_NOWAIT) == 0) {
		// 唤醒等待的被调试线程
		KeSetEvent(&DebugEvent->ContinueEvent, 0, FALSE);
	}
	else {
		DbgkpFreeDebugEvent(DebugEvent);
	}
}

VOID DbgkpMarkProcessPeb(
	_In_ PEPROCESS Process
) {
	KAPC_STATE ApcState;
	PEPROCESS process = Process;
	//if (ExAcquireRundownProtection(&process->RundownProtect)) {
	//	/*if (process->Peb != nullptr) {
	//		KeStackAttachProcess(Process, &ApcState);
	//		ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);
	//		__try {
	//			auto peb = (PPEB_S)process->Peb;
	//			peb->BeingDebugged = (BOOLEAN)(process->DebugPort != nullptr ? TRUE : FALSE);
	//		}
	//		__except (EXCEPTION_EXECUTE_HANDLER) {

	//		}
	//		ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

	//		KeUnstackDetachProcess(&ApcState);
	//	}*/
	//	
	//	//ExReleaseRundownProtection(&process->RundownProtect);
	//}
}

NTSTATUS
DbgkpSetProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ NTSTATUS MsgStatus,
	_In_ PETHREAD LastThread
) {
	NTSTATUS status;
	PETHREAD ThisThread;
	LIST_ENTRY TempList;
	PLIST_ENTRY Entry;
	PDEBUG_EVENT DebugEvent;
	BOOLEAN First;
	PETHREAD Thread;
	BOOLEAN GlobalHeld;
	PETHREAD FirstThread;

	PAGED_CODE();

	ThisThread = PsGetCurrentThread();
	InitializeListHead(&TempList);

	First = TRUE;
	GlobalHeld = FALSE;

	if (!NT_SUCCESS(MsgStatus)) {
		LastThread = nullptr;
		status = MsgStatus;
	}
	else {
		status = STATUS_SUCCESS;
	}
	auto process = (PEPROCESS)Process;

	if (NT_SUCCESS(status)) {
		while (true) {
			GlobalHeld = TRUE;
			ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

			// 如果被调试进程的Debug Port已经设置，那么跳出循环
		/*	if (process->DebugPort != nullptr) {
				status = STATUS_PORT_ALREADY_SET;
				break;
			}*/

			//process->DebugPort = DebugObject;
			ObReferenceObject(LastThread);

			//Thread = (PETHREAD_S)PsGetNextProcessThread(Process, LastThread);
						
			//if (Thread != nullptr) {
			//	process->DebugPort = nullptr;
			//	ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
			//	GlobalHeld = FALSE;

			//	ObDereferenceObject(LastThread);
			//	// 通知线程创建消息
			//	status = DbgkpPostFakeThreadMessages(
			//		Process,
			//		(PETHREAD)Thread,
			//		DebugObject,
			//		&FirstThread,
			//		&LastThread
			//	);
			//	if (!NT_SUCCESS(status)) {
			//		LastThread = nullptr;
			//		break;
			//	}
			//	ObDereferenceObject(FirstThread);
			//}
			//else {
			//	break;
			//}
		}
	}

	ExAcquireFastMutex(&DebugObject->Mutex);

	if (NT_SUCCESS(status)) {
		//if ((DebugObject->Flags & DEBUG_OBJECT_DELETE_PENDING) == 0) {
		//	PS_SET_BITS(&Process->Flags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT | PS_PROCESS_FLAGS_CREATE_REPORTED);
		//	ObReferenceObject(DebugObject);
		//}
		//else {
		//	//process->DebugPort = nullptr;
		//	status = STATUS_DEBUGGER_INACTIVE;
		//}
	}

	// 遍历所有调试事件
	for (Entry = DebugObject->EventList.Flink; Entry != &DebugObject->EventList;) {
		// 取出调试事件
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		Entry = Entry->Flink;

		if ((DebugEvent->Flags & DEBUG_EVENT_INACTIVE) != 0 && DebugEvent->BackoutThread == ThisThread) {
			Thread = DebugEvent->Thread;
			if (NT_SUCCESS(status)) {
				/*if ((DebugEvent->Flags & DEBUG_EVENT_PROTECT_FAILED) != 0) {
					PS_SET_BITS(&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG);
					RemoveEntryList(&DebugEvent->EventList);
					InsertTailList(&TempList, &DebugEvent->EventList);
				}
				else {
					if (First) {
						DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
						KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);
						First = FALSE;
					}
					DebugEvent->BackoutThread = nullptr;
					PS_SET_BITS(&Thread->CrossThreadFlags, PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG);
				}*/
			}
			else {
				RemoveEntryList(&DebugEvent->EventList);
				InsertTailList(&TempList, &DebugEvent->EventList);
			}

			if (DebugEvent->Flags & DEBUG_EVENT_RELEASE) {
				DebugEvent->Flags &= ~DEBUG_EVENT_RELEASE;
				//ExReleaseRundownProtection(&Thread->RundownProtect);
			}
		}
	}

	ExReleaseFastMutex(&DebugObject->Mutex);

	if (GlobalHeld) {
		ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
	}

	if (LastThread != nullptr) {
		ObDereferenceObject(LastThread);
	}

	while (!IsListEmpty(&TempList)) {
		Entry = RemoveHeadList(&TempList);
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		DbgkpWakeTarget(DebugEvent);
	}

	if (NT_SUCCESS(status)) {
		DbgkpMarkProcessPeb(Process);
	}

	return status;
}

HANDLE DbgkpSectionToFileHandle(
	_In_ PVOID SectionObject
) {
	return nullptr;
}



NTSTATUS PsSuspendThread(
	_In_ PETHREAD Thread,
	_Out_opt_ PULONG PreviousSuspendCount
) {

	return STATUS_SUCCESS;
}

NTSTATUS KeResumeThread(
	_Inout_ PETHREAD Thread
) {
	return STATUS_SUCCESS;
}

VOID DbgkSendSystemDllMessages(
	PETHREAD Thread,
	PDEBUG_OBJECT DebugObject,
	PDBGKM_APIMSG ApiMsg
) {

}

NTSTATUS DbgkpPostFakeThreadMessages(
	PEPROCESS	Process,
	PETHREAD	StartThread,
	PDEBUG_OBJECT	DebugObject,
	PETHREAD* pFirstThread,
	PETHREAD* pLastThread
) {
	NTSTATUS status;
	PETHREAD Thread, FirstThread, LastThread, CurrentThread;
	DBGKM_APIMSG ApiMsg;
	BOOLEAN First = TRUE;
	BOOLEAN IsFirstThread;
	PIMAGE_NT_HEADERS NtHeaders;
	ULONG Flags;
	KAPC_STATE ApcState;
	auto process = Process;

	status = STATUS_UNSUCCESSFUL;

	LastThread = FirstThread = nullptr;

	CurrentThread = PsGetCurrentThread();

	if (StartThread == nullptr) {
		//StartThread = PsGetNextProcessThread(Process,nullptr);
		First = TRUE;
	}
	else {
		First = FALSE;
		FirstThread = StartThread;
		ObfReferenceObject(StartThread);
	}

	// 遍历调试进程的所有线程
	//for (Thread = (PETHREAD_S)StartThread;
	//	Thread != nullptr;
	//	Thread = (PETHREAD_S)PsGetNextProcessThread(Process, (PETHREAD)Thread))
	//	Flags = DEBUG_EVENT_NOWAIT;

	//	if (LastThread != nullptr) {
	//		ObfDereferenceObject(LastThread);
	//	}

	//	LastThread = Thread;
	//	ObfDereferenceObject(LastThread);

	//	if (Thread->ThreadInserted == 0) {
	//		// 涉及的内容太多
	//		continue;
	//	}

	//	if (ExAcquireRundownProtection(&Thread->RundownProtect)) {
	//		Flags |= DEBUG_EVENT_RELEASE;
	//		status = PsSuspendThread((PETHREAD)Thread, nullptr);
	//		if (NT_SUCCESS(status)) {
	//			Flags |= DEBUG_EVENT_SUSPEND;
	//		}
	//	}
	//	else {
	//		Flags |= DEBUG_EVENT_PROTECT_FAILED;
	//	}

	//	// 每次构造一个DBGKM_APIMSG结构
	//	memset(&ApiMsg, 0, sizeof(ApiMsg));

	//	if (First && (Flags & DEBUG_EVENT_PROTECT_FAILED) == 0) {
	//		// 进程的第一个线程
	//		IsFirstThread = TRUE;
	//		ApiMsg.ApiNumber = DbgKmCreateProcessApi;

	//		if (process->SectionObject) {
	//			ApiMsg.u.CreateProcess.FileHandle = DbgkpSectionToFileHandle(process->SectionObject);
	//		}
	//		else {
	//			ApiMsg.u.CreateProcess.FileHandle = nullptr;
	//		}
	//		ApiMsg.u.CreateProcess.BaseOfImage = process->SectionBaseAddress;

	//		KeStackAttachProcess(Process, &ApcState);

	//		__try {
	//			NtHeaders = RtlImageNtHeader(process->SectionBaseAddress);
	//			if (NtHeaders) {
	//				ApiMsg.u.CreateProcess.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
	//				ApiMsg.u.CreateProcess.InitialThread.StartAddress = nullptr;
	//				ApiMsg.u.CreateProcess.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
	//			}
	//		}
	//		__except (EXCEPTION_EXECUTE_HANDLER) {
	//			ApiMsg.u.CreateProcess.InitialThread.StartAddress = nullptr;
	//			ApiMsg.u.CreateProcess.DebugInfoFileOffset = 0;
	//			ApiMsg.u.CreateProcess.DebugInfoSize = 0;
	//		}

	//		KeUnstackDetachProcess(&ApcState);
	//	}
	//	else {
	//		IsFirstThread = FALSE;
	//		ApiMsg.ApiNumber = DbgKmCreateThreadApi;
	//		ApiMsg.u.CreateThread.StartAddress = Thread->StartAddress;
	//	}

	//	status = DbgkpQueueMessage(Process, (PETHREAD)Thread,
	//		&ApiMsg, Flags, DebugObject);

	//	if (!NT_SUCCESS(status)) {
	//		if (Flags & DEBUG_EVENT_SUSPEND) {
	//			KeResumeThread((PETHREAD)Thread);
	//		}

	//		if (Flags & DEBUG_EVENT_RELEASE) {
	//			ExReleaseRundownProtection(&Thread->RundownProtect);
	//		}

	//		if (ApiMsg.ApiNumber == DbgKmCreateProcessApi && ApiMsg.u.CreateProcess.FileHandle != nullptr) {
	//			ObCloseHandle(ApiMsg.u.CreateProcess.FileHandle, KernelMode);
	//		}

	//		ObfDereferenceObject(Thread);
	//		break;
	//	}
	//	else if (IsFirstThread) {
	//		First = FALSE;
	//		ObReferenceObject(Thread);
	//		FirstThread = Thread;

	//		DbgkSendSystemDllMessages((PETHREAD)Thread, DebugObject, &ApiMsg);
	//	}
	//}

	if (!NT_SUCCESS(status)) {
		if (FirstThread) {
			ObDereferenceObject(FirstThread);
		}
		if (LastThread != nullptr) {
			ObDereferenceObject(LastThread);
		}
	}
	else {
		if (FirstThread) {
			*pFirstThread = (PETHREAD)FirstThread;
			*pLastThread = (PETHREAD)LastThread;
		}
		else {
			if (LastThread != nullptr) {
				ObDereferenceObject(LastThread);
			}
			status = STATUS_UNSUCCESSFUL;
		}
	}

	return status;
}

NTSTATUS MmGetFileNameForAddress(
	_In_ PVOID ProcessVa,
	_Out_ PUNICODE_STRING FileName
) {
	return STATUS_SUCCESS;
}

NTSTATUS
DbgkpPostModuleMessages(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_In_ PDEBUG_OBJECT DebugObject) {
	
	auto process = Process;
	//PPEB Peb = process->Peb;
	PPEB_LDR_DATA Ldr = nullptr;
	PLIST_ENTRY LdrHead, LdrNext;
	PLDR_DATA_TABLE_ENTRY LdrEntry;
	ULONG i;
	OBJECT_ATTRIBUTES attr;
	UNICODE_STRING Name;
	PIMAGE_NT_HEADERS NtHeaders;
	NTSTATUS status;
	IO_STATUS_BLOCK ioStatus;
	DBGKM_APIMSG ApiMsg;

	/*if (Peb == nullptr) {
		return STATUS_SUCCESS;
	}*/

	__try {
		//Ldr = Peb->Ldr;

		LdrHead = &Ldr->InLoadOrderModuleList;
		ProbeForRead(LdrHead, sizeof(LIST_ENTRY), TYPE_ALIGNMENT(LIST_ENTRY));
		for (LdrNext = LdrHead->Flink, i = 0; LdrNext != LdrHead && i < 500; LdrNext = LdrNext->Flink,i++) {
			if (i > 1) {
				RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));

				LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
				ProbeForRead(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY), TYPE_ALIGNMENT(LDR_DATA_TABLE_ENTRY));
				
				ApiMsg.ApiNumber = DbgKmLoadDllApi;
				ApiMsg.u.LoadDll.BaseOfDll = LdrEntry->DllBase;

				ProbeForRead(ApiMsg.u.LoadDll.BaseOfDll, sizeof(IMAGE_DOS_HEADER), TYPE_ALIGNMENT(IMAGE_DOS_HEADER));

				NtHeaders = RtlImageNtHeader(ApiMsg.u.LoadDll.BaseOfDll);
				if (NtHeaders) {
					ApiMsg.u.LoadDll.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
					ApiMsg.u.LoadDll.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
				}
				status = MmGetFileNameForAddress(NtHeaders, &Name);
				if (NT_SUCCESS(status)) {
					InitializeObjectAttributes(&attr, &Name, OBJ_FORCE_ACCESS_CHECK | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
						nullptr, nullptr);
					status = ZwOpenFile(&ApiMsg.u.LoadDll.FileHandle,
						GENERIC_READ | SYNCHRONIZE,
						&attr,
						&ioStatus,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT);
					if (!NT_SUCCESS(status)) {
						ApiMsg.u.LoadDll.FileHandle = nullptr;
					}
					ExFreePool(Name.Buffer);
				}

				if (DebugObject) {
					status = DbgkpQueueMessage(
						Process, Thread, &ApiMsg, DEBUG_EVENT_NOWAIT, DebugObject
					);
				}
				else {
					DbgkpSendApiMessage(&ApiMsg, 0x3);
					status = STATUS_UNSUCCESSFUL;
				}

				if (!NT_SUCCESS(status) && ApiMsg.u.LoadDll.FileHandle != nullptr) {
					ObCloseHandle(ApiMsg.u.LoadDll.FileHandle, KernelMode);
				}
			}

			ProbeForRead(LdrNext, sizeof(LIST_ENTRY), TYPE_ALIGNMENT(LIST_ENTRY));
		}
		
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}

	return STATUS_SUCCESS;
}

VOID DbgkMapViewOfSection(
	_In_ PVOID SectionObject,
	_In_ PVOID BaseAddress,
	_In_ PEPROCESS Process
) {
	//PTEB	Teb;
	HANDLE	hFile = nullptr;
	DBGKM_APIMSG ApiMsg;
	PEPROCESS	CurrentProcess;
	PETHREAD	CurrentThread;
	PIMAGE_NT_HEADERS	pImageHeader;
	PEPROCESS process = Process;

	CurrentProcess = PsGetCurrentProcess();
	CurrentThread = PsGetCurrentThread();

	//if (ExGetPreviousMode() == KernelMode ||
	//	(CurrentThread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) ||
	//	process->DebugPort == nullptr) {
	//	return;
	//}

	/*if (CurrentThread->Tcb.ApcStateIndex != 0x1) {
		Teb = (PTEB)CurrentThread->Tcb.Teb;
	}
	else {
		Teb = nullptr;
	}*/

	/*if (Teb != nullptr && Process == (PEPROCESS)CurrentProcess) {
		if (!DbgkpSuppressDbgMsg(Teb)) {
			ApiMsg.u.LoadDll.NamePointer = Teb->NtTib.ArbitraryUserPointer;
		}
		else {
			return;
		}
	}
	else {
		ApiMsg.u.LoadDll.NamePointer = nullptr;
	}*/

	hFile = DbgkpSectionToFileHandle(SectionObject);
	ApiMsg.u.LoadDll.FileHandle = hFile;
	ApiMsg.u.LoadDll.BaseOfDll = BaseAddress;
	ApiMsg.u.LoadDll.DebugInfoFileOffset = 0;
	ApiMsg.u.LoadDll.DebugInfoSize = 0;

	__try{
		pImageHeader = RtlImageNtHeader(BaseAddress);
		if (pImageHeader != nullptr) {
			ApiMsg.u.LoadDll.DebugInfoFileOffset = pImageHeader->FileHeader.PointerToSymbolTable;
			ApiMsg.u.LoadDll.DebugInfoSize = pImageHeader->FileHeader.NumberOfSections;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		ApiMsg.u.LoadDll.DebugInfoFileOffset = 0;
		ApiMsg.u.LoadDll.DebugInfoSize = 0;
		ApiMsg.u.LoadDll.NamePointer = nullptr;
	}

	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmLoadDllApi, sizeof(DBGKM_LOAD_DLL));

	DbgkpSendApiMessage(&ApiMsg, 0x1);
	if (ApiMsg.u.LoadDll.FileHandle != nullptr) {
		ObCloseHandle(ApiMsg.u.LoadDll.FileHandle, KernelMode);
	}
}

VOID DbgkUnMapViewOfSection(
	_In_ PEPROCESS Process,
	_In_ PVOID BaseAddress
) {
	//PTEB	Teb;
	DBGKM_APIMSG ApiMsg;
	PEPROCESS	CurrentProcess;
	PETHREAD	CurrentThread;

	CurrentProcess = PsGetCurrentProcess();
	CurrentThread = PsGetCurrentThread();

	/*if (ExGetPreviousMode() == KernelMode ||
		(CurrentThread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG)) {
		return;
	}*/

	/*if (CurrentThread->Tcb.ApcStateIndex != 0x1) {
		Teb = (PTEB)CurrentThread->Tcb.Teb;
	}
	else {
		Teb = nullptr;
	}*/

	//if (Teb != nullptr && Process == (PEPROCESS)CurrentProcess) {
	//	if (!DbgkpSuppressDbgMsg(Teb)) {

	//	}
	//	else {
	//		return;
	//	}
	//}

	ApiMsg.u.UnloadDll.BaseAddress = BaseAddress;
	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmUnloadDllApi, sizeof(DBGKM_UNLOAD_DLL));
	DbgkpSendApiMessage(&ApiMsg, 0x1);
}

PEPROCESS PsGetCurrentProcessByThread(PETHREAD Thread) {
	//return Thread->Tcb.Process;
	return nullptr;
}

//PSYSTEM_DLL_INFO PsQuerySystemDllInfo(
//	ULONG index
//) {
//	PSYSTEM_DLL Dll;
//	Dll = PspSystemDlls[index];
//	if (Dll != nullptr && Dll->DllInfo.DllBase != nullptr) {
//		return &Dll->DllInfo;
//	}
//	
//	return nullptr;
//}

//PVOID ObFastReferenceObjectLocked(
//	_In_ PEX_FAST_REF FastRef
//) {
//	PVOID Object;
//	EX_FAST_REF OldRef;
//
//	return nullptr;
//}

VOID DbgkCreateThread(
	PETHREAD Thread
) {
	PVOID Port;
	DBGKM_APIMSG m;
	PDBGKM_CREATE_THREAD CreateThreadArgs;
	PDBGKM_CREATE_PROCESS CreateProcessArgs;
	PEPROCESS Process;
	PDBGKM_LOAD_DLL LoadDllArgs;
	NTSTATUS status;
	PIMAGE_NT_HEADERS NtHeaders = nullptr;
	//PTEB Teb;
	ULONG OldFlags = 0;
	PFILE_OBJECT FileObject;
	OBJECT_ATTRIBUTES ioStatus;
	PVOID Object;
	PETHREAD CurrentThread;

#if defined(_WIN64)
	PVOID Wow64Process = nullptr;
#endif

	PAGED_CODE();

	Process = PsGetCurrentProcessByThread(Thread);

#if defined(_WIN64)
	//Wow64Process = Process->WoW64Process;
#endif

	/*OldFlags = PS_TEST_SET_BITS(&Process->Flags, PS_PROCESS_FLAGS_CREATE_REPORTED | PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE);*/
	if ((OldFlags & PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE) == 0 && (PspNotifyEnableMask&1)) {
		IMAGE_INFO_EX ImageInfoEx;
		PUNICODE_STRING ImageName;
		POBJECT_NAME_INFORMATION FileNameInfo;

		//
		// notification of main .exe
		//
		ImageInfoEx.ImageInfo.Properties = 0;
		ImageInfoEx.ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
		//ImageInfoEx.ImageInfo.ImageBase = Process->SectionBaseAddress;
		ImageInfoEx.ImageInfo.ImageSize = 0;
		
		__try {
			//NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);
			if (NtHeaders) {
#ifdef _WIN64
				if (Wow64Process != nullptr) {
					ImageInfoEx.ImageInfo.ImageSize = DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER((PIMAGE_NT_HEADERS32)NtHeaders, SizeOfImage);
				}
				else {
#endif
					ImageInfoEx.ImageInfo.ImageSize = DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders, SizeOfImage);
#ifdef _WIN64
				}
#endif
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			ImageInfoEx.ImageInfo.ImageSize = 0;
		}

		ImageInfoEx.ImageInfo.ImageSelector = 0;
		ImageInfoEx.ImageInfo.ImageSectionNumber = 0;

		PsReferenceProcessFilePointer((PEPROCESS)Process, &FileObject);
		status = SeLocateProcessImageName((PEPROCESS)Process, &ImageName);
		if (!NT_SUCCESS(status)) {
			ImageName = nullptr;
		}

		//PsCallImageNotifyRoutines(ImageName, Process->UniqueProcessId, FileObject, &ImageInfoEx);
		if (ImageName) {
			//因为在SeLocateProcessImageName中为ImageName申请了内存，所以要在此处释放掉
			ExFreePool(ImageName);
		}
		
		ObDereferenceObject(FileObject);

		for (int i = 0; i < 6; i++) {
			PSYSTEM_DLL_INFO info = PsQuerySystemDllInfo(i);
			if (info != nullptr) {
				ImageInfoEx.ImageInfo.Properties = 0;
				ImageInfoEx.ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
				ImageInfoEx.ImageInfo.ImageBase = info->DllBase;
				ImageInfoEx.ImageInfo.ImageSize = 0;

				__try {
					NtHeaders = RtlImageNtHeader(info->DllBase);
					if (NtHeaders) {
						ImageInfoEx.ImageInfo.ImageSize = DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders, SizeOfImage);
					}
				}
				__except(EXCEPTION_EXECUTE_HANDLER) {
					ImageInfoEx.ImageInfo.ImageSize = 0;
				}

				ImageInfoEx.ImageInfo.ImageSelector = 0;
				ImageInfoEx.ImageInfo.ImageSectionNumber = 0;

				PSYSTEM_DLL sysDll = (PSYSTEM_DLL)((char*)info - 0x10);
				Object = ObFastReferenceObject(&sysDll->FastRef);
				if (Object == nullptr) {
					KeEnterCriticalRegion();
					Object = ObFastReferenceObjectLocked(&sysDll->FastRef);
					//ExAcquirePushLockShared(&sysDll->Lock);
					KeLeaveCriticalRegion();
				}

				FileObject = CcGetFileObjectFromSectionPtrs((PSECTION_OBJECT_POINTERS)Object);
				if (FileObject != nullptr) {
					ObFastDereferenceObject(&sysDll->FastRef, Object);
				}
				/*PsCallImageNotifyRoutines(&sysDll->DllInfo.FileName,
					Process->UniqueProcessId,
					FileObject,
					&ImageInfoEx);*/

				ObDereferenceObject(FileObject);
			}
		}

		// 检测新创建的线程所在的进程是否正在被调试
		/*PDEBUG_OBJECT DebugObject = (PDEBUG_OBJECT)Process->DebugPort;
		if (DebugObject == nullptr) {
			return;
		}*/
		// 判断是否调用过调试函数
		if ((OldFlags & PS_PROCESS_FLAGS_CREATE_REPORTED) == 0) {
			CreateThreadArgs = &m.u.CreateProcess.InitialThread;
			CreateThreadArgs->SubSystemKey = 0;
			CreateThreadArgs->StartAddress = nullptr;

			CreateProcessArgs = &m.u.CreateProcess;
			CreateProcessArgs->SubSystemKey = 0;
		/*	CreateProcessArgs->FileHandle = DbgkpSectionToFileHandle(
				Process->SectionObject
			);*/
			//CreateProcessArgs->BaseOfImage = Process->SectionBaseAddress;
			CreateProcessArgs->DebugInfoFileOffset = 0;
			CreateProcessArgs->DebugInfoSize = 0;

			__try {
				//NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);
				if (NtHeaders) {
					CreateThreadArgs->StartAddress = (PVOID)(DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders, AddressOfEntryPoint)+
						DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders,ImageBase));
					CreateProcessArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
					CreateProcessArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				CreateThreadArgs->StartAddress = nullptr;
				CreateProcessArgs->DebugInfoFileOffset = 0;
				CreateProcessArgs->DebugInfoSize = 0;
			}

			DBGKM_FORMAT_API_MSG(m, DbgKmCreateProcessApi, sizeof(*CreateProcessArgs));

			DbgkpSendApiMessage(&m, FALSE);
			if (CreateProcessArgs->FileHandle != nullptr) {
				ObCloseHandle(CreateProcessArgs->FileHandle, KernelMode);
			}

			DbgkSendSystemDllMessages(nullptr, nullptr, &m);
		}
		else {
			CreateThreadArgs = &m.u.CreateThread;
			CreateThreadArgs->SubSystemKey = 0;
			//CreateThreadArgs->StartAddress = Thread->Win32Thread;

			DBGKM_FORMAT_API_MSG(m, DbgKmCreateThreadApi, sizeof(*CreateThreadArgs));

			DbgkpSendApiMessage(&m, TRUE);
		}

		/*if (Thread->ClonedThread == TRUE) {
			DbgkpPostModuleMessages(
				(PEPROCESS)Process,
				Thread,
				nullptr
			);
		}*/
	}
	
}

NTSTATUS
DbgkpQueueMessage(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_Inout_ PDBGKM_APIMSG ApiMsg,
	_In_ ULONG Flags,
	_In_ PDEBUG_OBJECT TargetDebugObject
) {
	PDEBUG_EVENT DebugEvent;
	DEBUG_EVENT StaticDebugEvent;
	PDEBUG_OBJECT DebugObject = nullptr;
	NTSTATUS status;

	auto process = Process;
	auto thread = Thread;
	if (Flags & DEBUG_EVENT_NOWAIT) {
		// POOL_QUOTA_FAIL_INSTEAD_OF_RAISE
		DebugEvent = (PDEBUG_EVENT)ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(*DebugEvent), 'EgbD');
		if (DebugEvent == nullptr) {
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		DebugEvent->Flags |= DEBUG_EVENT_INACTIVE;
		ObReferenceObject(Process);
		ObReferenceObject(Thread);
		DebugEvent->BackoutThread = PsGetCurrentThread();
		DebugObject = TargetDebugObject;
	}
	else {
		DebugEvent = &StaticDebugEvent;
		DebugEvent->Flags = Flags;
		ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

		//DebugObject = (PDEBUG_OBJECT)process->DebugPort;
		
		if (ApiMsg->ApiNumber == DbgKmCreateProcessApi ||
			ApiMsg->ApiNumber == DbgKmCreateProcessApi) {
			/*if (thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG) {
				DebugObject = nullptr;
			}*/
		}

		/*if (ApiMsg->ApiNumber == DbgKmLoadDllApi &&
			thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG &&
			Flags & 0x40) {
			DebugObject = nullptr;
		}*/

		if (ApiMsg->ApiNumber == DbgKmExitThreadApi ||
			ApiMsg->ApiNumber == DbgKmExitProcessApi) {
		/*	if (thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG) {
				DebugObject = nullptr;
			}*/
		}

		KeInitializeEvent(&DebugEvent->ContinueEvent, SynchronizationEvent, FALSE);
	}

	DebugEvent->Process = Process;
	DebugEvent->Thread = Thread;
	DebugEvent->ApiMsg = *ApiMsg;
	//DebugEvent->ClientId = thread->Cid;

	if (DebugObject == nullptr) {
		status = STATUS_PORT_NOT_SET;
	}
	else {
		ExAcquireFastMutex(&DebugObject->Mutex);

		if ((DebugObject->Flags & DEBUG_KILL_ON_CLOSE) == 0) {
			InsertTailList(&DebugObject->EventList, &DebugEvent->EventList);

			if ((Flags & DEBUG_EVENT_NOWAIT) == 0) {
				// 通知调试器有消息要读取
				KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);
			}
			status = STATUS_SUCCESS;
		}
		else {
			status = STATUS_DEBUGGER_INACTIVE;
		}

		ExReleaseFastMutex(&DebugObject->Mutex);
	}

	if ((Flags & DEBUG_EVENT_NOWAIT) == 0) {
		ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
		if (NT_SUCCESS(status)) {
			// 等待调试器的回复
			KeWaitForSingleObject(
				&DebugEvent->ContinueEvent,
				Executive,
				KernelMode,
				FALSE,
				nullptr
			);
			status = DebugEvent->Status;
			*ApiMsg = DebugEvent->ApiMsg;
		}
	}
	else {
		if (!NT_SUCCESS(status)) {
			ObDereferenceObject(Process);
			ObDereferenceObject(Thread);
			ExFreePool(DebugEvent);
		}
	}

	return status;
}

NTSTATUS NtWaitForDebugEvent(
	_In_ HANDLE DebugObjectHandle,
	_In_ BOOLEAN Alertable,
	_In_opt_ PLARGE_INTEGER Timeout,
	_Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
) {
	NTSTATUS status;
	KPROCESSOR_MODE PreviousMode;
	PDEBUG_OBJECT DebugObject;
	LARGE_INTEGER StartTime = { 0 };
	DBGUI_WAIT_STATE_CHANGE waitState;
	PEPROCESS Process;
	PETHREAD Thread;
	
	PreviousMode = ExGetPreviousMode();

	__try {
		if (ARGUMENT_PRESENT(Timeout)) {
			if (PreviousMode != KernelMode) {
				ProbeForRead(Timeout, sizeof(*Timeout), TYPE_ALIGNMENT(LARGE_INTEGER));
			}
			KeQuerySystemTime(&StartTime);
		}
		if (PreviousMode != KernelMode) {
			ProbeForWrite(WaitStateChange, sizeof(*WaitStateChange),TYPE_ALIGNMENT(DBGUI_WAIT_STATE_CHANGE));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return GetExceptionCode();
	}

	status = ObReferenceObjectByHandle(DebugObjectHandle,
		DEBUG_OBJECT_WAIT_STATE_CHANGE,
		*DbgkDebugObjectType,
		PreviousMode,
		(PVOID*)&DebugObject,
		nullptr);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	Process = nullptr;
	Thread = nullptr;

	while (true) {
		// 在调试对象有事件时产生
		status = KeWaitForSingleObject(&DebugObject->EventsPresent,
			Executive, PreviousMode, Alertable, Timeout);
		if (!NT_SUCCESS(status)||status == STATUS_TIMEOUT
			||status == STATUS_ALERTED||status==STATUS_USER_APC) {
			break;
		}

		
	}
}

NTSTATUS NtDebugContinue(
	_In_ HANDLE DebugObjectHandle,
	_In_ PCLIENT_ID AppClientId,
	_In_ NTSTATUS ContinueStatus
) {
	NTSTATUS status;
	PDEBUG_OBJECT DebugObject;
	PDEBUG_EVENT DebugEvent, FoundDebugEvent;
	KPROCESSOR_MODE PreviousMode;
	PLIST_ENTRY Entry;
	CLIENT_ID ClientId;
	BOOLEAN GetEvent;

	PreviousMode = ExGetPreviousMode();

	__try {
		if (PreviousMode != KernelMode) {
			ClientId = *AppClientId;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return GetExceptionCode();
	}

	switch (ContinueStatus)
	{
	case DBG_EXCEPTION_HANDLED:
	case DBG_CONTINUE:
	case DBG_TERMINATE_PROCESS:
	case DBG_TERMINATE_THREAD:
	case DBG_EXCEPTION_NOT_HANDLED:
		break;
	default:
		return STATUS_INVALID_PARAMETER;
	}

	status = ObReferenceObjectByHandle(DebugObjectHandle,
		DEBUG_OBJECT_WAIT_STATE_CHANGE,
		*DbgkDebugObjectType,
		PreviousMode,
		(PVOID*)&DebugObject,
		nullptr);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	GetEvent = FALSE;
	FoundDebugEvent = nullptr;

	ExAcquireFastMutex(&DebugObject->Mutex);
	for (Entry = DebugObject->EventList.Flink;
		Entry != &DebugObject->EventList;
		Entry = Entry->Flink) {
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);

		if (DebugEvent->ClientId.UniqueProcess == AppClientId->UniqueProcess) {
			if (!GetEvent) {
				if (DebugEvent->ClientId.UniqueThread == ClientId.UniqueThread &&
					(DebugEvent->Flags & DEBUG_EVENT_READ) != 0) {
					RemoveEntryList(Entry);
					FoundDebugEvent = DebugEvent;
					GetEvent = TRUE;
				}
			}
			else {
				DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
				KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);
				break;
			}
		}
	}

	ExReleaseFastMutex(&DebugObject->Mutex);
	ObReferenceObject(DebugObject);

	if (GetEvent) {
		FoundDebugEvent->ApiMsg.ReturnedStatus = ContinueStatus;
		FoundDebugEvent->Status = STATUS_SUCCESS;
		DbgkpWakeTarget(FoundDebugEvent);
	}
	else {
		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}

NTSTATUS NtRemoveProcessDebug(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
) {
	NTSTATUS status;
	PEPROCESS Process, CurrentProcess;
	KPROCESSOR_MODE PreviousMode;
	PDEBUG_OBJECT DebugObject;

	PreviousMode = ExGetPreviousMode();

	status = ObReferenceObjectByHandle(
		ProcessHandle,
		PROCESS_SUSPEND_RESUME,
		*PsProcessType,
		PreviousMode,
		(PVOID*)&Process,
		nullptr
	);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	if (PreviousMode == UserMode) {
		CurrentProcess = PsGetCurrentProcess();
	}

	status = ObReferenceObjectByHandle(DebugObjectHandle,
		DEBUG_OBJECT_KILL_ON_CLOSE,
		*DbgkDebugObjectType,
		PreviousMode,
		(PVOID*)&DebugObject,
		nullptr);
	if (!NT_SUCCESS(status)) {
		ObDereferenceObject(Process);
		return status;
	}

	status = DbgkClearProcessDebugObject((PEPROCESS)Process, DebugObject);

	ObDereferenceObject(DebugObject);
	ObDereferenceObject(Process);
	return status;
}

BOOLEAN
DbgkpSuspendProcess(
) {

	return FALSE;
}

VOID DbgkpResumeProcess(
	_In_ PEPROCESS Process
) {
	// PsThawProcess
	KeLeaveCriticalRegion();
}

NTSTATUS DbgkpSendApiMessage(
	PDBGKM_APIMSG ApiMsg,
	ULONG	Flag
) {
	NTSTATUS status;
	BOOLEAN isSuspend;
	PEPROCESS Process;
	PETHREAD Thread;

	isSuspend = FALSE;

	if (Flag & 0x1) {
		isSuspend = DbgkpSuspendProcess();
	}

	Thread = PsGetCurrentThread();
	Process = PsGetCurrentProcess();

	ApiMsg->ReturnedStatus = STATUS_PENDING;
	status = DbgkpQueueMessage(
		(PEPROCESS)Process,
		(PETHREAD)Thread,
		ApiMsg,
		((Flag & 0x2) << 0x5),
		nullptr
	);
	ZwFlushInstructionCache();
	if (isSuspend) {
		// PsThawProcess()
	}

	return status;
}

LOGICAL ExFastRefDereference(
	_Inout_ PEX_FAST_REF FastRef,
	_In_ PVOID Object
) {
	EX_FAST_REF OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);
		if ((OldRef.Value ^ (ULONG_PTR)Object) >= MAX_FAST_REFS) {
			return FALSE;
		}

		NewRef.Value = OldRef.Value + 1;
		NewRef.Object = InterlockedCompareExchangePointerRelease(&FastRef->Object, NewRef.Object, OldRef.Object);

		if (NewRef.Object != OldRef.Object) {
			continue;
		}
		break;
	}
}

EX_FAST_REF
ExFastReference(
	_Inout_ PEX_FAST_REF FastRef
) {
	EX_FAST_REF OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);
		if (OldRef.RefCnt != 0) {
			NewRef.Value = OldRef.Value - 1;
			NewRef.Object = InterlockedCompareExchangePointerRelease(&FastRef->Object, NewRef.Object, OldRef.Object);
			if (NewRef.Object != OldRef.Object) {
				continue;
			}
		}
		break;
	}
	return OldRef;
}

LOGICAL
ExFastRefAddAdditionalReferenceCounts(
	_Inout_ PEX_FAST_REF FastRef,
	_In_ PVOID Object,
	_In_ ULONG RefsToAdd
) {
	EX_FAST_REF OldRef, NewRef;

	while (true) {
		OldRef = ReadForWriteAccess(FastRef);

		if (OldRef.RefCnt + RefsToAdd > MAX_FAST_REFS ||
			(ULONG_PTR)Object != (OldRef.Value & ~MAX_FAST_REFS)) {
			return FALSE;
		}

		NewRef.Value = OldRef.Value + RefsToAdd;
		NewRef.Object = InterlockedCompareExchangePointerAcquire(&FastRef->Object,
			NewRef.Object,
			OldRef.Object);
		if (NewRef.Object != OldRef.Object) {
			continue;
		}
		break;
	}
	return TRUE;
}

PVOID ObFastReferenceObject(
	_In_ PEX_FAST_REF FastRef
) {
	EX_FAST_REF OldRef;
	PVOID Object;
	ULONG RefsToAdd, Unused;

	OldRef = ExFastReference(FastRef);

	Object = (PVOID)(OldRef.Value & (~MAX_FAST_REFS));

	Unused = OldRef.RefCnt;

	if (Unused <= 1) {
		if (Unused == 0) {
			return nullptr;
		}

		RefsToAdd = MAX_FAST_REFS;
		ObfReferenceObject(Object);

		if (!ExFastRefAddAdditionalReferenceCounts(FastRef, Object, RefsToAdd)) {
			ObfDereferenceObject(Object);
		}
	}

	return Object;
}

VOID DbgkExitThread(
	NTSTATUS ExitStatus
) {
	DBGKM_APIMSG ApiMsg;
	PEPROCESS Process;
	PETHREAD CurrentThread;

	CurrentThread = PsGetCurrentThread();
	Process = PsGetCurrentProcess();

	/*if (!(CurrentThread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) &&
		Process->DebugPort != nullptr && CurrentThread->ThreadInserted == TRUE) {
		ApiMsg.u.ExitThread.ExitStatus = ExitStatus;
		DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmExitThreadApi, sizeof(DBGKM_EXIT_THREAD));
		DbgkpSendApiMessage(&ApiMsg, 0x1);
	}*/
}

VOID DbgkExitProcess(
	NTSTATUS ExitStatus
) {
	DBGKM_APIMSG ApiMsg;
	PEPROCESS Process;
	PETHREAD CurrentThread;

	CurrentThread = PsGetCurrentThread();
	Process = PsGetCurrentProcess();

	/*if (!(CurrentThread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) &&
		Process->DebugPort != nullptr && CurrentThread->ThreadInserted == TRUE) {
		KeQuerySystemTime(&Process->ExitTime);

		ApiMsg.u.ExitProcess.ExitStatus = ExitStatus;
		DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmExitProcessApi, sizeof(DBGKM_EXIT_PROCESS));
		DbgkpSendApiMessage(&ApiMsg, FALSE);
	}*/
}

PVOID PsCaptureExceptionPort(
	_In_ PEPROCESS Process
) {

	return nullptr;
}

NTSTATUS DbgkpSendApiMessageLpc(
	_Inout_ PDBGKM_APIMSG ApiMsg,
	_In_ PVOID Port,
	_In_ BOOLEAN SuspendProcess
) {
	return STATUS_SUCCESS;
}

BOOLEAN DbgkForwardException(
	_In_ PEXCEPTION_RECORD ExceptionRecord,
	_In_ BOOLEAN DebugException,
	_In_ BOOLEAN SecondChance
) {
	NTSTATUS status;

	PEPROCESS		Process;
	PVOID			ExceptionPort = nullptr;
	PDEBUG_OBJECT	DebugObject = nullptr;
	BOOLEAN			bLpcPort = FALSE;

	DBGKM_APIMSG m;
	PDBGKM_EXCEPTION args;

	args = &m.u.Exception;
	DBGKM_FORMAT_API_MSG(m, DbgKmExceptionApi, sizeof(*args));

	if (DebugException == TRUE) {
		PETHREAD CurrentThread = PsGetCurrentThread();
		//if (CurrentThread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
		//	DebugObject = nullptr;
		//}
		//else {
		//	// DebugObject = (PDEBUG_OBJECT)Process->DebugPort;
		//}
	}
	else {
		//ExceptionPort = PsCaptureExceptionPort((PEPROCESS)Process);
		m.h.u2.ZeroInit = LPC_EXCEPTION;
		bLpcPort = TRUE;
	}

	if ((ExceptionPort == nullptr && DebugObject == nullptr) && DebugException == TRUE) {
		return FALSE;
	}

	args->ExceptionRecord = *ExceptionRecord;
	args->FirstChance = !SecondChance;

	if (bLpcPort == FALSE) {
		status = DbgkpSendApiMessage(&m, DebugException);
	}
	else if (ExceptionPort) {
		status = DbgkpSendApiMessageLpc(&m, ExceptionPort, DebugException);
		ObDereferenceObject(ExceptionPort);
	}
	else {
		m.ReturnedStatus = DBG_EXCEPTION_NOT_HANDLED;
		status = STATUS_SUCCESS;
	}

	if (NT_SUCCESS(status)) {
		status = m.ReturnedStatus;
		if (m.ReturnedStatus == DBG_EXCEPTION_NOT_HANDLED) {
			if (DebugException == TRUE) {
				return FALSE;
			}

			status = DbgkpSendErrorMessage(ExceptionRecord, &m);
		}
	}

	return NT_SUCCESS(status);
}

VOID DbgkpConvertKernelToUserStateChange(
	PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
	PDEBUG_EVENT DebugEvent
) {
	WaitStateChange->AppClientId = DebugEvent->ClientId;
	switch (DebugEvent->ApiMsg.ApiNumber){
	case DbgKmExceptionApi:
		switch (DebugEvent->ApiMsg.u.Exception.ExceptionRecord.ExceptionCode) {
		case STATUS_BREAKPOINT:
			WaitStateChange->NewState = DbgBreakpointStateChange;
			break;;
		case STATUS_SINGLE_STEP:
			WaitStateChange->NewState = DbgSingleStepStateChange;
			break;
		default:
			WaitStateChange->NewState = DbgExceptionStateChange;
			break;
		}
		WaitStateChange->StateInfo.Exception = DebugEvent->ApiMsg.u.Exception;
		break;
	
	case DbgKmCreateThreadApi:
		WaitStateChange->NewState = DbgCreateThreadStateChange;
		WaitStateChange->StateInfo.CreateThread.NewThread = DebugEvent->ApiMsg.u.CreateThread;
		break;

	case DbgKmCreateProcessApi:
		WaitStateChange->NewState = DbgCreateProcessStateChange;
		WaitStateChange->StateInfo.CreateProcessInfo.NewProcess = DebugEvent->ApiMsg.u.CreateProcess;
		DebugEvent->ApiMsg.u.CreateProcess.FileHandle = nullptr;
		break;

	case DbgKmExitThreadApi:
		WaitStateChange->NewState = DbgExitThreadStateChange;
		WaitStateChange->StateInfo.ExitThread = DebugEvent->ApiMsg.u.ExitThread;
		break;

	case DbgKmExitProcessApi:
		WaitStateChange->NewState = DbgExitProcessStateChange;
		WaitStateChange->StateInfo.ExitProcess = DebugEvent->ApiMsg.u.ExitProcess;
		break;

	case DbgKmLoadDllApi:
		WaitStateChange->NewState = DbgLoadDllStateChange;
		WaitStateChange->StateInfo.LoadDll = DebugEvent->ApiMsg.u.LoadDll;
		DebugEvent->ApiMsg.u.LoadDll.FileHandle = NULL;
		break;

	case DbgKmUnloadDllApi:
		WaitStateChange->NewState = DbgUnloadDllStateChange;
		WaitStateChange->StateInfo.UnloadDll = DebugEvent->ApiMsg.u.UnloadDll;
		break;

	default:
		ASSERT(FALSE);
	}
}

NTSTATUS DbgkClearProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT SourceDebugObject
) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEBUG_OBJECT DebugObject = nullptr;
	PDEBUG_EVENT DebugEvent;
	LIST_ENTRY TempList;
	PLIST_ENTRY Entry;

	ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

	auto process = Process;
	/*DebugObject = (PDEBUG_OBJECT)process->DebugPort;
	if (DebugObject == nullptr || (DebugObject != SourceDebugObject && SourceDebugObject != nullptr)) {
		DebugObject = nullptr;
		status = STATUS_PORT_NOT_SET;
	}
	else {
		process->DebugPort = nullptr;
		status = STATUS_SUCCESS;
	}*/
	ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

	if (NT_SUCCESS(status)) {
		DbgkpMarkProcessPeb(Process);
	}

	if (DebugObject) {
		InitializeListHead(&TempList);

		ExAcquireFastMutex(&DebugObject->Mutex);
		for (Entry = DebugObject->EventList.Flink; Entry != &DebugObject->EventList;) {
			DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
			Entry = Entry->Flink;
			// 一个调试器可以同时调试多个被调试进程
			if (DebugEvent->Process == Process) {
				RemoveEntryList(&DebugEvent->EventList);
				InsertTailList(&TempList, &DebugEvent->EventList);
			}
		}
		ExReleaseFastMutex(&DebugObject->Mutex);

		ObDereferenceObject(DebugObject);

		while (!IsListEmpty(&TempList)) {
			Entry = RemoveHeadList(&TempList);
			DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
			DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
			DbgkpWakeTarget(DebugEvent);
		}
	}
	return status;
}

NTSTATUS NtSetInformationDebugObject(
	_In_ HANDLE DebugObjectHandle,
	_In_ DEBUG_OBJECT_INFORMATION_CLASS DebugObjectInformationClass,
	_In_ PVOID DebugInformation,
	_In_ ULONG DebugInformationLength,
	_Out_opt_ PULONG ReturnLength
) {
	KPROCESSOR_MODE PreviousMode;
	ULONG Flags;
	NTSTATUS status;
	PDEBUG_OBJECT DebugObject;

	PreviousMode = ExGetPreviousMode();

	__try {
		if (PreviousMode != KernelMode) {
			if (DebugInformationLength) {
				ProbeForRead(DebugInformation, DebugInformationLength,
					sizeof(ULONG));
				if (ARGUMENT_PRESENT(ReturnLength)) {
					ProbeForWriteUlong(ReturnLength);
				}
			}
		}
		if (ARGUMENT_PRESENT(ReturnLength)) {
			*ReturnLength = 0;
		}

		switch (DebugObjectInformationClass) {
			case DebugObjectFlagsInformation: 
			{
				if (DebugInformationLength != sizeof(ULONG)) {
					if (ARGUMENT_PRESENT(ReturnLength)) {
						*ReturnLength = sizeof(ULONG);
					}
					return STATUS_INFO_LENGTH_MISMATCH;
				}
				Flags = *(PULONG)DebugInformation;

				break;
			}
			default:
				return STATUS_INVALID_PARAMETER;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return GetExceptionCode();
	}
	
	switch (DebugObjectInformationClass)
	{
		case DebugObjectFlagsInformation: 
		{
			if (Flags & ~DEBUG_KILL_ON_CLOSE) {
				return STATUS_INVALID_PARAMETER;
			}
			status = ObReferenceObjectByHandle(DebugObjectHandle,
				DEBUG_OBJECT_SET_INFORMATION,
				*DbgkDebugObjectType, 
				PreviousMode,
				(PVOID*)&DebugObject,
				nullptr);
			if (!NT_SUCCESS(status)) {
				return status;
			}
			ExAcquireFastMutex(&DebugObject->Mutex);

			if (Flags & DEBUG_KILL_ON_CLOSE) {
				DebugObject->Flags |= DEBUG_OBJECT_KILL_ON_CLOSE;
			}else {
				DebugObject->Flags &= ~DEBUG_OBJECT_KILL_ON_CLOSE;
			}
			ExReleaseFastMutex(&DebugObject->Mutex);
			ObDereferenceObject(DebugObject);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS NtQueryDebugFilterState(
	_In_ ULONG ComponentId,
	_In_ ULONG Level
) {
	

	return STATUS_SUCCESS;
}

NTSTATUS NtSetDebugFilterState(
	_In_ ULONG ComponentId,
	_In_ ULONG Level,
	_In_ BOOLEAN State
) {
	return STATUS_SUCCESS;
}

NTSTATUS DbgkOpenProcessObject(
	_In_ PEPROCESS Process,
	_In_ PACCESS_STATE AccessState,
	_In_ ACCESS_MASK DesiredAccess
) {
	return STATUS_SUCCESS;
}

NTSTATUS DbgkCopyProcessDebugPort(
	_In_ PEPROCESS TargetProcess,
	_In_ PEPROCESS SourceProcess,
	_In_ PDEBUG_OBJECT DebugObject,
	_Out_ PBOOLEAN bFlag
) {
	return STATUS_SUCCESS;
}

VOID DbgkpOpenHandles(
	_In_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread
) {
	NTSTATUS status;
	PEPROCESS CurrentProcess = nullptr;
	HANDLE OldHandle;

	switch (WaitStateChange->NewState)
	{
	case DbgCreateThreadStateChange:
		status = ObOpenObjectByPointer(Thread, 0, nullptr, THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | \
			THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION | THREAD_TERMINATE |
			READ_CONTROL | SYNCHRONIZE, *PsThreadType, KernelMode, &WaitStateChange->StateInfo.CreateThread.HandleToThread);
		if (!NT_SUCCESS(status)) {
			WaitStateChange->StateInfo.CreateThread.HandleToThread = nullptr;
		}
		break;

	case DbgCreateProcessStateChange:
		status = ObOpenObjectByPointer(Thread, 0, nullptr, THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | \
			THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION | THREAD_TERMINATE |
			READ_CONTROL | SYNCHRONIZE, *PsThreadType, KernelMode, &WaitStateChange->StateInfo.CreateThread.HandleToThread);
		if (!NT_SUCCESS(status)) {
			WaitStateChange->StateInfo.CreateThread.HandleToThread = nullptr;
		}
		status = ObOpenObjectByPointer(Process, 0, nullptr, PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
			PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_TERMINATE |
			READ_CONTROL | SYNCHRONIZE,*PsProcessType,KernelMode,&WaitStateChange->StateInfo.CreateProcessInfo.HandleToProcess);
		if (!NT_SUCCESS(status)) {
			WaitStateChange->StateInfo.CreateProcessInfo.HandleToProcess = nullptr;
		}

		OldHandle = WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle;
		if (OldHandle != nullptr) {
			CurrentProcess = PsGetCurrentProcess();
			status = ObDuplicateObject((PEPROCESS)CurrentProcess, OldHandle, (PEPROCESS)CurrentProcess,
				&WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle,
				0,
				0,
				DUPLICATE_SAME_ACCESS,
				KernelMode);
			if (!NT_SUCCESS(status)) {
				WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle = nullptr;
			}
			ObCloseHandle(OldHandle, KernelMode);
		}
		break;

	case DbgLoadDllStateChange:
		OldHandle = WaitStateChange->StateInfo.LoadDll.FileHandle;
		if (OldHandle != nullptr) {
			status = ObDuplicateObject((PEPROCESS)CurrentProcess,
				OldHandle, (PEPROCESS)CurrentProcess,
				&WaitStateChange->StateInfo.LoadDll.FileHandle,
				0,
				0,
				DUPLICATE_SAME_ACCESS,
				KernelMode);
			if (!NT_SUCCESS(status)) {
				WaitStateChange->StateInfo.LoadDll.FileHandle = nullptr;
			}
			ObCloseHandle(OldHandle, KernelMode);
		}
		break;

	default:
		break;
	}
}

PEPROCESS PsGetNextProcess(
	_In_ PEPROCESS Process
) {
	return nullptr;
}

NTSTATUS PsTerminateProcess(
	PEPROCESS Process,
	NTSTATUS Status
) {
	return STATUS_SUCCESS;
}

VOID DbgkpCloseObject(
	_In_ PEPROCESS Process,
	_In_ PVOID Object,
	_In_ ACCESS_MASK GrantedAccess,
	_In_ ULONG_PTR ProcessHandleCount,
	_In_ ULONG_PTR SystemHandleCount
) {
	PDEBUG_OBJECT DebugObject = (PDEBUG_OBJECT)Object;
	PDEBUG_EVENT DebugEvent;
	PLIST_ENTRY	Entry;
	BOOLEAN Deref;

	if (SystemHandleCount > 1) {
		return;
	}

	ExAcquireFastMutex(&DebugObject->Mutex);

	DebugObject->Flags |= DEBUG_OBJECT_DELETE_PENDING;

	Entry = DebugObject->EventList.Flink;
	InitializeListHead(&DebugObject->EventList);
	ExReleaseFastMutex(&DebugObject->Mutex);

	KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);

	// 枚举系统内的所有进程，如果发现某个进程的DebugObject字段的值与要关闭的对象相同，则将其置为0
	for (Process = PsGetNextProcess(nullptr); Process != nullptr; Process = PsGetNextProcess(Process)) {
		auto process = Process;
		/*if (process->DebugPort == DebugObject) {
			Deref = FALSE;
			ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);
			if (process->DebugPort == DebugObject) {
				process->DebugPort = nullptr;
				Deref = TRUE;
			}
			ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

			if (Deref) {
				if (DebugObject->Flags & DEBUG_OBJECT_KILL_ON_CLOSE) {
					PsTerminateProcess(Process, STATUS_DEBUGGER_INACTIVE);
				}
				ObDereferenceObject(DebugObject);
			}
		}*/
	}

	while (Entry != &DebugObject->EventList) {
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		Entry = Entry->Flink;
		DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
		DbgkpWakeTarget(DebugEvent);
	}
}