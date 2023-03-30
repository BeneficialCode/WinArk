#pragma once
#include "SysMon.h"
#include "kDbgCore.h"

extern "C"
NTSTATUS ObCreateObject(
	_In_opt_ KPROCESSOR_MODE ObjectAttributesAccessMode,
	_In_ POBJECT_TYPE Type,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ KPROCESSOR_MODE AccessMode,
	_Inout_opt_ PVOID ParseContext,
	_In_ ULONG ObjectSize,
	_In_opt_ ULONG PagedPoolCharge,
	_In_opt_ ULONG NonPagedPoolCharge,
	_Out_ PVOID * Object
);

extern "C"
PIMAGE_NT_HEADERS RtlImageNtHeader(
	_In_ PVOID BaseAddress
);

extern "C"
NTSTATUS
PsReferenceProcessFilePointer(
	_In_ PEPROCESS Process,
	_Out_ PFILE_OBJECT* FileObject
);

extern "C"{
	NTSYSAPI NTSTATUS NTAPI ZwFlushInstructionCache(_In_ HANDLE 	ProcessHandle,
		_In_ PVOID 	BaseAddress,
		_In_ ULONG 	NumberOfBytesToFlush
	);
}


extern "C"
NTSTATUS ObDuplicateObject(
	_In_ PEPROCESS SourceProcess,
	_In_ HANDLE SourceHandle,
	_In_opt_ PEPROCESS TargetProcess,
	_Out_opt_ PHANDLE TargetHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ ULONG HandleAttributes,
	_In_ ULONG Options,
	_In_ KPROCESSOR_MODE PreviousMode
);

PEPROCESS PsGetCurrentProcessByThread(PETHREAD Thread);

using PPsGetNextProcessThread = PETHREAD(NTAPI*) (
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread
);

using PPsSuspendThread = NTSTATUS(NTAPI*) (
	_In_ PETHREAD Thread,
	_Out_opt_ PULONG PreviousSuspendCount
);

using PDbgkpSectionToFileHandle = HANDLE(NTAPI*) (
	_In_ PVOID SectionObject
);

using PPsResumeThread = NTSTATUS(NTAPI*) (
	_In_ PETHREAD Thread,
	_Out_opt_ PULONG PreviousSuspendCount
);

using PMmGetFileNameForAddress = NTSTATUS(NTAPI*) (
	_In_ PVOID ProcessVa,
	_Out_ PUNICODE_STRING FileName
);

using PDbgkpSuspendProcess = BOOLEAN(NTAPI*)();

using PKeThawAllThreads = VOID(NTAPI*)();

using PDbgkpResumeProcess = VOID(NTAPI*) (
	_In_ PEPROCESS Process
);

using PPsQuerySystemDllInfo = PPS_SYSTEM_DLL_INFO(NTAPI*)(_In_ ULONG Type);

using PPsCallImageNotifyRoutines = VOID(NTAPI*)(
	_In_ PUNICODE_STRING ImageName,
	_In_ HANDLE ProcessId,
	_In_ PVOID FileObject,
	_Out_ PIMAGE_INFO_EX ImageInfoEx);

using PObFastReferenceObject = PVOID(NTAPI*) (
	_In_ PEX_FAST_REF FastRef
);

using PExfAcquirePushLockShared = PVOID(NTAPI*)(
	_Inout_ PEX_PUSH_LOCK PushLock
);

using PExfReleasePushLockShared = PVOID(NTAPI*)(
	_Inout_ PEX_PUSH_LOCK PushLock
	);

using PObFastReferenceObjectLocked = PVOID(NTAPI*)(
	_In_ PEX_FAST_REF FastRef
	);

using PPsGetNextProcess = PEPROCESS (NTAPI*)(
	_In_ PEPROCESS Process
);

// 创建调试对象
NTSTATUS 
NTAPI
NtCreateDebugObject(
	_Out_ PHANDLE DebugObjectHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ ULONG Flags
);

// 与一个已经运行的进程建立调试会话
NTSTATUS
NtDebugActiveProcess(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
);

// 将一个调试对象附加到被调试进程中
NTSTATUS
DbgkpSetProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ NTSTATUS MsgStatus,
	_In_ PETHREAD LastThread
);

/**
* Emulation System
* 
*/
// 向调试子系统发送虚假的进程创建消息
NTSTATUS DbgkpPostFakeProcessCreateMessages(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT DebugObject,
	_In_ PETHREAD* pLastThread
);

// 向调试子系统发送虚假的线程创建消息
NTSTATUS DbgkpPostFakeThreadMessages(
	PEPROCESS	Process,
	PDEBUG_OBJECT	DebugObject,
	PETHREAD	StartThread,
	PETHREAD* pFirstThread,
	PETHREAD* pLastThread
);

// 向调试子系统发送虚假的模块加载消息
NTSTATUS DbgkpPostModuleMessages(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_In_ PDEBUG_OBJECT DebugObject
);

//PVOID ObFastReferenceObjectLocked(
//	_In_ PEX_FAST_REF FastRef
//);
//
//PVOID ObFastDereferenceObject(
//	_In_ PEX_FAST_REF FastRef,
//	_In_ PVOID Object
//);

/**
* Event Collection Routine
*
*/
// 采集线程创建事件
VOID DbgkCreateThread(
	PETHREAD Thread
);

// 线程退出消息
VOID DbgkExitThread(
	NTSTATUS ExitStatus
);

// 进程退出消息
VOID DbgkExitProcess(
	NTSTATUS ExitStatus
);

// 模块加载
VOID DbgkMapViewOfSection(
	_In_ PVOID SectionObject,
	_In_ PVOID BaseAddress,
	_In_ PEPROCESS Process
);

// 模块卸载
VOID DbgkUnMapViewOfSection(
	_In_ PEPROCESS Process,
	_In_ PVOID BaseAddress
);

// 向一个调试对象的消息队列中追加调试事件
NTSTATUS DbgkpQueueMessage(
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread,
	_Inout_ PDBGKM_APIMSG ApiMsg,
	_In_ ULONG Flags,
	_In_ PDEBUG_OBJECT TargetDebugObject
);


// 等待调试事件
NTSTATUS NtWaitForDebugEvent(
	_In_ HANDLE DebugObjectHandle,
	_In_ BOOLEAN Alertable,
	_In_opt_ PLARGE_INTEGER Timeout,
	_Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
);


// 回复调试事件，恢复被调试进程
NTSTATUS NtDebugContinue(
	_In_ HANDLE DebugObjectHandle,
	_In_ PCLIENT_ID AppClientId,
	_In_ NTSTATUS ContinueStatus
);

// 分离调试会话
NTSTATUS NtRemoveProcessDebug(
	_In_ HANDLE ProcessHandle,
	_In_ HANDLE DebugObjectHandle
);

// 发送调试事件
NTSTATUS DbgkpSendApiMessage(
	UCHAR	Flags,
	PDBGKM_APIMSG ApiMsg
);

//PSYSTEM_DLL_INFO PsQuerySystemDllInfo(
//	ULONG index
//);

VOID ExAcquirePushLockShared(_In_ PEX_PUSH_LOCK_S PushLock);

VOID ExReleasePushLockShared(_In_ PEX_PUSH_LOCK_S PushLock);

LOGICAL ExFastRefDereference(
	_Inout_ PEX_FAST_REF FastRef,
	_In_ PVOID Object
);

EX_FAST_REF
ExFastReference(
	_Inout_ PEX_FAST_REF FastRef
);

LOGICAL
ExFastRefAddAdditionalReferenceCounts(
	_Inout_ PEX_FAST_REF FastRef,
	_In_ PVOID Object,
	_In_ ULONG RefsToAdd
);

//PETHREAD PsGetNextProcessThread(
//	_In_ PEPROCESS Process,
//	_In_ PETHREAD Thread
//);

// 取得Section对应的文件句柄
//HANDLE DbgkpSectionToFileHandle(
//	_In_ PVOID SectionObject
//);

//NTSTATUS PsSuspendThread(
//	_In_ PETHREAD Thread,
//	_Out_opt_ PULONG PreviousSuspendCount
//);

//NTSTATUS PsResumeThread(
//	_In_ PETHREAD Thread,
//	_Out_opt_ PULONG PreviousSuspendCount
//);

VOID DbgkSendSystemDllMessages(
	PETHREAD Thread,
	PDEBUG_OBJECT DebugObject,
	PDBGKM_APIMSG ApiMsg
);



//BOOLEAN DbgkpSuppressDbgMsg(
//	_In_ PTEB Teb
//);


// 向当前进程的异常端口发送异常的第二轮处理机会




// 恢复被调试进程的执行
VOID DbgkpResumeProcess(
	_In_opt_ PEPROCESS Process
);

// 向调试子系统发送异常消息
BOOLEAN DbgkForwardException(
	_In_ PEXCEPTION_RECORD ExceptionRecord,
	_In_ BOOLEAN DebugException,
	_In_ BOOLEAN SecondChance
);

// 唤醒等待调试器回复的线程


// 释放调试事件
VOID DbgkpFreeDebugEvent(
	_In_ PDEBUG_EVENT DebugEvent
);

// 设置进程环境块的BeingDebugged
VOID DbgkpMarkProcessPeb(
	_In_ PEPROCESS Process
);

// 将内核模式的结构体转换为用户模式的结构体


//NTSTATUS MmGetFileNameForAddress(
//	_In_ PVOID ProcessVa,
//	_Out_ PUNICODE_STRING FileName
//);

NTSTATUS DbgkClearProcessDebugObject(
	_In_ PEPROCESS Process,
	_In_ PDEBUG_OBJECT SourceDebugObject
);


// 设置调试对象的属性
NTSTATUS NtSetInformationDebugObject(
	_In_ HANDLE DebugObjectHandle,
	_In_ DEBUG_OBJECT_INFORMATION_CLASS DebugObjectInformationClass,
	_In_ PVOID DebugInformation,
	_In_ ULONG DebugInformationLength,
	_Out_opt_ PULONG ReturnLength
);

// 查询调试信息输出的过滤级别
NTSTATUS NtQueryDebugFilterState(
	_In_ ULONG ComponentId,
	_In_ ULONG Level
);

// 设置调试信息输出的过滤级别
NTSTATUS NtSetDebugFilterState(
	_In_ ULONG ComponentId,
	_In_ ULONG Level,
	_In_ BOOLEAN State
);

// 访问指定进程中的调试对象
NTSTATUS DbgkOpenProcessObject(
	_In_ PEPROCESS Process,
	_In_ PACCESS_STATE AccessState,
	_In_ ACCESS_MASK DesiredAccess
);

// 打开进程，线程对象，增加引用计数
VOID DbgkpOpenHandles(
	_In_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
	_In_ PEPROCESS Process,
	_In_ PETHREAD Thread
);

// 关闭调试对象
VOID DbgkpCloseObject(
	_In_ PEPROCESS Process,
	_In_ PVOID Object,
	_In_ ACCESS_MASK GrantedAccess,
	_In_ ULONG_PTR ProcessHandleCount,
	_In_ ULONG_PTR SystemHandleCount
);

// 拷贝调试对象
NTSTATUS DbgkCopyProcessDebugPort(
	_In_ PEPROCESS TargetProcess,
	_In_ PEPROCESS SourceProcess,
	_In_ PDEBUG_OBJECT DebugObject,
	_Out_ PBOOLEAN bFlag
);

NTSTATUS PsTerminateProcess(
	PEPROCESS Process,
	NTSTATUS Status
);


VOID DbgkpDeleteObject(_In_ PDEBUG_OBJECT DebugObject);