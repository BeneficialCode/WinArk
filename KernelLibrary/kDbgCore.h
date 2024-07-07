#pragma once

#include "NtMmApi.h"

#define DEBUG_OBJECT_DELETE_PENDING			(0x1) // Debug object is delete pending.
#define DEBUG_OBJECT_KILL_ON_CLOSE			(0x2) // Kill all debugged processes on close

#define DEBUG_KILL_ON_CLOSE					(0x01)
#define PROCESS_SUSPEND_RESUME             (0x0800) 

#define DEBUG_EVENT_READ					(0x01)  // Event had been seen by win32 app
#define DEBUG_EVENT_NOWAIT					(0x02)  // No waiter one this. Just free the pool
#define DEBUG_EVENT_INACTIVE				(0x04)  // The message is in inactive. It may be activated or deleted later
#define DEBUG_EVENT_RELEASE					(0x08)  // Release rundown protection on this thread
#define DEBUG_EVENT_PROTECT_FAILED			(0x10)  // Rundown protection failed to be acquired on this thread
#define DEBUG_EVENT_SUSPEND					(0x20)  // Resume thread on continue

// Used to signify that the delete APC has been queued or the
// thread has called PspExitThread itself.
//
#define PS_CROSS_THREAD_FLAGS_TERMINATED           0x00000001UL
//
// Thread create failed
//
#define PS_CROSS_THREAD_FLAGS_DEADTHREAD           0x00000002UL
//
// Debugger isn't shown this thread
//
#define PS_CROSS_THREAD_FLAGS_HIDEFROMDBG          0x00000004UL
//
// Thread is impersonating
//
#define PS_CROSS_THREAD_FLAGS_IMPERSONATING        0x00000008UL
//
// This is a system thread
//
#define PS_CROSS_THREAD_FLAGS_SYSTEM               0x00000010UL
//
// Hard errors are disabled for this thread
//
#define PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED 0x00000020UL
//
// We should break in when this thread is terminated
//
#define PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION 0x00000040UL
//
// This thread should skip sending its create thread message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG    0x00000080UL
//
// This thread should skip sending its final thread termination message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG 0x00000100UL

#define IS_SYSTEM_THREAD(CrossThreadFlags)  ((CrossThreadFlags&PS_CROSS_THREAD_FLAGS_SYSTEM)!=0)
//
// Debug Object Access Masks
//
#define DEBUG_OBJECT_WAIT_STATE_CHANGE      0x0001
#define DEBUG_OBJECT_ADD_REMOVE_PROCESS     0x0002
#define DEBUG_OBJECT_SET_INFORMATION        0x0004
#define DEBUG_OBJECT_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x0F)

#define PS_PROCESS_FLAGS_CREATE_REPORTED        0x00000001UL // Create process debug call has occurred
#define PS_PROCESS_FLAGS_NO_DEBUG_INHERIT       0x00000002UL // Don't inherit debug port
#define PS_PROCESS_FLAGS_PROCESS_EXITING        0x00000004UL // PspExitProcess entered
#define PS_PROCESS_FLAGS_PROCESS_DELETE         0x00000008UL // Delete process has been issued
#define PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES      0x00000010UL // Wow64 split pages
#define PS_PROCESS_FLAGS_VM_DELETED             0x00000020UL // VM is deleted
#define PS_PROCESS_FLAGS_OUTSWAP_ENABLED        0x00000040UL // Outswap enabled
#define PS_PROCESS_FLAGS_OUTSWAPPED             0x00000080UL // Outswapped
#define PS_PROCESS_FLAGS_FORK_FAILED            0x00000100UL // Fork status
#define PS_PROCESS_FLAGS_WOW64_4GB_VA_SPACE     0x00000200UL // Wow64 process with 4gb virtual address space
#define PS_PROCESS_FLAGS_ADDRESS_SPACE1         0x00000400UL // Addr space state1
#define PS_PROCESS_FLAGS_ADDRESS_SPACE2         0x00000800UL // Addr space state2
#define PS_PROCESS_FLAGS_SET_TIMER_RESOLUTION   0x00001000UL // SetTimerResolution has been called
#define PS_PROCESS_FLAGS_BREAK_ON_TERMINATION   0x00002000UL // Break on process termination
#define PS_PROCESS_FLAGS_CREATING_SESSION       0x00004000UL // Process is creating a session
#define PS_PROCESS_FLAGS_USING_WRITE_WATCH      0x00008000UL // Process is using the write watch APIs
#define PS_PROCESS_FLAGS_IN_SESSION             0x00010000UL // Process is in a session
#define PS_PROCESS_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00020000UL // Process must use native address space (Win64 only)
#define PS_PROCESS_FLAGS_HAS_ADDRESS_SPACE      0x00040000UL // This process has an address space
#define PS_PROCESS_FLAGS_LAUNCH_PREFETCHED      0x00080000UL // Process launch was prefetched
#define PS_PROCESS_INJECT_INPAGE_ERRORS         0x00100000UL // Process should be given inpage errors - hardcoded in trap.asm too
#define PS_PROCESS_FLAGS_VM_TOP_DOWN            0x00200000UL // Process memory allocations default to top-down
#define PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE      0x00400000UL // We have sent a message for this image
#define PS_PROCESS_FLAGS_PDE_UPDATE_NEEDED      0x00800000UL // The system PDEs need updating for this process (NT32 only)
#define PS_PROCESS_FLAGS_VDM_ALLOWED            0x01000000UL // Process allowed to invoke NTVDM support
#define PS_PROCESS_FLAGS_SMAP_ALLOWED           0x02000000UL // Process allowed to invoke SMAP support
#define PS_PROCESS_FLAGS_CREATE_FAILED          0x04000000UL // Process create failed

#define PS_PROCESS_FLAGS_DEFAULT_IO_PRIORITY    0x38000000UL // The default I/O priority for created threads. (3 bits)

#define PS_PROCESS_FLAGS_PRIORITY_SHIFT         27

#define PS_SET_BITS(Flags, Flag) \
	RtlInterlockedSetBitsDiscardReturn(Flags, Flag)

#define PS_TEST_SET_BITS(Flags, Flag) \
	RtlInterlockedSetBits(Flags, Flag)

#define LPC_REQUEST								1
#define LPC_REPLY								2
#define LPC_DATAGRAM							3
#define LPC_LOST_REPLY							4
#define LPC_PORT_CLOSED							5
#define LPC_CLIENT_DIED							6
#define LPC_EXCEPTION							7
#define LPC_DEBUG_EVENT							8
#define LPC_ERROR_EVENT							9
#define LPC_CONNECTION_REQUEST					10

#define THREAD_QUERY_INFORMATION         (0x0040)  



#define DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER(hdrs,field) \
	((hdrs)->OptionalHeader.##field)

#define DBGKM_MSG_OVERHEAD \
	(FIELD_OFFSET(DBGKM_APIMSG, u.Exception) - sizeof(PORT_MESSAGE))

#define DBGKM_API_MSG_LENGTH(TypeSize) \
	((sizeof(DBGKM_APIMSG) << 16) | (DBGKM_MSG_OVERHEAD + (TypeSize)))

#define DBGKM_FORMAT_API_MSG(m,Number,TypeSize)             \
	(m).h.u1.Length = DBGKM_API_MSG_LENGTH((TypeSize));     \
	(m).h.u2.ZeroInit = LPC_DEBUG_EVENT;                    \
	(m).ApiNumber = (Number)

#define DBGKP_API_SEND_ALL_FLAGS	(0x3)

//
// Portable LPC Types for 32/64-bit compatibility
//
#ifdef USE_LPC6432
#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG
#else
#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE
#endif

//
// Debug Object Information Structures
//
typedef struct _DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION
{
	ULONG KillProcessOnExit;
} DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION, * PDEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION;

//调试对象
typedef struct _DEBUG_OBJECT
{
	KEVENT EventsPresent;	// 指示有调试事件发生
	FAST_MUTEX Mutex;		// 用于同步的互斥对象
	LIST_ENTRY EventList;	// 保持调试事件的链表，调试消息队列
	union
	{
		ULONG Flags;
		struct
		{
			UCHAR DebuggerInactive : 1;
			UCHAR KillProcessOnExit : 1;
		};
	};
} DEBUG_OBJECT, * PDEBUG_OBJECT;

//
// Debug Message API Number
// 
// 指定是哪种事件
typedef enum _DBGKM_APINUMBER
{
	DbgKmExceptionApi = 0,		// 异常
	DbgKmCreateThreadApi = 1,	// 创建线程
	DbgKmCreateProcessApi = 2,	// 创建进程
	DbgKmExitThreadApi = 3,		// 线程退出
	DbgKmExitProcessApi = 4,	// 进程退出
	DbgKmLoadDllApi = 5,		// 加载DLL
	DbgKmUnloadDllApi = 6,		// 卸载DLL
	DbgKmErrorReportApi = 7,	// 内部错误
	DbgKmMaxApiNumber = 8,		// 这组常量的最大值
} DBGKM_APINUMBER;

// 异常消息
typedef struct _DBGKM_EXCEPTION
{
	EXCEPTION_RECORD ExceptionRecord;
	ULONG FirstChance;
} DBGKM_EXCEPTION, * PDBGKM_EXCEPTION;

// 创建线程消息
typedef struct _DBGKM_CREATE_THREAD
{
	ULONG SubSystemKey;
	PVOID StartAddress;
} DBGKM_CREATE_THREAD, * PDBGKM_CREATE_THREAD;

// 创建进程消息
typedef struct _DBGKM_CREATE_PROCESS
{
	ULONG SubSystemKey;
	HANDLE FileHandle;
	PVOID BaseOfImage;
	ULONG DebugInfoFileOffset;
	ULONG DebugInfoSize;
	DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, * PDBGKM_CREATE_PROCESS;

// 退出线程消息
typedef struct _DBGKM_EXIT_THREAD
{
	NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, * PDBGKM_EXIT_THREAD;

// 退出进程消息
typedef struct _DBGKM_EXIT_PROCESS
{
	NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, * PDBGKM_EXIT_PROCESS;

// 加载模块消息
typedef struct _DBGKM_LOAD_DLL
{
	HANDLE FileHandle;
	PVOID BaseOfDll;
	ULONG DebugInfoFileOffset;
	ULONG DebugInfoSize;
	PVOID NamePointer;
} DBGKM_LOAD_DLL, * PDBGKM_LOAD_DLL;

// 卸载模块消息
typedef struct _DBGKM_UNLOAD_DLL
{
	PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, * PDBGKM_UNLOAD_DLL;

typedef struct _DBGKM_ERROR_MSG {
	EXCEPTION_RECORD ExceptionRecord;
	SECTION_IMAGE_INFORMATION ImageInfo;
	union 
	{
		ULONG Flags;
		struct {
			ULONG IsProtectedProcess : 1;
			ULONG IsWow64Process : 1;
			ULONG IsFilterMessage : 1;
			ULONG SpareBits : 29;
		};
	};
}DBGKM_ERROR_MSG,*PDBGKM_ERROR_MSG;

//
// LPC Port Message
//
typedef struct _PORT_MESSAGE
{
	union
	{
		struct
		{
			CSHORT DataLength;
			CSHORT TotalLength;
		} s1;
		ULONG Length;
	} u1;
	union
	{
		struct
		{
			CSHORT Type;
			CSHORT DataInfoOffset;
		} s2;
		ULONG ZeroInit;
	} u2;
	union
	{
		CLIENT_ID ClientId;
		double DoNotUseThisField;
	};
	ULONG MessageId;
	union
	{
		LPC_SIZE_T ClientViewSize;
		ULONG CallbackId;
	};
} PORT_MESSAGE, * PPORT_MESSAGE;

// 消息结构
typedef struct _DBGKM_APIMSG
{
	PORT_MESSAGE h;
	DBGKM_APINUMBER ApiNumber;
	NTSTATUS ReturnedStatus;
	union
	{
		DBGKM_EXCEPTION Exception;
		DBGKM_CREATE_THREAD CreateThread;
		DBGKM_CREATE_PROCESS CreateProcess;
		DBGKM_EXIT_THREAD ExitThread;
		DBGKM_EXIT_PROCESS ExitProcess;
		DBGKM_LOAD_DLL LoadDll;
		DBGKM_UNLOAD_DLL UnloadDll;
		DBGKM_ERROR_MSG ErrorMsg;
	}u;
} DBGKM_APIMSG, * PDBGKM_APIMSG;

typedef struct _DEBUG_EVENT
{
	LIST_ENTRY EventList;
	KEVENT ContinueEvent;
	CLIENT_ID ClientId;
	PEPROCESS Process;
	PETHREAD Thread;
	NTSTATUS Status;
	ULONG Flags;
	PETHREAD BackoutThread;		// 产生fake message的线程
	DBGKM_APIMSG ApiMsg;
} DEBUG_EVENT, * PDEBUG_EVENT;

//
// Debug States
//
typedef enum _DBG_STATE
{
	DbgIdle,
	DbgReplyPending,
	DbgCreateThreadStateChange,
	DbgCreateProcessStateChange,
	DbgExitThreadStateChange,
	DbgExitProcessStateChange,
	DbgExceptionStateChange,
	DbgBreakpointStateChange,
	DbgSingleStepStateChange,
	DbgLoadDllStateChange,
	DbgUnloadDllStateChange
} DBG_STATE, * PDBG_STATE;

typedef struct _DBGUI_CREATE_THREAD {
	HANDLE HandleToThread;
	DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, * PDBGUI_CREATE_THREAD;

typedef struct _DBGUI_CREATE_PROCESS {
	HANDLE HandleToProcess;
	HANDLE HandleToThread;
	DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, * PDBGUI_CREATE_PROCESS;

//
// User-Mode Debug State Change Structure
//
typedef struct _DBGUI_WAIT_STATE_CHANGE
{
	DBG_STATE NewState;
	CLIENT_ID AppClientId;
	union
	{
		DBGUI_CREATE_THREAD CreateThread;
		DBGUI_CREATE_PROCESS CreateProcessInfo;
		DBGKM_EXIT_THREAD ExitThread;
		DBGKM_EXIT_PROCESS ExitProcess;
		DBGKM_EXCEPTION Exception;
		DBGKM_LOAD_DLL LoadDll;
		DBGKM_UNLOAD_DLL UnloadDll;
	} StateInfo;
} DBGUI_WAIT_STATE_CHANGE, * PDBGUI_WAIT_STATE_CHANGE;

typedef enum _DEBUG_OBJECT_INFORMATION_CLASS {
	DebugObjectFlagsInformation = 1,
	DebugObjectMaximumInfomation
}DEBUG_OBJECT_INFORMATION_CLASS, *PDEBUG_OBJECT_INFORMATION_CLASS;

#if defined(_AMD64_)
FORCEINLINE
VOID
ProbeForWriteUlong(
	IN PULONG Address
){
	if (Address >= (ULONG* const)MM_USER_PROBE_ADDRESS) {
		Address = (ULONG* const)MM_USER_PROBE_ADDRESS;
	}

	*((volatile ULONG*)Address) = *Address;
	return;
}
#else
#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}

#endif

#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForWriteHandle(
	IN PHANDLE Address
)

{

	if (Address >= (HANDLE* const)MM_USER_PROBE_ADDRESS) {
		Address = (HANDLE* const)MM_USER_PROBE_ADDRESS;
	}

	*((volatile HANDLE*)Address) = *Address;
	return;
}

#else

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

#endif

typedef enum _SYSTEM_DLL_TYPE  // 7 elements, 0x4 bytes
{
	PsNativeSystemDll = 0 /*0x0*/,
	PsWowX86SystemDll = 1 /*0x1*/,
	PsWowArm32SystemDll = 2 /*0x2*/,
	PsWowAmd64SystemDll = 3 /*0x3*/,
	PsWowChpeX86SystemDll = 4 /*0x4*/,
	PsVsmEnclaveRuntimeDll = 5 /*0x5*/,
	PsSystemDllTotalTypes = 6 /*0x6*/
}SYSTEM_DLL_TYPE, * PSYSTEM_DLL_TYPE;

typedef struct _PEB_LDR_DATA                            // 9 elements, 0x58 bytes (sizeof) 
{
	ULONG32      Length;
	UINT8        Initialized;
	VOID* SsHandle;
	struct _LIST_ENTRY InLoadOrderModuleList;          
	struct _LIST_ENTRY InMemoryOrderModuleList;        
	struct _LIST_ENTRY InInitializationOrderModuleList; 
	VOID* EntryInProgress;
	UINT8        ShutdownInProgress;
	VOID* ShutdownThreadId;
}PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef BOOLEAN(NTAPI* PLDR_INIT_ROUTINE)(
	_In_ PVOID DllHandle,
	_In_ ULONG Reason,
	_In_opt_ PVOID Context
	);

// symbols
typedef struct _LDR_SERVICE_TAG_RECORD
{
	struct _LDR_SERVICE_TAG_RECORD* Next;
	ULONG ServiceTag;
} LDR_SERVICE_TAG_RECORD, * PLDR_SERVICE_TAG_RECORD;

// symbols
typedef struct _LDRP_CSLIST
{
	PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, * PLDRP_CSLIST;


// symbols
typedef enum _LDR_DDAG_STATE
{
	LdrModulesMerged = -5,
	LdrModulesInitError = -4,
	LdrModulesSnapError = -3,
	LdrModulesUnloaded = -2,
	LdrModulesUnloading = -1,
	LdrModulesPlaceHolder = 0,
	LdrModulesMapping = 1,
	LdrModulesMapped = 2,
	LdrModulesWaitingForDependencies = 3,
	LdrModulesSnapping = 4,
	LdrModulesSnapped = 5,
	LdrModulesCondensed = 6,
	LdrModulesReadyToInit = 7,
	LdrModulesInitializing = 8,
	LdrModulesReadyToRun = 9
} LDR_DDAG_STATE;

// symbols
typedef struct _LDR_DDAG_NODE
{
	LIST_ENTRY Modules;
	PLDR_SERVICE_TAG_RECORD ServiceTagList;
	ULONG LoadCount;
	ULONG LoadWhileUnloadingCount;
	ULONG LowestLink;
	union
	{
		LDRP_CSLIST Dependencies;
		SINGLE_LIST_ENTRY RemovalLink;
	};
	LDRP_CSLIST IncomingDependencies;
	LDR_DDAG_STATE State;
	SINGLE_LIST_ENTRY CondenseLink;
	ULONG PreorderNumber;
} LDR_DDAG_NODE, * PLDR_DDAG_NODE;

// rev
typedef struct _LDR_DEPENDENCY_RECORD
{
	SINGLE_LIST_ENTRY DependencyLink;
	PLDR_DDAG_NODE DependencyNode;
	SINGLE_LIST_ENTRY IncomingDependencyLink;
	PLDR_DDAG_NODE IncomingDependencyNode;
} LDR_DEPENDENCY_RECORD, * PLDR_DEPENDENCY_RECORD;

// symbols
typedef enum _LDR_DLL_LOAD_REASON
{
	LoadReasonStaticDependency,
	LoadReasonStaticForwarderDependency,
	LoadReasonDynamicForwarderDependency,
	LoadReasonDelayloadDependency,
	LoadReasonDynamicLoad,
	LoadReasonAsImageLoad,
	LoadReasonAsDataLoad,
	LoadReasonEnclavePrimary, // REDSTONE3
	LoadReasonEnclaveDependency,
	LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON, * PLDR_DLL_LOAD_REASON;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	union
	{
		LIST_ENTRY InInitializationOrderLinks;
		LIST_ENTRY InProgressLinks;
	};
	PVOID DllBase;
	PLDR_INIT_ROUTINE EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	union
	{
		UCHAR FlagGroup[4];
		ULONG Flags;
		struct
		{
			ULONG PackagedBinary : 1;
			ULONG MarkedForRemoval : 1;
			ULONG ImageDll : 1;
			ULONG LoadNotificationsSent : 1;
			ULONG TelemetryEntryProcessed : 1;
			ULONG ProcessStaticImport : 1;
			ULONG InLegacyLists : 1;
			ULONG InIndexes : 1;
			ULONG ShimDll : 1;
			ULONG InExceptionTable : 1;
			ULONG ReservedFlags1 : 2;
			ULONG LoadInProgress : 1;
			ULONG LoadConfigProcessed : 1;
			ULONG EntryProcessed : 1;
			ULONG ProtectDelayLoad : 1;
			ULONG ReservedFlags3 : 2;
			ULONG DontCallForThreads : 1;
			ULONG ProcessAttachCalled : 1;
			ULONG ProcessAttachFailed : 1;
			ULONG CorDeferredValidate : 1;
			ULONG CorImage : 1;
			ULONG DontRelocate : 1;
			ULONG CorILOnly : 1;
			ULONG ChpeImage : 1;
			ULONG ReservedFlags5 : 2;
			ULONG Redirected : 1;
			ULONG ReservedFlags6 : 2;
			ULONG CompatDatabaseProcessed : 1;
		};
	};
	USHORT ObsoleteLoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	ULONG TimeDateStamp;
	struct _ACTIVATION_CONTEXT* EntryPointActivationContext;
	PVOID Lock; // RtlAcquireSRWLockExclusive
	PLDR_DDAG_NODE DdagNode;
	LIST_ENTRY NodeModuleLink;
	struct _LDRP_LOAD_CONTEXT* LoadContext;
	PVOID ParentDllBase;
	PVOID SwitchBackContext;
	RTL_BALANCED_NODE BaseAddressIndexNode;
	RTL_BALANCED_NODE MappingInfoIndexNode;
	ULONG_PTR OriginalBase;
	LARGE_INTEGER LoadTime;
	ULONG BaseNameHashValue;
	LDR_DLL_LOAD_REASON LoadReason;
	ULONG ImplicitPathOptions;
	ULONG ReferenceCount;
	ULONG DependentLoadFlags;
	UCHAR SigningLevel; // since REDSTONE2
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;


//
// Push lock definitions
//
typedef struct _EX_PUSH_LOCK_S {

	//
	// LOCK bit is set for both exclusive and shared acquires
	//
#define EX_PUSH_LOCK_LOCK_V          ((ULONG_PTR)0x0)
#define EX_PUSH_LOCK_LOCK            ((ULONG_PTR)0x1)

//
// Waiting bit designates that the pointer has chained waiters
//

#define EX_PUSH_LOCK_WAITING         ((ULONG_PTR)0x2)

//
// Waking bit designates that we are either traversing the list
// to wake threads or optimizing the list
//

#define EX_PUSH_LOCK_WAKING          ((ULONG_PTR)0x4)

//
// Set if the lock is held shared by multiple owners and there are waiters
//

#define EX_PUSH_LOCK_MULTIPLE_SHARED ((ULONG_PTR)0x8)

//
// Total shared Acquires are incremented using this
//
#define EX_PUSH_LOCK_SHARE_INC       ((ULONG_PTR)0x10)
#define EX_PUSH_LOCK_PTR_BITS        ((ULONG_PTR)0xf)

	union {
		struct {
			ULONG_PTR Locked : 1;
			ULONG_PTR Waiting : 1;
			ULONG_PTR Waking : 1;
			ULONG_PTR MultipleShared : 1;
			ULONG_PTR Shared : sizeof(ULONG_PTR) * 8 - 4;
		};
		ULONG_PTR Value;
		PVOID Ptr;
	};
} EX_PUSH_LOCK_S, * PEX_PUSH_LOCK_S;
/*
// like GetProcAddress
PVOID
RtlFindExportedRoutineByName (
	PVOID DllBase,
	PCHAR RoutineName);
*/


typedef struct _EX_FAST_REF {
	union {
		PVOID Object;
#if defined (_WIN64)
		ULONG_PTR RefCnt : 4;
#else
		ULONG_PTR RefCnt : 3;
#endif
		ULONG_PTR Value;
	};
} EX_FAST_REF, * PEX_FAST_REF;

typedef struct _PS_SYSTEM_DLL {
	//
	// _SECTION* object of the DLL.
	// Initialized at runtime by PspLocateSystemDll.
	//
	union {
		EX_FAST_REF SectionObjectFastRef;
		PVOID       SectionObject;
	};

	//
	// Push lock.
	//

	EX_PUSH_LOCK  PushLock;

	//
	// System DLL information.
	// This part is returned by PsQuerySystemDllInfo.
	//

	PS_SYSTEM_DLL_INFO SystemDllInfo;
}PS_SYSTEM_DLL, * PPS_SYSTEM_DLL;

typedef struct _WOW64_PROCESS {
	PVOID Wow64;
}WOW64_PROCESS,*PWOW64_PROCESS;

#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForReadSmallStructure(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG Alignment
)

/*++
Routine Description:
	Probes a structure for read access whose size is known at compile time.
	N.B. A NULL structure address is not allowed.
Arguments:
	Address - Supples a pointer to the structure.
	Size - Supplies the size of the structure.
	Alignment - Supplies the alignment of structure.
Return Value:
	None
--*/

{

	ASSERT((Alignment == 1) || (Alignment == 2) ||
		(Alignment == 4) || (Alignment == 8) ||
		(Alignment == 16));

	if ((Size == 0) || (Size >= 0x10000)) {

		ASSERT(0);

		ProbeForRead(Address, Size, Alignment);

	}
	else {
		if (((ULONG_PTR)Address & (Alignment - 1)) != 0) {
			ExRaiseDatatypeMisalignment();
		}

		if ((PUCHAR)Address >= (UCHAR* const)MM_USER_PROBE_ADDRESS) {
			Address = (UCHAR* const)MM_USER_PROBE_ADDRESS;
		}

		_ReadWriteBarrier();
		*(volatile UCHAR*)Address;
	}
}

#else

#define ProbeForReadSmallStructure(Address, Size, Alignment) {               \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8) ||                       \
           ((Alignment) == 16));                                             \
    if ((Size == 0) || (Size > 0x10000)) {                                   \
        ASSERT(0);                                                           \
        ProbeForRead(Address, Size, Alignment);                              \
    } else {                                                                 \
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {               \
            ExRaiseDatatypeMisalignment();                                   \
        }                                                                    \
        if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {      \
            *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;              \
        }                                                                    \
    }                                                                        \
}

#endif


FORCEINLINE
VOID
ProbeForWriteSmallStructure(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG Alignment
)

/*++
Routine Description:
	Probes a structure for write access whose size is known at compile time.
Arguments:
	Address - Supples a pointer to the structure.
	Size - Supplies the size of the structure.
	Alignment - Supplies the alignment of structure.
Return Value:
	None
--*/

{

	ASSERT((Alignment == 1) || (Alignment == 2) ||
		(Alignment == 4) || (Alignment == 8) ||
		(Alignment == 16));

	//
	// If the size of the structure is > 4k then call the standard routine.
	// wow64 uses a page size of 4k even on ia64.
	//

	if ((Size == 0) || (Size >= 0x1000)) {

		ASSERT(0);

		ProbeForWrite(Address, Size, Alignment);

	}
	else {
		if (((ULONG_PTR)(Address) & (Alignment - 1)) != 0) {
			ExRaiseDatatypeMisalignment();
		}

#if defined(_AMD64_)

		if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {
			Address = (UCHAR* const)MM_USER_PROBE_ADDRESS;
		}

		((volatile UCHAR*)(Address))[0] = ((volatile UCHAR*)(Address))[0];
		((volatile UCHAR*)(Address))[Size - 1] = ((volatile UCHAR*)(Address))[Size - 1];

#else

		if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {
			*((volatile UCHAR* const)MM_USER_PROBE_ADDRESS) = 0;
		}

		*(volatile UCHAR*)(Address) = *(volatile UCHAR*)(Address);
		if (Size > Alignment) {
			((volatile UCHAR*)(Address))[(Size - 1) & ~(SIZE_T)(Alignment - 1)] =
				((volatile UCHAR*)(Address))[(Size - 1) & ~(SIZE_T)(Alignment - 1)];
		}

#endif

	}
}