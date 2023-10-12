#pragma once

#include "../KernelLibrary/Common.h"
/*
DeviceType - identifies a type of device. 
This can be one of the FILE_DEVICE_xxx constants defined in the WDK headers, 
but this is mostly for hardware based drivers. 
For software drivers like ours, the number doesn't matter much. 
Still, Microsoft's documentation specifies that values for 3rd parties should start with0x8000.
*/

/*
Function - an ascending number indicating a specific operation. 
If nothing else, this number must be different between different control codes for the same driver. 
Again, any number will do, but the official documentation says 3rd party drivers should start with 0x800.
*/

/*
Method - the most important part of the control code. 
It indicates how the input and output buffers provided by the client pass to the driver.
*/

/*
Access - indicates whether this operation is to the driver (FILE_WRITE_ACCESS), from the driver (FILE_READ_ACCESS), or both ways (FILE_ANY_ACCESS). 
Typical drivers just use FILE_ANY_ACCESS and deal with the actual request in the IRP_MJ_DEVICE_CONTROL handler.
*/

// 64bit和32bit内核开发差异
// 一、新增机制
//		1.Wow64子系统
//			文件重定向 以下除外
//				%windir%\System32\catroot
//				%windir%\System32\catroot2
//				%windir%\System32\drivers\etc
//				%windir%\System32\logfiles
//				%windir%\System32\spool
//			注册表重定向
//		2.PatchGuard技术
// 二、编程差异
//		1.内嵌汇编
//		2.预处理与条件编译
//		3.数据结构调整
//			如果数据结构里包含指针，thunking不会帮助转换需要使用固定长度的类型来定义变量

// 回调驱动 链接器 命令行 + -----> /integritycheck
#define ANTI_ROOTKIT_DEVICE 0x8000

#define DRIVER_CURRENT_VERSION 0xD9


// 用MDL锁定用户内存
// METHOD_OUT_DIRECT in: Irp->AssociatedIrp.SystemBuffer out: Irp->MdlAddress write
// METHOD_IN_DIRECT in: Irp->AssociatedIrp.SystemBuffer out: Irp->MdlAddress read
	
#define IOCTL_ARK_OPEN_OBJECT						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
// 缓冲方式
#define IOCTL_ARK_OPEN_PROCESS						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_VERSION						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DUP_HANDLE						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_OPEN_THREAD						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_OPEN_KEY							CTL_CODE(ANTI_ROOTKIT_DEVICE,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_SHADOW_SERVICE_TABLE			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_SSDT_API_ADDR					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_INIT_NT_SERVICE_TABLE				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_SHADOW_SSDT_API_ADDR			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_SERVICE_LIMIT					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80A,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_PROCESS_NOTIFY				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80B,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_PROCESS_NOTIFY_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80C,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_THREAD_NOTIFY_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80D,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_THREAD_NOTIFY				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80E,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_IMAGE_NOTIFY_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x80F,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_IMAGELOAD_NOTIFY				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x810,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_PIDDBCACHE_TABLE				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x811,METHOD_NEITHER,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_UNLOADED_DRIVERS				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x812,METHOD_NEITHER,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_UNLOADED_DRIVERS_COUNT		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x813,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_PIDDBCACHE_DATA_SIZE			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x814,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_UNLOADED_DRIVERS_DATA_SIZE	CTL_CODE(ANTI_ROOTKIT_DEVICE,0x815,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_OBJECT_CALLBACK				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x816,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_OBJECT_CALLBACK_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x817,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_DRIVER_OBJECT_ROUTINES		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x818,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_OPEN_INTERCEPT_DRIVER_LOAD		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x819,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_CLOSE_INTERCEPT_DRIVER_LOAD		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x820,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_REMOVE_KERNEL_NOTIFY				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x821,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_CM_CALLBACK_NOTIFY_COUNT		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x822,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_CM_CALLBACK_NOTIFY			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x823,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENABLE_DBGSYS						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x824,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DISABLE_DBGSYS					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x825,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_KERNEL_TIMER					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x826,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_KERNEL_TIMER_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x827,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_IO_TIMER_COUNT				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x828,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_IO_TIMER						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x829,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_MINIFILTER_OPERATIONS		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82A,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_BYPASS_DETECT						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82B,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_UNBYPASS_DETECT					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82C,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_REMOVE_MINIFILTER					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82D,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DELPROTECT_SET_EXTENSIONS			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82E,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_PROCESS_VAD_COUNT				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x82F,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_EPROCESS						CTL_CODE(ANTI_ROOTKIT_DEVICE,0x830,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DUMP_SYS_MODULE					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x831,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DUMP_KERNEL_MEM					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x832,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_WIN_EXT_HOSTS_COUNT			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x833,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_WIN_EXT_HOSTS				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x834,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENUM_EXT_TABLE					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x835,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DETECT_KERNEL_INLINE_HOOK			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x836,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_KERNEL_INLINE_HOOK_COUNT		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x837,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_FORCE_DELETE_FILE					CTL_CODE(ANTI_ROOTKIT_DEVICE,0x838,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DISABLE_DRIVER_LOAD				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x839,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_GET_LEGO_NOTIFY_ROUTINE			CTL_CODE(ANTI_ROOTKIT_DEVICE,0x83A,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENABLE_DRIVER_LOAD				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x840,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_START_LOG_DRIVER_HASH				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x841,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_STOP_LOG_DRIVER_HASH				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x842,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_REMOVE_OB_CALLBACK				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x843,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_DISABLE_OB_CALLBACK				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x844,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_ARK_ENABLE_OB_CALLBACK				CTL_CODE(ANTI_ROOTKIT_DEVICE,0x845,METHOD_BUFFERED,FILE_ANY_ACCESS)


// 原始方式
// METHOD_NEITHER in: DeviceIoControl.Type3InputBuffer out: Irp->UersBuffer
#define IOCTL_ARK_SET_PRIORITY		CTL_CODE(ANTI_ROOTKIT_DEVICE,0x803,METHOD_NEITHER,FILE_ANY_ACCESS)

#define BYPASS_KERNEL_DEBUGGER			(0x0001)




struct OpenObjectData {
	void* Address;
	ACCESS_MASK Access;
};

struct OpenProcessThreadData {
	ULONG Id;
	ACCESS_MASK AccessMask;
};

struct ThreadData {
	ULONG ThreadId;
	int Priority;
};

struct DupHandleData {
	ULONG Handle;
	ULONG SourcePid;
	ACCESS_MASK AccessMask;
	ULONG Flags;
};

struct KeyData {
	ULONG Length;
	ULONG Access;
	WCHAR Name[1];
};

struct ProcessNotifyCountData {
	PULONG pCount;
	PULONG pExCount;
};

struct NotifyInfo {
	void* pRoutine;
	ULONG Count;
};

struct KernelCallbackInfo {
	ULONG Count;
	void* Address[ANYSIZE_ARRAY];
};

struct ThreadNotifyCountData {
	PULONG pCount;
	PULONG pNonSystemCount;
};

struct UnloadedDriversInfo {
	void* pMmUnloadedDrivers;
	ULONG Count;
};

struct NameInfo {
	ULONG Offset;
	ULONG Length;
};

// I recommend you define a structure for user-mode that looks something like this:
struct PiDDBCacheData {
	USHORT NextEntryOffset;
	USHORT StringLen;
	USHORT StringOffset;
	ULONG TimeDateStamp;
	NTSTATUS LoadStatus;
};
// and the string characters would follow the structure in memory
// (the length and offset would point to where the string starts / ends)

struct UnloadedDriverData {
	USHORT NextEntryOffset;
	USHORT StringLen;
	USHORT StringOffset;
	PVOID StartAddress;
	PVOID EndAddress;
	LARGE_INTEGER CurrentTime;
};

enum class NotifyType {
	CreateProcessNotify = 0, 
	CreateThreadNotify, 
	LoadImageNotify,
	RegistryNotify
};

struct KernelNotifyInfo {
	NotifyType Type;
	ULONG Offset;
};

enum class ObjectCallbackType {
	Process, Thread, Desktop
};

struct ObCallbackInfo{
	PVOID RegistrationHandle;
	PVOID PreOperation;
	PVOID PostOperation;
	ObjectCallbackType Type;
	PVOID CallbackEntry;
	bool Enabled;
	ULONG Operations;
};

struct NotifyData {
	NotifyType Type;
	PVOID Address;
	LARGE_INTEGER Cookie;
	ULONG Offset;
};

struct CmCallbackInfo {
	LARGE_INTEGER Cookie;
	PVOID Address;
};

struct DbgSysCoreInfo {
	void* NtCreateDebugObjectAddress;
	void* DbgkDebugObjectTypeAddress;
	void* ZwProtectVirtualMemory;
	void* NtDebugActiveProcess;
	void* DbgkpPostFakeProcessCreateMessages;
	void* DbgkpSetProcessDebugObject;
	void* DbgkpPostModuleMessages;
	void* DbgkpPostFakeThreadMessages;
	void* PsGetNextProcessThread;
	void* DbgkpProcessDebugPortMutex;
	void* DbgkpWakeTarget;
	void* DbgkpMarkProcessPeb;
	void* DbgkpMaxModuleMsgs;
	void* MmGetFileNameForAddress;
	void* DbgkpSendApiMessage;
	void* DbgkpQueueMessage;
	void* KeThawAllThreads;
	void* DbgkpSectionToFileHandle;
	void* PsResumeThread;
	void* DbgkSendSystemDllMessages;
	void* PsSuspendThread;
	void* PsQuerySystemDllInfo;
	void* ObFastReferenceObject;
	void* ExfAcquirePushLockShared;
	void* ExfReleasePushLockShared;
	void* ObFastReferenceObjectLocked;
	void* PsGetNextProcess;
	void* DbgkpConvertKernelToUserStateChange;
	void* PsCaptureExceptionPort;
	void* DbgkpSendErrorMessage;
	void* DbgkpSendApiMessageLpc;
	void* DbgkpOpenHandles;
	void* DbgkpSuppressDbgMsg;
	void* ObFastDereferenceObject;
	void* DbgkCreateThread;
	void* DbgkExitThread;
	void* DbgkExitProcess;
	void* DbgkMapViewOfSection;
	void* DbgkUnMapViewOfSection;
	void* NtWaitForDebugEvent;
	void* NtDebugContinue;
	void* NtRemoveProcessDebug;
	void* DbgkForwardException;
	void* DbgkCopyProcessDebugPort;
	void* DbgkClearProcessDebugObject;
	void* NtSetInformationDebugObject;
	void* PsTerminateProcess;
	void* PspNotifyEnableMask;
	void* PspLoadImageNotifyRoutine;
	void* MiSectionControlArea;
	void* MiReferenceControlAreaFile;
	EProcessOffsets EprocessOffsets;
	EThreadOffsets EthreadOffsets;
	KThreadOffsets KthreadOffsets;
	PebOffsets PebOffsets;
};

struct ObPreOperationData {
	NotifyType Type;
	ULONG Offset;
	void* Address;
};


struct CiSymbols {
	void* MinCryptIsFileRevoked;
	void* I_MinCryptHashSearchCompare;
};