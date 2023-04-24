#include "pch.h"
#include "kDbgCore.h"
#include "kDbgSys.h"
#include <ntimage.h>
#include "kDbgUtil.h"
#include "NtWow64.h"


// 调试对象类型
POBJECT_TYPE* DbgkDebugObjectType;

// 保护对EPROCESS的DebugPort的访问
PFAST_MUTEX	g_pDbgkpProcessDebugPortMutex;

PULONG g_pDbgkpMaxModuleMsgs;

extern PULONG	g_pPspNotifyEnableMask;

NTSTATUS NTAPI
NewNtCreateDebugObject(
	_Out_ PHANDLE DebugObjectHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ ULONG Flags
) 
/*++

Routine Description:

	Creates a new debug object that maintains the context for a single debug session. Multiple processes may be
	associated with a single debug object.

Arguments:

	DebugObjectHandle - Pointer to a handle to recive the output objects handle
	DesiredAccess     - Required handle access
	ObjectAttributes  - Standard object attributes structure
	Flags             - Only one flag DEBUG_KILL_ON_CLOSE

Return Value:

	NTSTATUS - Status of call.

--*/
{
	NTSTATUS status;
	HANDLE handle;
	PDEBUG_OBJECT DebugObject;
	KPROCESSOR_MODE PreviousMode;

	PreviousMode = ExGetPreviousMode();

	// 判断用户层句柄地址是否合法
	__try {
		if (PreviousMode != KernelMode) {
			ProbeForWriteHandle(DebugObjectHandle);
		}
		*DebugObjectHandle = nullptr;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
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
		Flags,
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
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
	}

	return status;
}

NTSTATUS NewNtDebugActiveProcess(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
)
/*++
Routine Description:
	Attach a debug object to a process.

Arguments:
	ProcessHandle     - Handle to a process to be debugged
	DebugObjectHandle - Handle to a debug object

Return Value:

	NTSTATUS - Status of call.
--*/
{
	NTSTATUS status;
	KPROCESSOR_MODE PreviousMode;
	PDEBUG_OBJECT DebugObject;
	PEPROCESS Process, CurrentProcess;
	PETHREAD LastThread = nullptr;

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

	//
	// Don't let us debug ourselves or the system process.
	//
	if (Process == CurrentProcess || Process == PsInitialSystemProcess) {
		ObDereferenceObject(Process);
		return STATUS_ACCESS_DENIED;
	}

	// 得到调试句柄关联的调试对象DebugObject
	status = ObReferenceObjectByHandle(DebugObjectHandle,
		DEBUG_OBJECT_ADD_REMOVE_PROCESS,
		*DbgkDebugObjectType,
		PreviousMode,
		(PVOID*)&DebugObject,
		nullptr);

	if (NT_SUCCESS(status)) {
		//
		// We will be touching process address space. Block process rundown.
		//
		if (ExAcquireRundownProtection(kDbgUtil::GetProcessRundownProtect(Process))) {
			//
			// Post the fake process create messages etc.
			//
			status = DbgkpPostFakeProcessCreateMessages(Process, DebugObject, &LastThread);
			//
			// Set the debug port. If this fails it will remove any faked messages.
			//
			status = DbgkpSetProcessDebugObject(Process, DebugObject, status, LastThread);

			ExReleaseRundownProtection(kDbgUtil::GetProcessRundownProtect(Process));
		}
		else {
			status = STATUS_PROCESS_IS_TERMINATING;
		}

		ObDereferenceObject(DebugObject);
	}
	ObDereferenceObject(Process);

	return status;
}

NTSTATUS DbgkpPostFakeProcessCreateMessages(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ PETHREAD* pLastThread
)
/*++
Routine Description:
	This routine posts the faked initial process create, thread create and mudule load messages
Arguments:
	ProcessHandle     - Handle to a process to be debugged
	DebugObjectHandle - Handle to a debug object
Return Value:
	None.
--*/
{
	NTSTATUS status;
	KAPC_STATE ApcState;
	PETHREAD StartThread, Thread;
	PETHREAD LastThread;


	// 收集所有线程创建的消息
	status = kDbgUtil::g_pDbgkpPostFakeThreadMessages(
		Process,
		DebugObject,
		nullptr,
		&Thread,
		&LastThread
	);

	if (NT_SUCCESS(status)) {
		//
		// Attach to the process so we can touch its address space
		// 
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

	switch (DebugEvent->ApiMsg.ApiNumber) {
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

VOID DbgkpMarkProcessPeb(
	_In_ PEPROCESS Process
) 
/*++

Routine Description:
	
	This routine writes the debug variable in the PEB

Arguments:
	Process - Process that needs its PEB modified

Return Value:

	None.

--*/ 
{
	KAPC_STATE ApcState;

	//
	// Acquire process rundown protection as we are about to look at the processes address space
	// 
	
	if (ExAcquireRundownProtection(kDbgUtil::GetProcessRundownProtect(Process))) {
		PPEB Peb = kDbgUtil::GetProcessPeb(Process);
		if (Peb != nullptr) {
			KeStackAttachProcess(Process, &ApcState);
			ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);
			__try {
				PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
				PBOOLEAN pBeingDebugged = kDbgUtil::GetPEBBeingDebugged(Peb);
				*pBeingDebugged = (*pDebugObject != nullptr) ? TRUE : FALSE;
#ifdef _WIN64
				PWOW64_PROCESS Wow64Process = (PWOW64_PROCESS)kDbgUtil::GetProcessWow64Process(Process);
				if (Wow64Process != nullptr) {
					PPEB32 Peb32 = (PPEB32)Wow64Process->Wow64;
					if (Peb32 != nullptr) {
						Peb32->BeingDebugged = *pBeingDebugged;
					}
				}
#endif // 
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
			}
			ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);
			KeUnstackDetachProcess(&ApcState);
		}
		
		ExReleaseRundownProtection(kDbgUtil::GetProcessRundownProtect(Process));
	}
}

NTSTATUS
DbgkpSetProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ NTSTATUS MsgStatus,
	_In_ PETHREAD LastThread
)
/*++

Routine Description:
	Attach a debug object to a process.

Arguments:
	Process     - Process to be debugged
	DebugObject - Debug object to attach
	MsgStatus   - Status from queing the messages
	LastThread  - Last thread seen in attach loop.

Return Value:
	NTSTATUS - Status of call.
--*/
{
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

	PDEBUG_OBJECT* pProcessDebugPort = kDbgUtil::GetProcessDebugPort(Process);

	//
	// Pick up any threads we missed
	//
	if (NT_SUCCESS(status)) {
		while (true) {
			//
			// Acquire the debug port mutex so we know that any new threads will
			// have to wait to behind us.
			//
			GlobalHeld = TRUE;

			ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);

			//
			// If the port has been set then exit now.
			//
			// 如果被调试进程的Debug Port已经设置，那么跳出循环
			if (*pProcessDebugPort != nullptr) {
				status = STATUS_PORT_ALREADY_SET;
				break;
			}
			//
			// Assign the debug port to the process to pick up any new threads
			//
			//没有设置debugport，在这里设置
			*pProcessDebugPort = DebugObject;

			//
		   // Reference the last thread so we can deref outside the lock
		   //
			ObReferenceObject(LastThread);

			//
			// Search forward for new threads
			//
			Thread = kDbgUtil::g_pPsGetNextProcessThread(Process, LastThread);

			if (Thread != nullptr) {
				//
				// Remove the debug port from the process as we are
				// about to drop the lock
				//
				pProcessDebugPort = nullptr;
				ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);
				GlobalHeld = FALSE;

				ObDereferenceObject(LastThread);
				// 通知线程创建消息
				//
				// Queue any new thread messages and repeat.
				//

				status = DbgkpPostFakeThreadMessages(
					Process,
					DebugObject,
					Thread,
					&FirstThread,
					&LastThread
				);

				if (!NT_SUCCESS(status)) {
					LastThread = nullptr;
					break;
				}
				ObDereferenceObject(FirstThread);
			}
			else {
				break;
			}
		}
	}

	//
	// Lock the debug object so we can check its deleted status
	//
	ExAcquireFastMutex(&DebugObject->Mutex);

	//
	// We must not propagate a debug port thats got no handles left.
	//
	if (NT_SUCCESS(status)) {
		PULONG pFlags = kDbgUtil::GetProcessFlags(Process);
		if ((DebugObject->Flags & DEBUG_OBJECT_DELETE_PENDING) == 0) {
			PS_SET_BITS(pFlags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT | PS_PROCESS_FLAGS_CREATE_REPORTED);
			ObReferenceObject(DebugObject);
		}
		else {
			pProcessDebugPort = nullptr;
			status = STATUS_DEBUGGER_INACTIVE;
		}
	}

	// 遍历所有调试事件
	for (Entry = DebugObject->EventList.Flink; Entry != &DebugObject->EventList;) {
		// 取出调试事件
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		Entry = Entry->Flink;

		if ((DebugEvent->Flags & DEBUG_EVENT_INACTIVE) != 0 && DebugEvent->BackoutThread == ThisThread) {
			Thread = DebugEvent->Thread;

			//
			// If the thread has not been inserted by CreateThread yet then don't
			// create a handle. We skip system threads here also
			//
			PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(Thread);
			bool isSystemThread = (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_SYSTEM) != 0;
			if (NT_SUCCESS(status) &&
				!isSystemThread) {
				//
				// If we could not acquire rundown protection on this
				// thread then we need to suppress its exit message.
				//
				if ((DebugEvent->Flags & DEBUG_EVENT_PROTECT_FAILED) != 0) {
					PS_SET_BITS(pCrossThreadFlags, PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG);
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
					PS_SET_BITS(pCrossThreadFlags, PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG);
				}
			}
			else {
				RemoveEntryList(&DebugEvent->EventList);
				InsertTailList(&TempList, &DebugEvent->EventList);
			}

			if (DebugEvent->Flags & DEBUG_EVENT_RELEASE) {
				DebugEvent->Flags &= ~DEBUG_EVENT_RELEASE;
				PEX_RUNDOWN_REF RundownProtect = kDbgUtil::GetThreadRundownProtect(Thread);
				ExReleaseRundownProtection(RundownProtect);
			}
		}
	}

	ExReleaseFastMutex(&DebugObject->Mutex);

	if (GlobalHeld) {
		ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);
	}

	if (LastThread != nullptr) {
		ObDereferenceObject(LastThread);
	}

	while (!IsListEmpty(&TempList)) {
		Entry = RemoveHeadList(&TempList);
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		kDbgUtil::g_pDbgkpWakeTarget(DebugEvent);
	}

	if (NT_SUCCESS(status)) {
		kDbgUtil::g_pDbgkpMarkProcessPeb(Process);
	}

	return status;
}

VOID DbgkSendSystemDllMessages(
	PETHREAD Thread,
	PDEBUG_OBJECT DebugObject,
	PDBGKM_APIMSG ApiMsg
) 
{
	NTSTATUS status;
	PDBGKM_LOAD_DLL LoadDll;
	PIMAGE_NT_HEADERS NtHeaders;
	PTEB Teb;
	PPS_SYSTEM_DLL_INFO SystemDllInfo;
	IO_STATUS_BLOCK IoStatusBlock;
	PEPROCESS Process;
	BOOLEAN Attached;
	KAPC_STATE ApcState;

	if (Thread) {
		Process = PsGetThreadProcess(Thread);
	}
	else {
		Thread = KeGetCurrentThread();
		Process = kDbgUtil::GetThreadApcState(Thread)->Process;
	}
	
	LoadDll = &ApiMsg->u.LoadDll;

	for (int i = 0; i < 2; i++) {
		SystemDllInfo = kDbgUtil::g_pPsQuerySystemDllInfo(i);
		if (SystemDllInfo && (i != 1
#ifdef _WIN64
			|| kDbgUtil::GetProcessWow64Process(Process)
#endif // _WIN64
			)) {
			memset(LoadDll, 0, sizeof(DBGKM_LOAD_DLL));
			LoadDll->BaseOfDll = SystemDllInfo->ImageBase;
			if (Thread && i) {
				Attached = TRUE;
				KeStackAttachProcess(Process, &ApcState);
			}
			else {
				Attached = FALSE;
			}

			NtHeaders = RtlImageNtHeader(SystemDllInfo->ImageBase);
			if (NtHeaders) {
				LoadDll->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
				LoadDll->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
			}
			if (Thread) {
				Teb = nullptr;
			}
			else {
				Teb = (PTEB)PsGetCurrentThreadTeb();
				if (Teb) {
					RtlStringCbCopyW(Teb->StaticUnicodeBuffer, 
						sizeof(Teb->StaticUnicodeBuffer), 
						SystemDllInfo->DllName);
					PWCHAR* pArbitraryUserPointer = (PWCHAR*)Teb->NtTib.ArbitraryUserPointer;
					*pArbitraryUserPointer = Teb->StaticUnicodeBuffer;
					LoadDll->NamePointer = pArbitraryUserPointer;
				}
			}

			if (Attached) {
				KeUnstackDetachProcess(&ApcState);
			}
			OBJECT_ATTRIBUTES ObjectAttr;
			InitializeObjectAttributes(&ObjectAttr, &SystemDllInfo->Ntdll32Path,
				OBJ_CASE_INSENSITIVE | OBJ_FORCE_ACCESS_CHECK | OBJ_KERNEL_HANDLE,
				nullptr, nullptr);
			status = ZwOpenFile(&LoadDll->FileHandle, GENERIC_READ | SYNCHRONIZE,
				&ObjectAttr,
				&IoStatusBlock,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_SYNCHRONOUS_IO_NONALERT);
			if (!NT_SUCCESS(status)) {
				LoadDll->FileHandle = nullptr;
			}
			DBGKM_FORMAT_API_MSG(*ApiMsg, DbgKmLoadDllApi, sizeof(DBGKM_LOAD_DLL));
			if (Thread) {
				status = DbgkpQueueMessage(Process, Thread, ApiMsg, 2, DebugObject);
				if (!NT_SUCCESS(status)) {
					ObCloseHandle(LoadDll->FileHandle, KernelMode);
				}
			}
			else {
				DbgkpSendApiMessage(3, ApiMsg);
				if (LoadDll->FileHandle) {
					ObCloseHandle(LoadDll->FileHandle, KernelMode);
				}
				if (Teb) {
					Teb->NtTib.ArbitraryUserPointer = nullptr;
				}
			}
		}
	}
}

NTSTATUS DbgkpPostFakeThreadMessages(
	PEPROCESS	Process,
	PDEBUG_OBJECT	DebugObject,
	PETHREAD	StartThread,
	PETHREAD* pFirstThread,
	PETHREAD* pLastThread
)
/*++

Routine Description:
	This routine posts the faked initial process create, thread create messages

Arguments:

	Process      - Process to be debugged
	DebugObject  - Debug object to queue messages to
	StartThread  - Thread to start search from
	pFirstThread - First thread found in the list
	pLastThread  - Last thread found in the list

Return Value:

	None.

--*/
{
	NTSTATUS status;
	PETHREAD Thread, FirstThread, LastThread, CurrentThread;
	DBGKM_APIMSG ApiMsg;
	BOOLEAN First = TRUE;
	BOOLEAN IsFirstThread;
	PIMAGE_NT_HEADERS NtHeaders;
	ULONG Flags;
	KAPC_STATE ApcState;

	status = STATUS_UNSUCCESSFUL;

	LastThread = FirstThread = nullptr;

	CurrentThread = PsGetCurrentThread();

	if (StartThread != nullptr) {
		First = FALSE;
		FirstThread = StartThread;
		ObfReferenceObject(StartThread);
	}
	else {
		StartThread = kDbgUtil::g_pPsGetNextProcessThread(Process, nullptr);
		First = TRUE;
	}

	// 遍历调试进程的所有线程
	for (Thread = StartThread;
		Thread != nullptr;
		Thread = kDbgUtil::g_pPsGetNextProcessThread(Process, Thread)) {

		Flags = DEBUG_EVENT_NOWAIT;

		//
		// Keep a track on the last thread we have seen.
		// We use this as a starting point for new threads after we
		// really attach so we can pick up any new threads.
		//
		if (LastThread != nullptr) {
			ObDereferenceObject(LastThread);
		}

		LastThread = Thread;
		ObDereferenceObject(LastThread);

		//
		// Acquire rundown protection of the thread.
		// This stops the thread exiting so we know it can't send
		// it's termination message
		//
		PEX_RUNDOWN_REF RundownProtect = kDbgUtil::GetThreadRundownProtect(Thread);
		PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(Thread);
		bool isSystemThread = (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_SYSTEM) != 0;
		if (ExAcquireRundownProtection(RundownProtect)) {
			Flags |= DEBUG_EVENT_RELEASE;

			//
			// Suspend the thread if we can for the debugger
			// We don't suspend terminating threads as we will not be giving details
			// of these to the debugger.
			//

			if (!isSystemThread) {
				status = kDbgUtil::g_pPsSuspendThread(Thread, nullptr);
				if (NT_SUCCESS(status)) {
					Flags |= DEBUG_EVENT_SUSPEND;
				}
			}
		}
		else {
			//
			// Rundown protection failed for this thread.
			// This means the thread is exiting. We will mark this thread
			// later so it doesn't sent a thread termination message.
			// We can't do this now because this attach might fail.
			//
			Flags |= DEBUG_EVENT_PROTECT_FAILED;
		}

		// 每次构造一个DBGKM_APIMSG结构
		RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));

		if (First && (Flags & DEBUG_EVENT_PROTECT_FAILED) == 0 &&
			!isSystemThread) {
			// 进程的第一个线程
			IsFirstThread = TRUE;
		}
		else {
			IsFirstThread = FALSE;
		}

		if (IsFirstThread) {
			ApiMsg.ApiNumber = DbgKmCreateProcessApi;

			VOID* SectionObject = kDbgUtil::GetProcessSectionObject(Process);
			if (SectionObject) {
				ApiMsg.u.CreateProcess.FileHandle = kDbgUtil::g_pDbgkpSectionToFileHandle(SectionObject);
			}
			else {
				ApiMsg.u.CreateProcess.FileHandle = nullptr;
			}

			PVOID SectionBaseAddress = kDbgUtil::GetProcessSectionBaseAddress(Process);
			ApiMsg.u.CreateProcess.BaseOfImage = SectionBaseAddress;

			KeStackAttachProcess(Process, &ApcState);

			__try {
				NtHeaders = RtlImageNtHeader(SectionBaseAddress);
				if (NtHeaders) {
					ApiMsg.u.CreateProcess.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
					ApiMsg.u.CreateProcess.InitialThread.StartAddress = nullptr;
					ApiMsg.u.CreateProcess.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				ApiMsg.u.CreateProcess.InitialThread.StartAddress = nullptr;
				ApiMsg.u.CreateProcess.DebugInfoFileOffset = 0;
				ApiMsg.u.CreateProcess.DebugInfoSize = 0;
			}

			KeUnstackDetachProcess(&ApcState);
		}
		else {
			ApiMsg.ApiNumber = DbgKmCreateThreadApi;
			ApiMsg.u.CreateThread.StartAddress = kDbgUtil::GetThreadStartAddress(Thread);
		}
		status = DbgkpQueueMessage(Process, Thread, &ApiMsg,
			Flags,
			DebugObject);


		if (!NT_SUCCESS(status)) {
			if (Flags & DEBUG_EVENT_SUSPEND) {
				kDbgUtil::g_pPsResumeThread(Thread, nullptr);
			}
			if (Flags & DEBUG_EVENT_RELEASE) {
				ExReleaseRundownProtection(RundownProtect);
			}
			if (ApiMsg.ApiNumber == DbgKmCreateProcessApi && ApiMsg.u.CreateProcess.FileHandle != nullptr) {
				ObCloseHandle(ApiMsg.u.CreateProcess.FileHandle, KernelMode);
			}

			ObfDereferenceObject(Thread);
			break;
		}
		else if (IsFirstThread) {
			First = FALSE;
			ObReferenceObject(Thread);
			FirstThread = Thread;
			DbgkSendSystemDllMessages(Thread, DebugObject, &ApiMsg);
		}
	}

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


NTSTATUS
DbgkpPostModuleMessages(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_In_ PDEBUG_OBJECT DebugObject)
/*++

Routine Description:

	This routine posts the module load messages when we debug an active process.

--*/
{
	PPEB Peb = kDbgUtil::GetProcessPeb(Process);
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

	if (Peb == nullptr) {
		return STATUS_SUCCESS;
	}

	ULONG maxModuleMsgs = *g_pDbgkpMaxModuleMsgs;
	__try {
		Ldr = kDbgUtil::GetPEBLdr(Peb);

		LdrHead = &Ldr->InLoadOrderModuleList;
		ProbeForReadSmallStructure(LdrHead, sizeof(LIST_ENTRY), sizeof(UCHAR));
		for (LdrNext = LdrHead->Flink, i = 0;
			LdrNext != LdrHead && i < maxModuleMsgs;
			LdrNext = LdrNext->Flink, i++) {
			if (i > 1) {

				//
				// First image got send with process create message
				//
				RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));

				LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
				ProbeForReadSmallStructure(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY), sizeof(UCHAR));

				ApiMsg.ApiNumber = DbgKmLoadDllApi;
				ApiMsg.u.LoadDll.BaseOfDll = LdrEntry->DllBase;

				ProbeForReadSmallStructure(ApiMsg.u.LoadDll.BaseOfDll, sizeof(IMAGE_DOS_HEADER), sizeof(UCHAR));

				NtHeaders = RtlImageNtHeader(ApiMsg.u.LoadDll.BaseOfDll);
				if (NtHeaders) {
					ApiMsg.u.LoadDll.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
					ApiMsg.u.LoadDll.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
				}
				status = kDbgUtil::g_pMmGetFileNameForAddress(NtHeaders, &Name);
				if (NT_SUCCESS(status)) {
					InitializeObjectAttributes(&attr, &Name,
						OBJ_FORCE_ACCESS_CHECK | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
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
					DbgkpSendApiMessage(0x3, &ApiMsg);
					status = STATUS_UNSUCCESSFUL;
				}

				if (!NT_SUCCESS(status) && ApiMsg.u.LoadDll.FileHandle != nullptr) {
					ObCloseHandle(ApiMsg.u.LoadDll.FileHandle, KernelMode);
				}
			}

			ProbeForReadSmallStructure(LdrNext, sizeof(LIST_ENTRY), sizeof(UCHAR));
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}

#ifdef _WIN64
	PWOW64_PROCESS Wow64Process = (PWOW64_PROCESS)kDbgUtil::GetProcessWow64Process(Process);
	if (Wow64Process != nullptr && Wow64Process->Wow64 != nullptr) {
		PPEB32 Peb32;
		PPEB_LDR_DATA32 Ldr32;
		PLIST_ENTRY32 LdrHead32, LdrNext32;
		PLDR_DATA_TABLE_ENTRY32 LdrEntry32;
		PWCHAR pSys;

		Peb32 = (PPEB32)Wow64Process->Wow64;

		__try {
			Ldr32 = (PPEB_LDR_DATA32)ULongToPtr(Peb32->Ldr);

			LdrHead32 = &Ldr32->InLoadOrderModuleList;


			ProbeForReadSmallStructure(LdrHead32, sizeof(LIST_ENTRY32), sizeof(UCHAR));
			for (LdrNext32 = (PLIST_ENTRY32)UlongToPtr(LdrHead32->Flink), i = 0;
				LdrNext32 != LdrHead32 && i < maxModuleMsgs;
				LdrNext32 = (PLIST_ENTRY32)UlongToPtr(LdrNext32->Flink), i++) {

				if (i > 1) {
					RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));

					LdrEntry32 = CONTAINING_RECORD(LdrNext32, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);
					ProbeForReadSmallStructure(LdrEntry32, sizeof(LDR_DATA_TABLE_ENTRY32), sizeof(UCHAR));

					ApiMsg.ApiNumber = DbgKmLoadDllApi;
					ApiMsg.u.LoadDll.BaseOfDll = UlongToPtr(LdrEntry32->DllBase);
					ApiMsg.u.LoadDll.NamePointer = nullptr;

					ProbeForReadSmallStructure(ApiMsg.u.LoadDll.BaseOfDll, sizeof(IMAGE_DOS_HEADER), sizeof(UCHAR));

					NtHeaders = RtlImageNtHeader(ApiMsg.u.LoadDll.BaseOfDll);
					if (NtHeaders) {
						ApiMsg.u.LoadDll.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
						ApiMsg.u.LoadDll.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
					}

					status = kDbgUtil::g_pMmGetFileNameForAddress(NtHeaders, &Name);
					if (NT_SUCCESS(status)) {
						ASSERT(sizeof(L"SYSTEM32") == sizeof(WOW64_SYSTEM_DIRECTORY_U));
						pSys = wcsstr(Name.Buffer, L"\\SYSTEM32\\");
						if (pSys != NULL) {
							RtlCopyMemory(pSys + 1,
								WOW64_SYSTEM_DIRECTORY_U,
								sizeof(WOW64_SYSTEM_DIRECTORY_U) - sizeof(UNICODE_NULL));
						}

						InitializeObjectAttributes(&attr,
							&Name,
							OBJ_FORCE_ACCESS_CHECK | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
							nullptr,
							nullptr);

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
						status = DbgkpQueueMessage(Process,
							Thread,
							&ApiMsg,
							DEBUG_EVENT_NOWAIT,
							DebugObject);
					}
					else {
						status = DbgkpSendApiMessage(3, &ApiMsg);
						status = STATUS_UNSUCCESSFUL;
					}

					if (!NT_SUCCESS(status) && ApiMsg.u.LoadDll.FileHandle != NULL) {
						ObCloseHandle(ApiMsg.u.LoadDll.FileHandle, KernelMode);
					}
				}
				ProbeForReadSmallStructure(LdrNext32, sizeof(LIST_ENTRY32), sizeof(UCHAR));
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {

		}
	}
#endif

	return STATUS_SUCCESS;
}

VOID NewDbgkMapViewOfSection(
	_In_ PEPROCESS Process,
	_In_ PVOID SectionObject,
	_In_ PVOID BaseAddress
) 
/*++

Routine Description:
	This function is called when the current process successfully
	maps a view of an image section. If the process has an associated
	debug port, then a load dll message is sent.

Arguments:
	SectionObject - Supplies a pointer to the section mapped by the process.

	BaseAddress - Supplies the base address of where the section is 
	mapped in the current process address space.
--*/
{
	PVOID Port;
	HANDLE	hFile = nullptr;
	DBGKM_APIMSG ApiMsg;
	PEPROCESS	CurrentProcess;
	PETHREAD	CurrentThread;
	PIMAGE_NT_HEADERS	pImageHeader;
	PDBGKM_LOAD_DLL LoadDllArgs;

	CurrentProcess = PsGetCurrentProcess();
	CurrentThread = PsGetCurrentThread();

	if (ExGetPreviousMode() == KernelMode){
		return;
	}

	PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(CurrentThread);
	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
		Port = nullptr;
	}
	else {
		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		Port = *pDebugObject;
	}

	if (!Port) {
		return;
	}
	
	LoadDllArgs = &ApiMsg.u.LoadDll;
	LoadDllArgs->FileHandle = kDbgUtil::g_pDbgkpSectionToFileHandle(SectionObject);
	LoadDllArgs->BaseOfDll = BaseAddress;
	LoadDllArgs->DebugInfoFileOffset = 0;
	LoadDllArgs->DebugInfoSize = 0;

	//
	// The loader fills in the module name in this pointer before mapping
	// the section. It's a very poor linkage.
	//

	PTEB Teb = (PTEB)PsGetCurrentThreadTeb();
	LoadDllArgs->NamePointer = Teb->NtTib.ArbitraryUserPointer;

	__try {
		pImageHeader = RtlImageNtHeader(BaseAddress);
		if (pImageHeader != nullptr) {
			ApiMsg.u.LoadDll.DebugInfoFileOffset = pImageHeader->FileHeader.PointerToSymbolTable;
			ApiMsg.u.LoadDll.DebugInfoSize = pImageHeader->FileHeader.NumberOfSections;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		ApiMsg.u.LoadDll.DebugInfoFileOffset = 0;
		ApiMsg.u.LoadDll.DebugInfoSize = 0;
		ApiMsg.u.LoadDll.NamePointer = nullptr;
	}

	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmLoadDllApi, sizeof(DBGKM_LOAD_DLL));

	DbgkpSendApiMessage(0x1, &ApiMsg);
	if (ApiMsg.u.LoadDll.FileHandle != nullptr) {
		ObCloseHandle(ApiMsg.u.LoadDll.FileHandle, KernelMode);
	}
}

VOID NewDbgkUnMapViewOfSection(
	_In_ PEPROCESS Process,
	_In_ PVOID BaseAddress
)/*++

 Routine Description:

	This function is called when the current process successfully
	unmpas a view of an image section. If the process has an associated
	debug port, then an "unmap view of section" message is sent.

 --*/ 
{
	PVOID Port;
	DBGKM_APIMSG ApiMsg;
	PEPROCESS	CurrentProcess;
	PETHREAD	CurrentThread;

	CurrentProcess = PsGetCurrentProcess();
	CurrentThread = PsGetCurrentThread();

	if (ExGetPreviousMode() == KernelMode) {
		return;
	}

	PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(CurrentThread);
	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
		Port = nullptr;
	}
	else {
		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		Port = *pDebugObject;
	}

	if (!Port) {
		return;
	}

	PDBGKM_UNLOAD_DLL UnloadDllArgs;
	UnloadDllArgs = &ApiMsg.u.UnloadDll;
	UnloadDllArgs->BaseAddress = BaseAddress;

	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmUnloadDllApi, sizeof(DBGKM_UNLOAD_DLL));
	DbgkpSendApiMessage(0x1, &ApiMsg);
}

VOID NewDbgkCreateThread(
	PETHREAD Thread
)
/*++

Routine Description:

	This function is called when a new thread begins to execute. If the
	thread has an associated DebugPort, then a message is sent thru the
	port.

	If this thread is the first thread in the process, then this event
	is translated into a CreateProcessInfo message.

	If a message is sent, then while the thread is waiting a reply,
	all other threads in the process are suspended.

Arguments:
	
	Thread - New thread just being started.

Return Value:

	None.

 --*/ 
{
	PVOID Port;
	DBGKM_APIMSG m;
	PDBGKM_CREATE_THREAD CreateThreadArgs;
	PDBGKM_CREATE_PROCESS CreateProcessArgs;
	PEPROCESS Process;
	PDBGKM_LOAD_DLL LoadDllArgs;
	NTSTATUS status;
	PIMAGE_NT_HEADERS NtHeaders = nullptr;
	PTEB Teb;
	ULONG OldFlags = 0;
	PFILE_OBJECT FileObject;
	OBJECT_ATTRIBUTES ioStatus;
	PVOID SectionObject;
	PETHREAD CurrentThread;


#if defined(_WIN64)
	PVOID Wow64Process = nullptr;
#endif

	CurrentThread = PsGetCurrentThread();

	Process = kDbgUtil::GetThreadApcState(CurrentThread)->Process;

#if defined(_WIN64)
	Wow64Process = kDbgUtil::GetProcessWow64Process(Process);
#endif

	PULONG pFlags = kDbgUtil::GetProcessFlags(Process);
	OldFlags = PS_TEST_SET_BITS(pFlags, PS_PROCESS_FLAGS_CREATE_REPORTED | PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE);
	ULONG PspNotifyEnableMask = *g_pPspNotifyEnableMask;
	if ((OldFlags & PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE) == 0 && (PspNotifyEnableMask & 1)) {
		IMAGE_INFO_EX ImageInfoEx;
		PUNICODE_STRING ImageName;
		POBJECT_NAME_INFORMATION FileNameInfo;

		//
		// notification of main.exe
		//
		ImageInfoEx.ImageInfo.Properties = 0;
		ImageInfoEx.ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
		PVOID SectionBaseAddress = kDbgUtil::GetProcessSectionBaseAddress(Process);
		ImageInfoEx.ImageInfo.ImageBase = SectionBaseAddress;
		ImageInfoEx.ImageInfo.ImageSize = 0;

		__try {
			NtHeaders = RtlImageNtHeader(SectionBaseAddress);
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
		
		PsCallImageNotifyRoutines(ImageName, PsGetProcessId(Process), &ImageInfoEx, FileObject);
		if (ImageName) {
			//因为在SeLocateProcessImageName中为ImageName申请了内存，所以要在此处释放掉
			ExFreePool(ImageName);
		}

		ObDereferenceObject(FileObject);

		//
		// system dll
		// 
		for (int i = 0; i < 2; i++) {
			PPS_SYSTEM_DLL_INFO info = kDbgUtil::g_pPsQuerySystemDllInfo(i);
			if (info != nullptr) {
				ImageInfoEx.ImageInfo.Properties = 0;
				ImageInfoEx.ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
				ImageInfoEx.ImageInfo.ImageBase = info->ImageBase;
				ImageInfoEx.ImageInfo.ImageSize = 0;

				__try {
					NtHeaders = RtlImageNtHeader(info->ImageBase);
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

				
				PPS_SYSTEM_DLL sysDll = CONTAINING_RECORD(info, PS_SYSTEM_DLL, SystemDllInfo);
				
				SectionObject = kDbgUtil::g_pObFastReferenceObject(&sysDll->SectionObjectFastRef);
				if (SectionObject == nullptr) {
					KeEnterCriticalRegion();
					ExAcquirePushLockShared((PEX_PUSH_LOCK_S)&sysDll->PushLock);
					SectionObject = kDbgUtil::g_pObFastReferenceObjectLocked(&sysDll->SectionObjectFastRef);
					ExReleasePushLockShared((PEX_PUSH_LOCK_S)&sysDll->PushLock);
					KeLeaveCriticalRegion();
				}
				
				PVOID SectionControlArea = kDbgUtil::g_pMiSectionControlArea(SectionObject);
				FileObject = kDbgUtil::g_pMiReferenceControlAreaFile(SectionControlArea);
				if (FileObject != nullptr) {
					kDbgUtil::g_pObFastDereferenceObject(&sysDll->SectionObjectFastRef, SectionObject);
				}
				PsCallImageNotifyRoutines(&info->Ntdll32Path,
					PsGetProcessId(Process),
					&ImageInfoEx,
					FileObject);
				if(FileObject!=nullptr)
					ObDereferenceObject(FileObject);
			}
		}

		// 检测新创建的线程所在的进程是否正在被调试
		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		if (*pDebugObject == nullptr) {
			return;
		}
		// 判断是否调用过调试函数
		if ((OldFlags & PS_PROCESS_FLAGS_CREATE_REPORTED) == 0) {
			CreateThreadArgs = &m.u.CreateProcess.InitialThread;
			CreateThreadArgs->SubSystemKey = 0;
			CreateThreadArgs->StartAddress = nullptr;

			CreateProcessArgs = &m.u.CreateProcess;
			CreateProcessArgs->SubSystemKey = 0;
			PVOID SectionObject = kDbgUtil::GetProcessSectionObject(Process);
			CreateProcessArgs->FileHandle = kDbgUtil::g_pDbgkpSectionToFileHandle(SectionObject);

			PVOID SectionBaseAddress = kDbgUtil::GetProcessSectionBaseAddress(Process);
			CreateProcessArgs->BaseOfImage = SectionBaseAddress;
			CreateProcessArgs->DebugInfoFileOffset = 0;
			CreateProcessArgs->DebugInfoSize = 0;

			__try {
				NtHeaders = RtlImageNtHeader(SectionBaseAddress);
				if (NtHeaders) {
					CreateThreadArgs->StartAddress = (PVOID)(DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders, AddressOfEntryPoint) +
						DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(NtHeaders, ImageBase));
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

			DbgkpSendApiMessage(0, &m);
			if (CreateProcessArgs->FileHandle != nullptr) {
				ObCloseHandle(CreateProcessArgs->FileHandle, KernelMode);
			}

			DbgkSendSystemDllMessages(nullptr, nullptr, &m);
		}
		else {
			CreateThreadArgs = &m.u.CreateThread;
			CreateThreadArgs->SubSystemKey = 0;
			CreateThreadArgs->StartAddress = kDbgUtil::GetThreadWin32StartAddress(Thread);
			DBGKM_FORMAT_API_MSG(m, DbgKmCreateThreadApi, sizeof(*CreateThreadArgs));
			DbgkpSendApiMessage(1, &m);
		}
		ULONG ClonedThread = kDbgUtil::GetThreadClonedThread(Thread);
		if (ClonedThread) {
			DbgkpPostModuleMessages(
				Process,
				Thread,
				nullptr
			);
		}
	}
}

NTSTATUS
DbgkpQueueMessage(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_Inout_ PDBGKM_APIMSG ApiMsg,
	_In_ ULONG Flags,
	_In_ PDEBUG_OBJECT TargetDebugObject
)
/*++

Routine Description:

	Queues a debug message to the port for a user mode debugger to get.

Arguments:

	Process				- Process being debugged
	Thread				- Thread making call
	ApiMsg				- Message being sent and received
	NoWait				- Don't wait for a response. Buffer message and return.
	TargetDebugObject	- Port to queue nowait messages to

Return Value:

	NTSTATUS - Status of call.
--*/
{
	PDEBUG_EVENT DebugEvent;
	DEBUG_EVENT StaticDebugEvent;
	PDEBUG_OBJECT DebugObject = nullptr;
	NTSTATUS status;


	if (Flags & DEBUG_EVENT_NOWAIT) {
		// NoWait: Don't wait for a response. Buffer message and return.
		DebugEvent = (PDEBUG_EVENT)ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(*DebugEvent), 'ssap');
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
		ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);

		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		DebugObject = *pDebugObject;

		PULONG pCrossThreadFlags = kDbgUtil::GetProcessFlags(Process);
		ULONG CrossThreadFlags = *pCrossThreadFlags;
		//
		// See if this create message has already been sent.
		//
		if (ApiMsg->ApiNumber == DbgKmCreateProcessApi ||
			ApiMsg->ApiNumber == DbgKmCreateProcessApi) {
			if (CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG) {
				DebugObject = nullptr;
			}
		}

		if (ApiMsg->ApiNumber == DbgKmLoadDllApi &&
			CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG &&
			Flags & 0x40) {
			DebugObject = nullptr;
		}

		//
		// See if this exit message is for thread that never had a create
		//
		if (ApiMsg->ApiNumber == DbgKmExitThreadApi ||
			ApiMsg->ApiNumber == DbgKmExitProcessApi) {
			if (CrossThreadFlags & PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG) {
				DebugObject = nullptr;
			}
		}

		KeInitializeEvent(&DebugEvent->ContinueEvent, SynchronizationEvent, FALSE);
	}


	DebugEvent->Process = Process;
	DebugEvent->Thread = Thread;
	DebugEvent->ApiMsg = *ApiMsg;
	DebugEvent->ClientId = kDbgUtil::GetThreadCid(Thread);

	if (DebugObject == nullptr) {
		status = STATUS_PORT_NOT_SET;
	}
	else {
		//
		// We must not use a debug port thats got no handles left
		//
		ExAcquireFastMutex(&DebugObject->Mutex);
		//
		// If the object is delete pending then don't use this object.
		//
		if ((DebugObject->Flags & DEBUG_OBJECT_DELETE_PENDING) == 0) {
			InsertTailList(&DebugObject->EventList, &DebugEvent->EventList);
			//
			// Set the event to say there is an unread event in the object.
			//
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
		ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);
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

NTSTATUS NewNtWaitForDebugEvent(
	_In_ HANDLE DebugObjectHandle,
	_In_ BOOLEAN Alertable,
	_In_opt_ PLARGE_INTEGER Timeout,
	_Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
)
/*++

Routine Description:

	Waits for a debug event and returns it to the user if one arrives

Arguments:

	DebugObjectHandle - Handle to a debug object
	Alertable - TRUE is the wait to be alertable
	Timeout - Operation timeout value
	WaitStateChange - Returned debug event

Return Value:

	Status of operation

--*/ {
	NTSTATUS status;
	KPROCESSOR_MODE PreviousMode;
	PDEBUG_OBJECT DebugObject;
	LARGE_INTEGER Tmo = { 0 };
	LARGE_INTEGER StartTime = { 0 };
	DBGUI_WAIT_STATE_CHANGE waitState{};
	PEPROCESS Process;
	PETHREAD Thread;

	PreviousMode = ExGetPreviousMode();

	__try {
		if (ARGUMENT_PRESENT(Timeout)) {
			if (PreviousMode != KernelMode) {
				ProbeForReadSmallStructure(Timeout, sizeof(*Timeout), sizeof(UCHAR));
			}
			
			KeQuerySystemTime(&StartTime);
		}
		if (PreviousMode != KernelMode) {
			ProbeForWriteSmallStructure(WaitStateChange, sizeof(*WaitStateChange), sizeof(UCHAR));
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

	BOOLEAN GotEvent;
	PDEBUG_EVENT DebugEvent, DebugEvent2;
	PLIST_ENTRY Entry, Entry2;

	while (true) {
		// 在调试对象有事件时产生
		status = KeWaitForSingleObject(&DebugObject->EventsPresent,
			Executive, PreviousMode, Alertable, Timeout);
		if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT
			|| status == STATUS_ALERTED || status == STATUS_USER_APC) {
			break;
		}

		GotEvent = FALSE;

		DebugEvent = nullptr;

		ExAcquireFastMutex(&DebugObject->Mutex);

		//
		// If the object is delete pending then return an error.
		//
		if ((DebugObject->Flags & DEBUG_OBJECT_DELETE_PENDING)==0) {

			for (Entry = DebugObject->EventList.Flink;
				Entry != &DebugObject->EventList;
				Entry = Entry->Flink) {

				DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);

				//
				// If this event has not been given back to the user yet and is not
				// inactive then pass it back.
				// We check to see if we have any other outstanding messages for this
				// thread as this confuses VC. You can only get multiple events
				// for the same thread for the attach faked messages.
				//
				if ((DebugEvent->Flags & (DEBUG_EVENT_READ | DEBUG_EVENT_INACTIVE)) == 0) {
					GotEvent = TRUE;
					for (Entry2 = DebugObject->EventList.Flink;
						Entry2 != Entry;
						Entry2 = Entry2->Flink) {

						DebugEvent2 = CONTAINING_RECORD(Entry2, DEBUG_EVENT, EventList);

						if (DebugEvent->ClientId.UniqueProcess == DebugEvent2->ClientId.UniqueProcess) {
							//
							// This event has the same process as an earlier event. Mark it as inactive.
							//
							DebugEvent->Flags |= DEBUG_EVENT_INACTIVE;
							DebugEvent->BackoutThread = nullptr;
							GotEvent = FALSE;
							break;
						}
					}
					if (GotEvent) {
						break;
					}
				}
			}

			if (GotEvent) {
				Process = DebugEvent->Process;
				Thread = DebugEvent->Thread;
				ObReferenceObject(Thread);
				ObReferenceObject(Process);
				kDbgUtil::g_pDbgkpConvertKernelToUserStateChange(&waitState, DebugEvent);
				DebugEvent->Flags |= DEBUG_EVENT_READ;
			}
			else {
				//
				// No unread events there. Clear the event.
				KeClearEvent(&DebugObject->EventsPresent);
			}
			status = STATUS_SUCCESS;;

		}
		else {
			status = STATUS_DEBUGGER_INACTIVE;
		}

		ExReleaseFastMutex(&DebugObject->Mutex);

		if (NT_SUCCESS(status)) {
			//
			// If we woke up and found nothing
			//
			if (GotEvent == FALSE) {
				//
				// If timeout is a delta time then adjust it for the wait so far.
				//
				if (Tmo.QuadPart < 0) {
					LARGE_INTEGER NewTime;
					KeQuerySystemTime(&NewTime);
					Tmo.QuadPart = Tmo.QuadPart + (NewTime.QuadPart - StartTime.QuadPart);
					StartTime = NewTime;
					if (Tmo.QuadPart >= 0) {
						status = STATUS_TIMEOUT;
						break;
					}
				}
			}
			else {
				//
				// Fixup needed handles. The caller could have guessed the thread id etc by now and made the target thread
				// continue. This isn't a problem as we won't do anything damaging to the system in this case. The caller
				// won't get the correct results but they set out to break us.
				//
				kDbgUtil::g_pDbgkpOpenHandles(&waitState, Process, Thread);
				ObDereferenceObject(Thread);
				ObDereferenceObject(Process);
				break;
			}
		}
		else {
			break;
		}
	}

	ObDereferenceObject(DebugObject);

	__try {
		*WaitStateChange = waitState;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
	}
	return status;
}

NTSTATUS NewNtDebugContinue(
	_In_ HANDLE DebugObjectHandle,
	_In_ PCLIENT_ID AppClientId,
	_In_ NTSTATUS ContinueStatus
) 
/*++

Routine Description:
	Continues a stalled debugged thread

Arguments:

	DebugObjectHandle - Handle to a debug object
	ClientId - ClientId of thread tro continue
	ContinueStatus - Status of continue

Return Value:
	Status of operation

--*/
{
	NTSTATUS status;
	PDEBUG_OBJECT DebugObject;
	PDEBUG_EVENT DebugEvent, FoundDebugEvent;
	KPROCESSOR_MODE PreviousMode;
	PLIST_ENTRY Entry;
	CLIENT_ID ClientId;
	BOOLEAN GotEvent;

	PreviousMode = ExGetPreviousMode();

	__try {
		if (PreviousMode != KernelMode) {
			ProbeForReadSmallStructure(AppClientId, sizeof(*AppClientId), sizeof(UCHAR));
		}
		ClientId = *AppClientId;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {// If previous mode is kernel then don't handle the exception
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

	GotEvent = FALSE;
	FoundDebugEvent = nullptr;

	ExAcquireFastMutex(&DebugObject->Mutex);
	for (Entry = DebugObject->EventList.Flink;
		Entry != &DebugObject->EventList;
		Entry = Entry->Flink) {

		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);

		//
		// Make sure the client ID matches that the debugger saw all the events.
		// We don't allow the caller to start a thread that it never saw a message for.
		//
		if (DebugEvent->ClientId.UniqueProcess == AppClientId->UniqueProcess) {
			if (!GotEvent) {
				if (DebugEvent->ClientId.UniqueThread == ClientId.UniqueThread &&
					(DebugEvent->Flags & DEBUG_EVENT_READ) != 0) {
					RemoveEntryList(Entry);
					FoundDebugEvent = DebugEvent;
					GotEvent = TRUE;
				}
			}
			else {
				//
				// VC breaks if it sees more than one event at a time
				// for the same process.
				//
				DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
				KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);
				break;
			}
		}
	}

	ExReleaseFastMutex(&DebugObject->Mutex);
	ObReferenceObject(DebugObject);

	if (GotEvent) {
		FoundDebugEvent->ApiMsg.ReturnedStatus = ContinueStatus;
		FoundDebugEvent->Status = STATUS_SUCCESS;
		kDbgUtil::g_pDbgkpWakeTarget(FoundDebugEvent);
	}
	else {
		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}

NTSTATUS NewNtRemoveProcessDebug(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
) 
/*++
Routine Description:

	Remove a debug object from a process.

Arguments:

	ProcessHandle - Handle to a process currently being debugged

Return Value:

	NTSTATUS - Status of call.

--*/
{
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
		nullptr);
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

	status = NewDbgkClearProcessDebugObject((PEPROCESS)Process, DebugObject);

	ObDereferenceObject(DebugObject);
	ObDereferenceObject(Process);
	return status;
}

VOID DbgkpResumeProcess(
	_In_opt_ PEPROCESS Process
) {
	if (Process == nullptr) {
		kDbgUtil::g_pKeThawAllThreads();
	}
}

NTSTATUS DbgkpSendApiMessage(
	UCHAR	Flags,
	PDBGKM_APIMSG ApiMsg
)
/*++

Routine Description:

	This function sends the specified API message over the specified
	port. It is the caller's responsibility to format the API message
	prior to calling this function.

	If the SuspendProcess flag is supplied, then all threads in the calling
	process are first suspended.
	Upon receipt of the reply message, the threads,the threads are resumed.


--*/
{
	NTSTATUS status;
	BOOLEAN SuspendProcess;
	PEPROCESS Process;
	PETHREAD Thread;

	SuspendProcess = FALSE;

	if (Flags & 0x1) {
		SuspendProcess = kDbgUtil::g_pDbgkpSuspendProcess();
	}

	ApiMsg->ReturnedStatus = STATUS_PENDING;

	status = DbgkpQueueMessage(
		PsGetCurrentProcess(),
		KeGetCurrentThread(),
		ApiMsg,
		((Flags & 0x2) << 0x5),
		nullptr
	);
	ZwFlushInstructionCache(ZwCurrentProcess(), nullptr, 0);
	if (SuspendProcess) {
		DbgkpResumeProcess(nullptr);
	}

	return status;
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

VOID NewDbgkExitThread(
	NTSTATUS ExitStatus
) 
/*++
Routine Description:
	
	This function is called when a new thread terminates. At this 
	point, the thread will no longer execute in user-mode. No other
	exit processing has occured.

	If a message is sent, then while the thread is awaiting a reply,
	all other threads in the process are suspended.

Arguments:

	ExitStatus - Supplies the ExitStatus of the exiting thread.

Return Value:

	None.

--*/
{
	DBGKM_APIMSG ApiMsg;
	PEPROCESS Process;
	PETHREAD CurrentThread;
	PVOID Port;
	PDBGKM_EXIT_THREAD args;

	CurrentThread = PsGetCurrentThread();
	Process = PsGetCurrentProcess();

	PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(CurrentThread);
	
	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
		Port = nullptr;
	}
	else {
		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		Port = *pDebugObject;
	}

	if (!Port) {
		return;
	}

	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_DEADTHREAD) {
		return;
	}

	args = &ApiMsg.u.ExitThread;
	args->ExitStatus = ExitStatus;

	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmExitThreadApi, sizeof(*args));

	DbgkpSendApiMessage(FALSE, &ApiMsg);

	BOOLEAN Frozen = kDbgUtil::g_pDbgkpSuspendProcess();

	if (Frozen) {
		DbgkpResumeProcess(nullptr);
	}
}

VOID NewDbgkExitProcess(
	NTSTATUS ExitStatus
)
/*++

Routine Description:

	This function is called when a process terminates. The address
    space of the process is still intact, but no threads exist in
    the process.

Arguments:

    ExitStatus - Supplies the ExitStatus of the exiting process.

Return Value:

    None.
--*/
{
	DBGKM_APIMSG ApiMsg;
	PEPROCESS Process;
	PETHREAD CurrentThread;
	PVOID Port;
	PDBGKM_EXIT_PROCESS args;

	CurrentThread = PsGetCurrentThread();
	Process = PsGetCurrentProcess();

	PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(CurrentThread);

	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
		Port = nullptr;
	}
	else {
		PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
		Port = *pDebugObject;
	}

	if (!Port) {
		return;
	}

	if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_DEADTHREAD) {
		return;
	}

	//
	// this ensures that other timed lockers of the process will bail
	// since this call is done while holding the process lock, and lock duration
	// is controlled by debugger
	//
	PLARGE_INTEGER pExitTime = kDbgUtil::GetProcessExitTime(Process);
	KeQuerySystemTime(pExitTime);

	args = &ApiMsg.u.ExitProcess;
	args->ExitStatus = ExitStatus;

	DBGKM_FORMAT_API_MSG(ApiMsg, DbgKmExitProcessApi, sizeof(*args));

	DbgkpSendApiMessage(FALSE, &ApiMsg);
}

BOOLEAN NewDbgkForwardException(
	_In_ PEXCEPTION_RECORD ExceptionRecord,
	_In_ BOOLEAN DebugException,
	_In_ BOOLEAN SecondChance
)
/*++
Routine Description:

	This function is called forward an exception to the calling process's
	debug or subsystem exception port.

Arguments:

	ExceptionRecord - Supplies a pointer to an exception record.
	DebugException - Supplies a boolean variable that specifies whether
		this exception is to be forwarded to the process's
		DebugPort(TRUE), or to its ExceptionPort(FALSE).

Return Value:

	TRUE - The process has a DebugPort or an ExceptionPort, and the reply
		received from the port indicated that the exception was handled.

	FALSE - The process either does not have a DebugPort or
		ExceptionPort, or the process has a port, but the reply received
		from the port indicated that the exception was not handled.

--*/
{
	NTSTATUS status;

	PEPROCESS		Process;
	PVOID			ExceptionPort = nullptr;
	PDEBUG_OBJECT	DebugObject = nullptr;
	BOOLEAN			LpcPort = FALSE;

	DBGKM_APIMSG m;
	PDBGKM_EXCEPTION args;

	args = &m.u.Exception;

	//
	// Initialize the debug LPC message with default information
	//

	DBGKM_FORMAT_API_MSG(m, DbgKmExceptionApi, sizeof(*args));

	//
	// Get the address of the destination LPC port.
	//

	Process = PsGetCurrentProcess();
	if (DebugException) {
		PETHREAD CurrentThread = PsGetCurrentThread();

		PULONG pCrossThreadFlags = kDbgUtil::GetThreadCrossThreadFlags(CurrentThread);
		if (*pCrossThreadFlags & PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
			DebugObject = nullptr;
		}
		else {
			PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(Process);
			DebugObject = *pDebugObject;
		}
		LpcPort = FALSE;
	}
	else {
		ExceptionPort = kDbgUtil::g_pPsCaptureExceptionPort((PEPROCESS)Process);
		m.h.u2.ZeroInit = LPC_EXCEPTION;
		LpcPort = TRUE;
	}

	//
	// If the destination LPC port address is NULL, then return FALSE.
	//

	if (DebugObject == nullptr) {
		return FALSE;
	}

	//
	// Fill in the reminder of the debug LPC message
	//

	args->ExceptionRecord = *ExceptionRecord;
	args->FirstChance = !SecondChance;

	//
	// Send the debug message to the destination LPC port.
	//

	if (LpcPort) {
		status = kDbgUtil::g_pDbgkpSendApiMessageLpc(&m, ExceptionPort, DebugException);
	}
	else{
		status = DbgkpSendApiMessage(DebugException, &m);
	}

	//
	// If the send was not successful, then return a FALSE indicating that 
	// the port did not handle the exception. Otherwise, if the debug port
	// is specified, then look at the return status in the message.
	//

	if (!NT_SUCCESS(status) ||
		(DebugException) && 
		(m.ReturnedStatus == DBG_EXCEPTION_NOT_HANDLED || !NT_SUCCESS(m.ReturnedStatus))
		) {
		if (SecondChance) {
			status = kDbgUtil::g_pDbgkpSendErrorMessage(ExceptionRecord, FALSE, &m);
			return NT_SUCCESS(status);
		}
		return FALSE;
	}
	else {
		return TRUE;
	}
}

NTSTATUS NewDbgkClearProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT SourceDebugObject
)
/*++

Routine Description:
	
	Remove a debug port from a process

Arguments:
	
	Process - Process to be debugged
	SourceDebugObject - Debug object to detach

Return Value:
	
	NTSTATUS - Status of call.

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEBUG_OBJECT DebugObject = nullptr;
	PDEBUG_EVENT DebugEvent;
	LIST_ENTRY TempList;
	PLIST_ENTRY Entry;

	ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);

	PDEBUG_OBJECT* pDebugPort = kDbgUtil::GetProcessDebugPort(Process);
	DebugObject = *pDebugPort;
	if (DebugObject == nullptr || (DebugObject != SourceDebugObject && SourceDebugObject != nullptr)) {
		DebugObject = nullptr;
		status = STATUS_PORT_NOT_SET;
	}
	else {
		*pDebugPort = nullptr;
		status = STATUS_SUCCESS;
	}
	ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);

	if (NT_SUCCESS(status)) {
		DbgkpMarkProcessPeb(Process);
	}

	//
	// Remove any events for this process and wake up the threads
	//
	if (DebugObject) {
		//
		// Remove any events and queue them to a tempory queue
		//
		InitializeListHead(&TempList);

		ExAcquireFastMutex(&DebugObject->Mutex);
		for (Entry = DebugObject->EventList.Flink; 
			Entry != &DebugObject->EventList;
			) {

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

		//
		// Wake up all the removed threads.
		//
		while (!IsListEmpty(&TempList)) {
			Entry = RemoveHeadList(&TempList);
			DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
			DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
			kDbgUtil::g_pDbgkpWakeTarget(DebugEvent);
		}
	}

	return status;
}

NTSTATUS NewNtSetInformationDebugObject(
	_In_ HANDLE DebugObjectHandle,
	_In_ DEBUG_OBJECT_INFORMATION_CLASS DebugObjectInformationClass,
	_In_ PVOID DebugInformation,
	_In_ ULONG DebugInformationLength,
	_Out_opt_ PULONG ReturnLength
) 
/*++

Routine Description:

	This function sets the state of a debug object.

Arguments:

	DebugObjectHandle -  Supplies a handle to a process debug object

	DebugObjectInformationClass - Supplies the class of information being
		set.

	DebugInformation - Supplies a pointer to a record that contains the information
		to set.

	ReturnLength - Supplies the length of the record that contains the information
		to set.

Return Value:

	NTSTATUS - Status of call

--*/ 
{
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
			}
			else {
				DebugObject->Flags &= ~DEBUG_OBJECT_KILL_ON_CLOSE;
			}
			ExReleaseFastMutex(&DebugObject->Mutex);
			ObDereferenceObject(DebugObject);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS NewDbgkCopyProcessDebugPort(
	_In_ PEPROCESS TargetProcess,
	_In_ PEPROCESS SourceProcess,
	_In_ PDEBUG_OBJECT DebugObject,
	_Out_ PBOOLEAN bFlag
)
/*++

Routine Description:

	Copies a debug port from one process to another.

Arguments:

	TargetProcess - Process to move port to
	SourceProcess - Process to move port from

--*/
{

	PDEBUG_OBJECT* pDebugObject = kDbgUtil::GetProcessDebugPort(TargetProcess);

	// New process. Needs no locks.
	*pDebugObject = nullptr;

	PDEBUG_OBJECT* pSourceDebugPort = kDbgUtil::GetProcessDebugPort(SourceProcess);
	if (*pSourceDebugPort != nullptr) {
		ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);
		DebugObject = *pSourceDebugPort;
		PULONG pFlags = kDbgUtil::GetProcessFlags(SourceProcess);
		if (DebugObject != nullptr && (*pFlags & PS_PROCESS_FLAGS_NO_DEBUG_INHERIT) == 0){
			//
			// We must not propagate a debug port thats got no handles left.
			//
			ExAcquireFastMutex(&DebugObject->Mutex);

			//
			// If the object is delete pending then don't propagate this object.
			//
			if ((DebugObject->Flags & DEBUG_OBJECT_DELETE_PENDING) == 0) {
				ObReferenceObject(DebugObject);
				*pDebugObject = DebugObject;
			}

			ExReleaseFastMutex(&DebugObject->Mutex);
		}
		ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);
	}

	return STATUS_SUCCESS;
}

VOID DbgkpCloseObject(
	_In_ PEPROCESS Process,
	_In_ PVOID Object,
	_In_ ACCESS_MASK GrantedAccess,
	_In_ ULONG_PTR ProcessHandleCount,
	_In_ ULONG_PTR SystemHandleCount
)
/*++

Routine Description:
	
	Called by the object manager when handle is closed to the object.

Arguments:
	
	Process - Process doing the close
	Object - Debug object being deleted
	GrantedAccess - Access granted for this handle
	ProcessHandleCount - Unused and unmaintained by OB
	SystemHandleCount - Current handle count for this object

Return Value:
	
	None.

--*/
{
	PDEBUG_OBJECT DebugObject = (PDEBUG_OBJECT)Object;
	PDEBUG_EVENT DebugEvent;
	PLIST_ENTRY	Entry;
	BOOLEAN Deref;

	//
	// If this isn't the last then do nothing
	//
	if (SystemHandleCount > 1) {
		return;
	}

	ExAcquireFastMutex(&DebugObject->Mutex);

	//
	// Mark this object as going away and wake up any processes that are waiting.
	//
	DebugObject->Flags |= DEBUG_OBJECT_DELETE_PENDING;

	//
	// Remove any events and queue them to a temporary queue
	//
	Entry = DebugObject->EventList.Flink;
	InitializeListHead(&DebugObject->EventList);

	ExReleaseFastMutex(&DebugObject->Mutex);

	//
	// Wake anyone waiting. They need to leave this object alone now as its deleting
	//
	KeSetEvent(&DebugObject->EventsPresent, 0, FALSE);

	//
	// Loop over all processes and remove the debug port from any that still have it.
	// Debug port propagation was disabled by setting the delete pending flag above so we only have to do this
	// once. No more refs can appear now.
	// 
	// 枚举系统内的所有进程，如果发现某个进程的DebugObject字段的值与要关闭的对象相同，则将其置为0
	for (Process = kDbgUtil::g_pPsGetNextProcess(nullptr); Process != nullptr; Process = kDbgUtil::g_pPsGetNextProcess(Process)) {
		
		PDEBUG_OBJECT* pDebugPort = kDbgUtil::GetProcessDebugPort(Process);
		if (*pDebugPort == DebugObject) {
			Deref = FALSE;
			ExAcquireFastMutex(g_pDbgkpProcessDebugPortMutex);
			if (*pDebugPort == DebugObject) {
				*pDebugPort = nullptr;
				Deref = TRUE;
			}
			ExReleaseFastMutex(g_pDbgkpProcessDebugPortMutex);

			if (Deref) {
				DbgkpMarkProcessPeb(Process);
				//
				// If the caller wanted process deletion on debugger dying (old interface) then kill off the process.
				//
				if (DebugObject->Flags & DEBUG_OBJECT_KILL_ON_CLOSE) {
					kDbgUtil::g_pPsTerminateProcess(Process, STATUS_DEBUGGER_INACTIVE);
				}
				ObDereferenceObject(DebugObject);
			}
		}
	}
	//
	// Wake up all removed threads
	//
	while (Entry != &DebugObject->EventList) {
		DebugEvent = CONTAINING_RECORD(Entry, DEBUG_EVENT, EventList);
		Entry = Entry->Flink;
		DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
		kDbgUtil::g_pDbgkpWakeTarget(DebugEvent);
	}
}

VOID ExAcquirePushLockShared(_In_ PEX_PUSH_LOCK_S PushLock)
/*++

Routine Description:
	Acquire a push lock shared

Arguments:

	PushLock - Push lock to be acquired
--*/ {
	EX_PUSH_LOCK_S OldValue, NewValue;

	OldValue.Value = 0;
	NewValue.Value = EX_PUSH_LOCK_SHARE_INC | EX_PUSH_LOCK_LOCK;

	if (InterlockedCompareExchangePointer(&PushLock->Ptr,
		NewValue.Ptr, OldValue.Ptr) != OldValue.Ptr) {
		kDbgUtil::g_pExfAcquirePushLockShared((PEX_PUSH_LOCK)PushLock);
	}
}

VOID ExReleasePushLockShared(_In_ PEX_PUSH_LOCK_S PushLock)
/*++
Routine Description:
	Release a push lock was acquired shared

Arguments:
	PushLock - Push lock to be released
--*/
{
	EX_PUSH_LOCK_S OldValue, NewValue;

	OldValue = *PushLock;

	OldValue.Value = EX_PUSH_LOCK_SHARE_INC | EX_PUSH_LOCK_LOCK;
	NewValue.Value = 0;

	if (InterlockedCompareExchangePointer(&PushLock->Ptr,
		NewValue.Ptr, OldValue.Ptr) != OldValue.Ptr) {
		kDbgUtil::g_pExfReleasePushLockShared((PEX_PUSH_LOCK)PushLock);
	}
}

VOID DbgkpDeleteObject(_In_ PDEBUG_OBJECT DebugObject) {
	IsListEmpty(&DebugObject->EventList);
}