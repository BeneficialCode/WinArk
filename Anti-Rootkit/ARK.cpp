#include <ntifs.h>
#include <intrin.h>
// get errors for deprecated functions
#include <dontuse.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include"util.h"
#include"AntiRootkit.h"
#include "..\KernelLibrary\khook.h"
#include "..\KernelLibrary\SysMon.h"
#include "..\KernelLibrary\Logging.h"
#include "..\KernelLibrary\SysMonCommon.h"
#include "..\KernelLibrary\kDbgUtil.h"
#include "..\KernelLibrary\KernelTimer.h"
#include "..\KernelLibrary\IoTimer.h"
#include "..\KernelLibrary\BypassAntiKernelDbg.h"
#include "..\KernelLibrary\MiniFilter.h"
#include "..\KernelLibrary\VadHelpers.h"
#include "..\KernelLibrary\Helpers.h"
#include "..\KernelLibrary\WinExtHosts.h"
#include "..\KernelLibrary\FileManager.h"


// SE_IMAGE_SIGNATURE_TYPE

// define a tag (because of little endianess, viewed in PoolMon as 'arkv'
#define DRIVER_TAG 'vkra'


// {86F32D72-5D88-49D2-A44D-F632FF240C06}
TRACELOGGING_DEFINE_PROVIDER(g_Provider, "AntiRootkit",\
	(0x86f32d72, 0x5d88, 0x49d2, 0xa4, 0x4d, 0xf6, 0x32, 0xff, 0x24, 0xc, 0x6));

// PiUpdateDriverDBCache RtlInsertElementGenericTableAvl 可以确定该结构体的大小
// 此结构x86 x64通用
typedef struct _PiDDBCacheEntry
{
	LIST_ENTRY		List;
	UNICODE_STRING	DriverName;
	ULONG			TimeDateStamp;
	NTSTATUS		LoadStatus;
	char			_0x0028[16]; // data from the shim engine, or uninitialized memory for custom drivers
} PiDDBCacheEntry, * PPiDDBCacheEntry;

typedef struct _UNLOADED_DRIVERS {
	UNICODE_STRING Name;
	PVOID StartAddress;
	PVOID EndAddress;
	LARGE_INTEGER CurrentTime;
}UNLOADED_DRIVER, * PUNLOADED_DRIVER;


// 条件编译
#ifdef _WIN64
	// 64位环境
#else
	// 32位环境
#endif

UNICODE_STRING g_RegisterPath;
PDEVICE_OBJECT g_DeviceObject;
PDRIVER_OBJECT g_DriverObject;
FilterState g_State;

DRIVER_UNLOAD AntiRootkitUnload;
DRIVER_DISPATCH AntiRootkitDeviceControl, AntiRootkitCreateClose;
DRIVER_DISPATCH AntiRootkitRead, AntiRootkitWrite, AntiRootkitShutdown;



extern "C" NTSTATUS NTAPI ZwQueryInformationProcess(
	_In_ HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength
);

extern "C" NTSTATUS ZwOpenThread(
	_Out_ PHANDLE ThreadHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PCLIENT_ID ClientId);

extern "C" POBJECT_TYPE * IoDriverObjectType;

extern "C" PULONG InitSafeBootMode;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
	_In_ HANDLE ProcessHandle,
	_In_opt_ PVOID BaseAddress,
	_Out_writes_bytes_(BufferSize) PVOID Buffer,
	_In_ SIZE_T BufferSize,
	_Out_opt_ PSIZE_T NumberOfBytesRead
);

extern "C"
NTSYSAPI
NTSTATUS NTAPI ObReferenceObjectByName(
	_In_ PUNICODE_STRING ObjectPath,
	_In_ ULONG Attributes,
	_In_opt_ PACCESS_STATE PassedAccessState,
	_In_opt_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_TYPE ObjectType,
	_In_ KPROCESSOR_MODE AccessMode,
	_Inout_opt_ PVOID ParseContext,
	_Out_ PVOID * Object);

// 自身逻辑“最小化”
// 标识哪些逻辑属于关键逻辑，非关键逻辑
// 自身模块加载时，会遇到一些操作失败的情况，如创建线程失败，写入文件失败等。如果遇到一个小错误，
// 就认为全部执行失败，那么是不合理的。
// 系统处于低资源，中毒状态下，应保证驱动能在恶劣环境下，最大程度地完成逻辑处理
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	if (*InitSafeBootMode > 0) {
		// The operating system is in Safe Mode.
		return STATUS_NOT_SAFE_MODE_DRIVER;
	}

	// 驱动被自加载了，会出现没有实体的驱动对象对应
	if (DriverObject == nullptr) {
		return STATUS_UNSUCCESSFUL;
	}

	g_DriverObject = DriverObject;

	TraceLoggingRegister(g_Provider);

	TraceLoggingWrite(g_Provider, "DriverEntry started",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue("Ark Driver", "DriverName"),
		TraceLoggingUnicodeString(RegistryPath, "RegistryPath")
	);

	Log(LogLevel::Information, "DriverEntry called.Registry Path: %wZ\n", RegistryPath);

	RTL_OSVERSIONINFOW info;
	NTSTATUS status = RtlGetVersion(&info);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to get the version information (0x%08X)\n", status));
		return status;
	}

	InitializeListHead(&g_SysMonGlobals.ItemHead);
	g_SysMonGlobals.Mutex.Init();

	// Set an Unload routine
	DriverObject->DriverUnload = AntiRootkitUnload;
	// Set dispatch routine the driver supports
	// 试图访问时
	DriverObject->MajorFunction[IRP_MJ_CREATE] = AntiRootkitCreateClose;
	// 结束访问时
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = AntiRootkitCreateClose;
	// 设备控制请求
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AntiRootkitDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_READ] = AntiRootkitRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = AntiRootkitWrite;
	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = AntiRootkitShutdown;

	// After the DriverUnload is been initialized, this is very important!
	status = g_State.Lock.Init();
	if (!NT_SUCCESS(status))
		return status;

	
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\AntiRootkit");
	// 定义变量时初始化这些变量
	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\AntiRootkit");
	bool symLinkCreated = false;
	bool notificationRegistered = false;
	do
	{
		status = InitMiniFilter(DriverObject, RegistryPath);
		if (!NT_SUCCESS(status)) {
			LogError("Failed to init mini-filter (0x%X)\n", status);
			break;
		}

		// Create a device object
		status = IoCreateDevice(
			DriverObject,		// our driver object,
			0,					// no need for extra bytes
			&devName,			// the device name
			FILE_DEVICE_UNKNOWN,// device type
			0,					// characteristics flags,
			FALSE,				// not exclusive
			&DeviceObject		// the resulting pointer
		);

		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to create device object (0x%08X)\n", status));
			break;
		}

		// get I/O Manager's help
		// Large buffers may be expensive to copy
		//DeviceObject->Flags |= DO_BUFFERED_IO;

		// Large buffers
		//DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoRegisterShutdownNotification(DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to register shutdown notify (0x%08X)\n", status));
			break;
		}
		notificationRegistered = true;

		// Create a symbolic link to the device object
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			TraceLoggingWrite(g_Provider,
				"Error",
				TraceLoggingLevel(TRACE_LEVEL_ERROR),
				TraceLoggingValue("Symbolic link creation failed", "Message"),
				TraceLoggingNTStatus(status, "Status", "Returned status"));
			KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
			break;
		}

		symLinkCreated = true;

		g_DeviceObject = DeviceObject;

		status = FltStartFiltering(g_State.Filter);
		if (!NT_SUCCESS(status)) {
			break;
		}
		
		// 所有的资源申请，请考虑失败的情况下，会引发什么问题
		g_RegisterPath.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool,
			RegistryPath->Length, DRIVER_TAG);
		if (g_RegisterPath.Buffer == nullptr) {
			KdPrint(("Failed to allocate memory\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		g_RegisterPath.MaximumLength = RegistryPath->Length;
		RtlCopyUnicodeString(&g_RegisterPath, (PUNICODE_STRING)RegistryPath);
	} while (false);
	
	if (!NT_SUCCESS(status)) {
		g_State.Lock.Delete();
		if (g_State.Filter)
			FltUnregisterFilter(g_State.Filter);
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (notificationRegistered)
			IoUnregisterShutdownNotification(DeviceObject);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);

		return status;
	}

	g_State.DriverObject = DriverObject;

	// More generally, if DriverEntry returns any failure status,the Unload routine is not called.
	return status;
}

void AntiRootkitUnload(_In_ PDRIVER_OBJECT DriverObject) {
	ExFreePool(g_RegisterPath.Buffer);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\AntiRootkit");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);
	IoUnregisterShutdownNotification(g_DeviceObject);
	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("Driver Unload called\n"));
	TraceLoggingWrite(g_Provider, "Unload",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingString("Driver unloading", "Message"));
	TraceLoggingUnregister(g_Provider);
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
/*
	In more complex cases,these could be separate functions,
	where in the Create case the driver can (for instance) check to see who the caller is
	and only let approved callers succeed with opening a device.
*/
_Use_decl_annotations_
NTSTATUS AntiRootkitCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Log(LogLevel::Verbose, "Create/Close called\n");

	auto status = STATUS_SUCCESS;
	// 获取请求的当前栈空间
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	// 判断请求发给谁，是否是之前生成的控制设备
	if (DeviceObject != g_DeviceObject) {
		CompleteIrp(Irp);
		return status;
	}

	TraceLoggingWrite(g_Provider, "Create/Close",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingValue(stack->MajorFunction == IRP_MJ_CREATE ? "Create" : "Close", "Operation"));

	if (stack->MajorFunction == IRP_MJ_CREATE) {
		// verify it's WinArk client (very simple at the moment)
		HANDLE hProcess;
		status = ObOpenObjectByPointer(PsGetCurrentProcess(), OBJ_KERNEL_HANDLE, nullptr, 0, *PsProcessType, KernelMode, &hProcess);
		if (NT_SUCCESS(status)) {
			UCHAR buffer[280] = { 0 };
			status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, buffer, sizeof(buffer) - sizeof(WCHAR), nullptr);
			if (NT_SUCCESS(status)) {
				auto path = (UNICODE_STRING*)buffer;
				auto bs = wcsrchr(path->Buffer, L'\\');
				NT_ASSERT(bs);
				if (bs == nullptr || 0 != _wcsicmp(bs, L"\\WinArk.exe"))
					status = STATUS_ACCESS_DENIED;
			}
			ZwClose(hProcess);
		}
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;	// a polymorphic member, meaning different things in different request.
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

// 在处理缓冲区的时候，内核中的内存是有限的，要注意防止缓冲区溢出
//		1.限制缓冲区的长度
_Use_decl_annotations_
NTSTATUS AntiRootkitDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	// get our IO_STACK_LOCATION
	auto status = STATUS_INVALID_DEVICE_REQUEST;
	const auto& dic = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl;
	ULONG len = 0;

	switch (dic.IoControlCode) {
		case IOCTL_ARK_GET_SHADOW_SERVICE_TABLE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(void*)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			void* p = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (!MmIsAddressValid(p))
				break;
			SystemServiceTable* pSystemServiceTable = (SystemServiceTable*)p;
			// 获得输出缓冲区的长度
			if (dic.OutputBufferLength < sizeof(PULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			pSystemServiceTable += 1;
			khook::_win32kTable = pSystemServiceTable;
			*(PULONG*)Irp->AssociatedIrp.SystemBuffer = pSystemServiceTable->ServiceTableBase;
			len = sizeof(PULONG);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_SSDT_API_ADDR:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			// 获得输出缓冲区的长度
			if (dic.OutputBufferLength < sizeof(void*)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			PVOID address;
			ULONG number = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			bool success = khook::GetApiAddress(number,&address);
			if (success) {
				*(PVOID*)Irp->AssociatedIrp.SystemBuffer = address;
				len = sizeof(address);
				status = STATUS_SUCCESS;
			}
			break;
		}

		case IOCTL_ARK_GET_SHADOW_SSDT_API_ADDR:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			// 获得输出缓冲区的长度
			if (dic.OutputBufferLength < sizeof(void*)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			PVOID address;
			ULONG number = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			bool success = khook::GetShadowApiAddress(number, &address);
			if (success) {
				*(PVOID*)Irp->AssociatedIrp.SystemBuffer = address;
				len = sizeof(address);
				status = STATUS_SUCCESS;
			}
			break;
		}
		
		case IOCTL_ARK_OPEN_PROCESS:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			// 获取输入和输出缓冲区的长度
			if (dic.InputBufferLength < sizeof(OpenProcessThreadData) || dic.OutputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			// 获得缓冲区
			auto data = (OpenProcessThreadData*)Irp->AssociatedIrp.SystemBuffer;
			OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
			CLIENT_ID id{};
			id.UniqueProcess = UlongToHandle(data->Id);
			// 嵌套陷阱，ZwOpenProcess内部可能会执行第三方的回调函数，
			// 回调函数调用时，函数处于同一个线程，且共用一个线程栈，这里可能存在“栈溢出”的可能
			// 1.对于调用存在回调函数的api，尽可能少使用栈空间，大量内存，考虑申请内存
			// 2.对于自身执行在过滤驱动或回调函数中的代码，尽可能少使用栈空间
			// 3.自身执行在过滤驱动或回调函数中的代码，如需要调用嵌套的api,考虑另起线程，
			// 通过线程间通信把系统api调用的结果返回给最初线程。
			// 4.避免在代码中使用递归，如果非要使用，注意递归深度

			status = ZwOpenProcess((HANDLE*)data, data->AccessMask, &attr, &id);
			len = NT_SUCCESS(status) ? sizeof(HANDLE) : 0;
			break;
		}

		case IOCTL_ARK_GET_VERSION:
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			// 获得输出缓冲区的长度
			if (dic.OutputBufferLength < sizeof(USHORT)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			*(USHORT*)Irp->AssociatedIrp.SystemBuffer = DRIVER_CURRENT_VERSION;
			len = sizeof(USHORT);
			status = STATUS_SUCCESS;
			break;

		case IOCTL_ARK_SET_PRIORITY:
		{
			len = dic.InputBufferLength;
			if (len < sizeof(ThreadData)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			// 获得缓冲区
			auto data = (ThreadData*)dic.Type3InputBuffer;
			if (data == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			__try {
				if (data->Priority < 1 || data->Priority>31) {
					status = STATUS_INVALID_PARAMETER;
					break;
				}

				PETHREAD Thread;
				status = PsLookupThreadByThreadId(UlongToHandle(data->ThreadId),&Thread);
				if (!NT_SUCCESS(status))
					break;

				KeSetPriorityThread(Thread, data->Priority);
				ObDereferenceObject(Thread);

				KdPrint(("Thread Priority change for %d to %d succeeded!\n",
					data->ThreadId, data->Priority));
			} __except (GetExceptionCode() == STATUS_ACCESS_VIOLATION
				? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
				status = STATUS_ACCESS_VIOLATION;
			}
			break;
		}

		case IOCTL_ARK_DUP_HANDLE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(DupHandleData)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if (dic.OutputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			const auto data = static_cast<DupHandleData*>(Irp->AssociatedIrp.SystemBuffer);
			HANDLE hProcess;
			OBJECT_ATTRIBUTES procAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, OBJ_KERNEL_HANDLE);
			CLIENT_ID pid{};
			pid.UniqueProcess = UlongToHandle(data->SourcePid);
			status = ZwOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &procAttributes, &pid);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Failed to open process %d (0x%08X)\n", data->SourcePid, status));
				break;
			}

			HANDLE hTarget;
			status = ZwDuplicateObject(hProcess, ULongToHandle(data->Handle), NtCurrentProcess(),
				&hTarget, data->AccessMask, 0, data->Flags);
			ZwClose(hProcess);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Failed to duplicate handle (0x%8X)\n", status));
				break;
			}

			*(HANDLE*)Irp->AssociatedIrp.SystemBuffer = hTarget;
			len = sizeof(HANDLE);
			break;
		}

		case IOCTL_ARK_OPEN_THREAD:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(OpenProcessThreadData) || dic.OutputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto data = (OpenProcessThreadData*)Irp->AssociatedIrp.SystemBuffer;
			OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
			CLIENT_ID id{};
			id.UniqueThread = UlongToHandle(data->Id);
			status = ZwOpenThread((HANDLE*)data, data->AccessMask, &attr, &id);
			len = NT_SUCCESS(status) ? sizeof(HANDLE) : 0;
			break;
		}

		case IOCTL_ARK_OPEN_KEY:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			auto data = static_cast<KeyData*>(Irp->AssociatedIrp.SystemBuffer);
			if (dic.InputBufferLength < sizeof(KeyData) + ULONG((data->Length - 1) * 2)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if (dic.OutputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if (data->Length > 2048) {
				status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			UNICODE_STRING keyName;
			keyName.Buffer = data->Name;
			keyName.Length = keyName.MaximumLength = (USHORT)data->Length * sizeof(WCHAR);
			OBJECT_ATTRIBUTES keyAttr;
			InitializeObjectAttributes(&keyAttr, &keyName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
			HANDLE hKey{ nullptr };
			status = ZwOpenKey(&hKey, data->Access, &keyAttr);
			if (NT_SUCCESS(status)) {
				*(HANDLE*)data = hKey;
				len = sizeof(HANDLE);
			}
			break;
		}
		case IOCTL_ARK_INIT_NT_SERVICE_TABLE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			void* address = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (!MmIsAddressValid(address))
				break;
			SystemServiceTable* pServiceTable = (SystemServiceTable*)address;
			khook::_ntTable = pServiceTable;
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_SERVICE_LIMIT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			
			void* address = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (!MmIsAddressValid(address))
				break;
			SystemServiceTable* pServiceTable = (SystemServiceTable*)address;
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = pServiceTable->NumberOfServices;
			len = sizeof(ULONG);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_PROCESS_NOTIFY_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ProcessNotifyCountData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto info = (ProcessNotifyCountData*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = 0;
			if (info->pCount) {
				count = *info->pCount;
			}
			if (info->pExCount) {
				count += *info->pExCount;
				*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
				status = STATUS_SUCCESS;
				len = sizeof(count);
			}
			
			break;
		}
		case IOCTL_ARK_ENUM_IMAGELOAD_NOTIFY:
		case IOCTL_ARK_ENUM_THREAD_NOTIFY:
		case IOCTL_ARK_ENUM_PROCESS_NOTIFY:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (dic.InputBufferLength < sizeof(NotifyInfo)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto info = (NotifyInfo*)Irp->AssociatedIrp.SystemBuffer;
			if (dic.OutputBufferLength < sizeof(void*) * info->Count) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			
			EnumSystemNotify((PEX_CALLBACK)info->pRoutine, info->Count,(KernelCallbackInfo*)Irp->AssociatedIrp.SystemBuffer);

			status = STATUS_SUCCESS;
			len = dic.OutputBufferLength;
			break;
		}

		case IOCTL_ARK_GET_THREAD_NOTIFY_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ThreadNotifyCountData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto info = (ThreadNotifyCountData*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = 0;
			if (info->pCount) {
				count = *info->pCount;
			}
			if (info->pNonSystemCount) {
				count += *info->pNonSystemCount;
				*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
				status = STATUS_SUCCESS;
				len = sizeof(count);
			}
			break;
		}

		case IOCTL_ARK_GET_UNLOADED_DRIVERS_COUNT:
		case IOCTL_ARK_GET_IMAGE_NOTIFY_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PULONG)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto pCount = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = 0;
			if (pCount) {
				count = *pCount;
				*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
				status = STATUS_SUCCESS;
				len = sizeof(count);
			}
			break;
		}

		case IOCTL_ARK_ENUM_PIDDBCACHE_TABLE:
		{
			// RtlLookupElementGenericTableAvl()
			// RtlDeleteElementGenericTableAvl()
			// EtwpFreeKeyNameList
			// PiDDBCacheTable
			len = dic.InputBufferLength;
			if (len < sizeof(ULONG_PTR)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if (dic.Type3InputBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			PRTL_AVL_TABLE PiDDBCacheTable = nullptr;

			__try {
				// 获得输入缓冲区
				ULONG_PTR PiDDBCacheTableAddress = *(ULONG_PTR*)dic.Type3InputBuffer;
				// 获得输出缓冲区
				auto pData = (PiDDBCacheData*)Irp->UserBuffer;
				if (pData == nullptr) {
					status = STATUS_INVALID_PARAMETER;
					break;
				}
				ULONG size = dic.OutputBufferLength;
				LogInfo("PiDDBCacheTableAddress: %p\n", PiDDBCacheTableAddress);
				PiDDBCacheTable = (PRTL_AVL_TABLE)PiDDBCacheTableAddress;
				if (PiDDBCacheTable != nullptr) {
					for (auto p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, TRUE);
						p != nullptr;
						) {
						// Process the element pointed to by p
						PiDDBCacheEntry* entry = (PiDDBCacheEntry*)p;
						LogInfo("%wZ,%d,%d\n", &entry->DriverName, entry->LoadStatus, entry->TimeDateStamp);
						// 首先得放得下结构体
						if (size < sizeof(PiDDBCacheData)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}
						// 减小size part1
						size -= sizeof(PiDDBCacheData);
						// 填充信息
						pData->LoadStatus = entry->LoadStatus;
						pData->StringLen = entry->DriverName.Length;
						pData->TimeDateStamp = entry->TimeDateStamp;
						if (size < entry->DriverName.Length + sizeof(WCHAR)) {
							status = STATUS_INFO_LENGTH_MISMATCH;
							break;
						}
						// 字符串放在这个结构体后面
						pData->StringOffset = sizeof(PiDDBCacheData);
						auto pString = (WCHAR*)((PUCHAR)pData + pData->StringOffset);
						memcpy(pString, entry->DriverName.Buffer, entry->DriverName.Length);
						// 减小size part2
						size -= entry->DriverName.Length;
						// 减小size part3
						size -= sizeof(WCHAR);
						pString[entry->DriverName.Length] = L'\0';
						// 下一项的位置
						pData->NextEntryOffset = sizeof(PiDDBCacheData) + pData->StringLen + sizeof(WCHAR);

						p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, FALSE);
						// 更新指针
						if (p != nullptr)
							pData = (PiDDBCacheData*)((PUCHAR)pData + pData->NextEntryOffset);
					};

					pData->NextEntryOffset = 0;

					len = dic.OutputBufferLength;
					status = STATUS_SUCCESS;
				}
			}
			__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION
				? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
				status = STATUS_ACCESS_VIOLATION;
			}
			
			break;
		}

		case IOCTL_ARK_ENUM_UNLOADED_DRIVERS:
		{
			if (dic.Type3InputBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(UnloadedDriversInfo)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			__try {
				// 获得输入缓冲区
				auto info = (UnloadedDriversInfo*)dic.Type3InputBuffer;
				// 获得输出缓冲区
				auto pData = (UnloadedDriverData*)Irp->UserBuffer;
				if (pData == nullptr) {
					status = STATUS_INVALID_PARAMETER;
					break;
				}
				ULONG size = dic.OutputBufferLength;
				// MmUnloadedDrivers MmLastUnloadedDriver
				PUNLOADED_DRIVER pMmUnloadDrivers = nullptr;
				if (!MmIsAddressValid(info->pMmUnloadedDrivers)) {
					break;
				}
				pMmUnloadDrivers = *(PUNLOADED_DRIVER*)info->pMmUnloadedDrivers;
				LogInfo("MmUnloadDriversAddress %p\n", pMmUnloadDrivers);
				if (!MmIsAddressValid(pMmUnloadDrivers)) {
					break;
				}
				for (ULONG i = 0; i < info->Count; ) {
					LogInfo("%wZ,%p,%p,%X\n", &pMmUnloadDrivers[i].Name, pMmUnloadDrivers[i].StartAddress,
						pMmUnloadDrivers[i].EndAddress,
						pMmUnloadDrivers[i].CurrentTime);
					// 首先得放得下结构体
					if (size < sizeof(UnloadedDriverData)) {
						status = STATUS_INFO_LENGTH_MISMATCH;
						break;
					}
					// 减小size part1
					size -= sizeof(UnloadedDriverData);
					// 填充数据
					pData->CurrentTime = pMmUnloadDrivers[i].CurrentTime;
					pData->EndAddress = pMmUnloadDrivers[i].EndAddress;
					pData->StartAddress = pMmUnloadDrivers[i].StartAddress;
					pData->StringLen = pMmUnloadDrivers[i].Name.Length;
					// 检查是否能存放字符串
					if (size < pMmUnloadDrivers[i].Name.Length + sizeof(WCHAR)) {
						status = STATUS_INFO_LENGTH_MISMATCH;
						break;
					}
					// 存储字符串
					pData->StringOffset = sizeof(UnloadedDriverData);
					auto pString = (WCHAR*)((PUCHAR)pData + pData->StringOffset);
					memcpy(pString, pMmUnloadDrivers[i].Name.Buffer, pData->StringLen);
					// 减小size part2
					size -= pData->StringLen;
					// 减小size part3
					size -= sizeof(WCHAR);
					pString[pData->StringLen] = L'\0';

					// 下一项的位置
					pData->NextEntryOffset = sizeof(UnloadedDriverData) + pData->StringLen + sizeof(WCHAR);

					i++;
					// 检查是否需要更新指针
					if (i < info->Count)
						pData = (UnloadedDriverData*)((PUCHAR)pData + pData->NextEntryOffset);
				}
				pData->NextEntryOffset = 0;

				len = dic.OutputBufferLength;
				status = STATUS_SUCCESS;
			}
			__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION
				? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
				status = STATUS_ACCESS_VIOLATION;
			}
			
			break;
		}

		case IOCTL_ARK_GET_PIDDBCACHE_DATA_SIZE: 
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG_PTR)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG size = 0;
			PRTL_AVL_TABLE PiDDBCacheTable = nullptr;
			ULONG_PTR PiDDBCacheTableAddress = *(ULONG_PTR*)Irp->AssociatedIrp.SystemBuffer;
			PiDDBCacheTable = (PRTL_AVL_TABLE)PiDDBCacheTableAddress;
			//ULONG count = RtlNumberGenericTableElementsAvl(PiDDBCacheTable);
			if (PiDDBCacheTable != nullptr) {
				for (auto p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, TRUE);
					p != nullptr;
					p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, FALSE)) {
					// Process the element pointed to by p
					PiDDBCacheEntry* entry = (PiDDBCacheEntry*)p;
					
					// part1 结构体大小
					size += sizeof(PiDDBCacheData);
					// part2 字符串长度+'\0'
					size += entry->DriverName.Length + sizeof(WCHAR);
				};
			}

			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = size;
			status = STATUS_SUCCESS;
			len = sizeof(ULONG);
			break;
		}

		case IOCTL_ARK_GET_UNLOADED_DRIVERS_DATA_SIZE:
		{
			ULONG size = 0;
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(UnloadedDriversInfo)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto info = (UnloadedDriversInfo*)Irp->AssociatedIrp.SystemBuffer;
			// MmUnloadedDrivers MmLastUnloadedDriver
			PUNLOADED_DRIVER pMmUnloadDrivers = nullptr;
			pMmUnloadDrivers = *(PUNLOADED_DRIVER*)info->pMmUnloadedDrivers;
			if (!MmIsAddressValid(pMmUnloadDrivers)) {
				break;
			}
			for (ULONG i = 0; i < info->Count; i++) {
				// part1 结构体大小
				size += sizeof(UnloadedDriverData);
				// part2 字符串长度+L'\0'
				size += pMmUnloadDrivers[i].Name.Length + sizeof(WCHAR);
			}
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = size;
			status = STATUS_SUCCESS;
			len = sizeof(ULONG);
			break;
		}

		case IOCTL_ARK_ENUM_OBJECT_CALLBACK:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG offset = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			LONG count = GetObCallbackCount(offset);
			if (dic.OutputBufferLength < sizeof(ObCallbackInfo) * count) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			bool success = EnumObCallbackNotify(offset,(ObCallbackInfo*)Irp->AssociatedIrp.SystemBuffer);
			if (success) {
				len = dic.OutputBufferLength;
				status = STATUS_SUCCESS;
			}
			break;
		}

		case IOCTL_ARK_GET_OBJECT_CALLBACK_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(LONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG offset = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			LONG count = GetObCallbackCount(offset);
			*(LONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(LONG);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_DRIVER_OBJECT_ROUTINES:
		{
			PCWSTR driverName = static_cast<PCWSTR>(Irp->AssociatedIrp.SystemBuffer);
			auto inputLen = dic.InputBufferLength;
			if (driverName[inputLen / sizeof(WCHAR) - 1] != L'\0') {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			UNICODE_STRING name;
			RtlInitUnicodeString(&name, driverName);
			PDRIVER_OBJECT driver;
			status = ObReferenceObjectByName(&name, OBJ_CASE_INSENSITIVE, nullptr, 0,
				*IoDriverObjectType, KernelMode, nullptr, (PVOID*)&driver);
			if (!NT_SUCCESS(status))
				break;
			PDRIVER_DISPATCH* pRoutines = (PDRIVER_DISPATCH*)Irp->AssociatedIrp.SystemBuffer;
			for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
				pRoutines[i] = driver->MajorFunction[i];
			}
			ObDereferenceObject(driver);
			if (NT_SUCCESS(status)) {
				len = sizeof(PVOID) * IRP_MJ_MAXIMUM_FUNCTION;
			}
			break;
		}

		case IOCTL_ARK_OPEN_INTERCEPT_DRIVER_LOAD:
		{
			HANDLE hProcess;
			status = ObOpenObjectByPointer(PsGetCurrentProcess(), OBJ_KERNEL_HANDLE, nullptr, 0, *PsProcessType, KernelMode, &hProcess);
			if (NT_SUCCESS(status)) {
				UCHAR buffer[280] = { 0 };
				status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, buffer, sizeof(buffer) - sizeof(WCHAR), nullptr);
				ZwClose(hProcess);
				if (NT_SUCCESS(status)) {
					auto path = (UNICODE_STRING*)buffer;
					auto bs = wcsrchr(path->Buffer, L'\\');
					NT_ASSERT(bs);
					if (bs == nullptr) {
						status = STATUS_INVALID_PARAMETER;
						break;
					}
					*(bs + 1) = L'\0';
					g_BackupDir.MaximumLength = path->Length;
					g_BackupDir.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, g_BackupDir.MaximumLength, 'kuab');
					if (g_BackupDir.Buffer == nullptr) {
						status = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					RtlAppendUnicodeToString(&g_BackupDir, path->Buffer);
				}
			}
			status = g_sec.Initialize(SectionType::Native);
			if (!NT_SUCCESS(status)) {
				LogError("failed to Initialize sec (status=%08X)\n", status);
				break;
			}
#ifdef _WIN64
			status = g_secWow.Initialize(SectionType::Wow);
			if (!NT_SUCCESS(status)) {
				LogError("failed to Initialize g_secWow (status=%08X)\n", status);
				break;
			}
#endif // _WIN64

			status = PsSetLoadImageNotifyRoutine(OnImageLoadNotify);
			if (!NT_SUCCESS(status)) {
				LogError("failed to set image load callbacks (status=%08X)\n", status);
				break;
			}
			break;
		}

		case IOCTL_ARK_CLOSE_INTERCEPT_DRIVER_LOAD:
		{
			HANDLE hThread;
			status = PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, RemoveImageNotify, nullptr);
			if (!NT_SUCCESS(status)) {
				KdPrint(("PsCreateSystemThread failed!"));
				break;
			}
			status = ZwClose(hThread);
			break;
		}

		case IOCTL_ARK_REMOVE_KERNEL_NOTIFY:
		{
			//ULONG size = 0;
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(NotifyData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto pData = (NotifyData*)Irp->AssociatedIrp.SystemBuffer;
			status = RemoveSystemNotify(pData);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Remove kernel notify failed! %x",status));
				break;
			}
			break;
		}

		case IOCTL_ARK_GET_CM_CALLBACK_NOTIFY_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto pCount = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (!MmIsAddressValid(pCount)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			ULONG count = *pCount;
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(int);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_ENUM_CM_CALLBACK_NOTIFY:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			void* address = *(PVOID*)Irp->AssociatedIrp.SystemBuffer;
			bool success = EnumRegistryNotify((PLIST_ENTRY)address,(CmCallbackInfo*)Irp->AssociatedIrp.SystemBuffer);
			if (success) {
				len = dic.OutputBufferLength;
				status = STATUS_SUCCESS;
			}
			break;
		}

		case IOCTL_ARK_ENABLE_DBGSYS:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(DbgSysCoreInfo)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto pInfo = (DbgSysCoreInfo*)Irp->AssociatedIrp.SystemBuffer;
			bool success = kDbgUtil::InitDbgSys(pInfo);
			if (!success)
				break;
			len = 0;
			status = STATUS_SUCCESS;

			break;
		}

		case IOCTL_ARK_DISABLE_DBGSYS:
		{
			bool success = kDbgUtil::ExitDbgSys();
			len = 0;
			if (success)
				status = STATUS_SUCCESS;
			else
				status = STATUS_UNSUCCESSFUL;
			break;
		}

		case IOCTL_ARK_ENUM_KERNEL_TIMER:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(KernelTimerData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(DpcTimerInfo)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			KernelTimerData* pData = (KernelTimerData*)Irp->AssociatedIrp.SystemBuffer;
			KernelTimer::EnumKernelTimer(pData,(DpcTimerInfo*)Irp->AssociatedIrp.SystemBuffer);
			len = dic.OutputBufferLength;
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_KERNEL_TIMER_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(KernelTimerData)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			KernelTimerData* pData = (KernelTimerData*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = KernelTimer::GetKernelTimerCount(pData);
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(count);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_IO_TIMER_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			auto pCount = *(PULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (!MmIsAddressValid(pCount)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			ULONG count = *pCount;
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(count);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_ENUM_IO_TIMER:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(IoTimerData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(IoTimerInfo)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			IoTimerData* pData = (IoTimerData*)Irp->AssociatedIrp.SystemBuffer;
			IoTimer::EnumIoTimer(pData, (IoTimerInfo*)Irp->AssociatedIrp.SystemBuffer);
			len = dic.OutputBufferLength;
			status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_ARK_ENUM_MINIFILTER_OPERATIONS:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(MiniFilterData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(OperationInfo)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			MiniFilterData* pData = (MiniFilterData*)Irp->AssociatedIrp.SystemBuffer;
			status = EnumMiniFilterOperations(pData, (OperationInfo*)Irp->AssociatedIrp.SystemBuffer);
			len = dic.OutputBufferLength;
			break;
		}

		case IOCTL_ARK_BYPASS_DETECT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG flag = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (flag & BYPASS_KERNEL_DEBUGGER) {
				status = BypassAntiKernelDbg::Bypass();
			}
			break;
		}
		case IOCTL_ARK_UNBYPASS_DETECT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG flag = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			if (flag & BYPASS_KERNEL_DEBUGGER) {
				status = BypassAntiKernelDbg::Unbypass();
			}
			break;
		}

		case IOCTL_ARK_REMOVE_MINIFILTER:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(MiniFilterData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			MiniFilterData* pData = (MiniFilterData*)Irp->AssociatedIrp.SystemBuffer;
			status = RemoveMiniFilter(pData);
			break;
		}

		case IOCTL_ARK_DELPROTECT_SET_EXTENSIONS:
		{
			auto ext = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;
			auto inputLen = dic.InputBufferLength;
			if (ext == nullptr || inputLen < sizeof(WCHAR) * 2 
				|| ext[inputLen / sizeof(WCHAR) - 1] != 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (g_State.Extentions.MaximumLength < inputLen - sizeof(WCHAR)) {
				//
				// allocate a new buffer to hold the extensions
				//
				auto buffer = ExAllocatePoolWithTag(PagedPool, inputLen, DRIVER_TAG);
				if (buffer == nullptr) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				g_State.Extentions.MaximumLength = (USHORT)inputLen;
				//
				// free the old buffer
				//
				ExFreePool(g_State.Extentions.Buffer);
				g_State.Extentions.Buffer = (PWSTR)buffer;
			}
			UNICODE_STRING ustr;
			RtlInitUnicodeString(&ustr, ext);
			//
			// make sure the extensions are uppercase
			//
			RtlUpcaseUnicodeString(&ustr, &ustr, FALSE);
			memcpy(g_State.Extentions.Buffer, ext, len = inputLen);
			g_State.Extentions.Length = (USHORT)inputLen;
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_PROCESS_VAD_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(VadData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			VadData* pData = (VadData*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = 0;
			status = VadHelpers::GetVadCount(pData, &count);
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(count);
			break;
		}
		case IOCTL_ARK_GET_EPROCESS:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			// 获得输出缓冲区的长度
			if (dic.OutputBufferLength < sizeof(PEPROCESS)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			HANDLE pid = *(HANDLE*)Irp->AssociatedIrp.SystemBuffer;
			PEPROCESS Process;
			status = PsLookupProcessByProcessId(pid, &Process);
			if (NT_SUCCESS(status)) {
				ObDereferenceObject(Process);
				len = sizeof(PEPROCESS);
				*(PVOID*)Irp->AssociatedIrp.SystemBuffer = Process;
			}
			break;
		}
		case IOCTL_ARK_DUMP_SYS_MODULE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			auto data = static_cast<DumpSysData*>(Irp->AssociatedIrp.SystemBuffer);
			if (dic.InputBufferLength < sizeof(DumpSysData) + ULONG((data->Length - 1) * 2)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (data->Length > 2048) {
				status = STATUS_BUFFER_OVERFLOW;
				break;
			}
			status = Helpers::DumpSysModule(data);
			break;
		}

		case IOCTL_ARK_DUMP_KERNEL_MEM:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			auto data = static_cast<DumpMemData*>(Irp->AssociatedIrp.SystemBuffer);
			if (dic.InputBufferLength < sizeof(DumpMemData)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			if (dic.OutputBufferLength < data->Size) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			
			status = Helpers::DumpKernelMem(data, Irp->AssociatedIrp.SystemBuffer);

			if (NT_SUCCESS(status)) {
				len = dic.OutputBufferLength;
			}
			break;
		}

		case IOCTL_ARK_GET_WIN_EXT_HOSTS_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			PLIST_ENTRY pList = *(PLIST_ENTRY*)Irp->AssociatedIrp.SystemBuffer;
			ULONG count = 0;
			count = WinExtHosts::GetCount(pList);
			status = STATUS_SUCCESS;
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(count);
			break;
		}

		case IOCTL_ARK_ENUM_WIN_EXT_HOSTS:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			if (dic.OutputBufferLength < sizeof(WinExtHostInfo)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			PLIST_ENTRY pList = *(PLIST_ENTRY*)Irp->AssociatedIrp.SystemBuffer;
			bool success = WinExtHosts::Enum(pList, (WinExtHostInfo*)Irp->AssociatedIrp.SystemBuffer);
			if (success) {
				len = dic.OutputBufferLength;
				status = STATUS_SUCCESS;
			}
			break;
		}

		case IOCTL_ARK_ENUM_EXT_TABLE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ExtHostData)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			ExtHostData* pData = (ExtHostData*)Irp->AssociatedIrp.SystemBuffer;
			if (dic.OutputBufferLength < sizeof(void*)*pData->Count) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			bool success = WinExtHosts::EnumExtTable(pData, (PVOID*)Irp->AssociatedIrp.SystemBuffer);
			if (success) {
				len = dic.OutputBufferLength;
				status = STATUS_SUCCESS;
			}
			break;
		}

		case IOCTL_ARK_DETECT_KERNEL_INLINE_HOOK:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			ULONG count = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
			KernelInlineHookData* pData = (KernelInlineHookData*)Irp->AssociatedIrp.SystemBuffer;
			if (dic.OutputBufferLength < sizeof(KernelInlineHookData)*count) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			khook::DetectInlineHook(count, pData);
			len = dic.OutputBufferLength;
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_GET_KERNEL_INLINE_HOOK_COUNT:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.OutputBufferLength < sizeof(ULONG)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			ULONG count = khook::GetInlineHookCount();
			status = STATUS_SUCCESS;
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = count;
			len = sizeof(count);
			break;
		}

		case IOCTL_ARK_FORCE_DELETE_FILE:
		{
			auto name = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;
			if (!name) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			auto bufferLen = dic.InputBufferLength;
			if (bufferLen > 1024) {
				// just too long for a directory
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			// make sure there is a NULL terminator somewhere
			name[bufferLen / sizeof(WCHAR) - 1] = L'\0';

			auto dosNameLen = ::wcslen(name);
			if (dosNameLen < 3) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			UNICODE_STRING ntName;
			status = FileManager::ConvertDosNameToNtName(name, &ntName);
			LogInfo("Delete File %ws <=> %wZ\n", &name, ntName);

			status = FileManager::DeleteFile(&ntName);

			// An attempt has been made to remove a file or directory that cannot be deleted.
			if (status == STATUS_CANNOT_DELETE || status == STATUS_SHARING_VIOLATION) {
				FileManager fileMgr;
				status = fileMgr.ForceDeleteFile(name);
				if (!NT_SUCCESS(status)) {
					LogError("Force delete file failed 0x%x\n", status);
				}
			}
			// A file cannot be opened because the share access flags are incompatible.
			if (!NT_SUCCESS(status)) {
				LogError("Force delete file failed 0x%x\n", status);
			}
			ExFreePool(ntName.Buffer);
			break;
		}
		case IOCTL_ARK_DISABLE_DRIVER_LOAD:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(CiSymbols)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto pSym = (CiSymbols*)Irp->AssociatedIrp.SystemBuffer;
			bool success = DisableDriverLoad(pSym);
			if (!success)
				break;
			len = 0;
			status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_ARK_ENABLE_DRIVER_LOAD:
		{
			bool success = EnableDriverLoad();
			len = 0;
			if (success)
				status = STATUS_SUCCESS;
			else
				status = STATUS_UNSUCCESSFUL;
			break;
		}
		case IOCTL_ARK_START_LOG_DRIVER_HASH:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(CiSymbols)) {
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}
			auto pSym = (CiSymbols*)Irp->AssociatedIrp.SystemBuffer;
			bool success = StartLogDriverHash(pSym);
			if (!success)
				break;
			len = 0;
			status = STATUS_SUCCESS;
			break;
			break;
		}
		case IOCTL_ARK_STOP_LOG_DRIVER_HASH:
		{
			bool success = StopLogDriverHash();
			len = 0;
			if (success)
				status = STATUS_SUCCESS;
			else
				status = STATUS_UNSUCCESSFUL;
			break;
		}
		case IOCTL_ARK_GET_LEGO_NOTIFY_ROUTINE:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(PVOID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if (dic.OutputBufferLength < sizeof(PVOID)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			void* pPspLegoNotifyRoutine = *(PVOID*)Irp->AssociatedIrp.SystemBuffer;
			PHYSICAL_ADDRESS physical = MmGetPhysicalAddress(pPspLegoNotifyRoutine);
			if (!physical.QuadPart) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			PLEGO_NOTIFY_ROUTINE pRoutine = *(PLEGO_NOTIFY_ROUTINE*)pPspLegoNotifyRoutine;
			*(PLEGO_NOTIFY_ROUTINE*)Irp->AssociatedIrp.SystemBuffer = pRoutine;
			status = STATUS_SUCCESS;
			len = sizeof(PVOID);
			break;
		}
		case IOCTL_ARK_REMOVE_OB_CALLBACK:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG_PTR)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			POB_CALLBACK_ENTRY pCallbackEntry = *(POB_CALLBACK_ENTRY*)Irp->AssociatedIrp.SystemBuffer;
			bool success = RemoveObCallbackNotify(pCallbackEntry);
			if (success) {
				status = STATUS_SUCCESS;
			}
			else
				status = STATUS_UNSUCCESSFUL;
			break;
		}

		case IOCTL_ARK_DISABLE_OB_CALLBACK:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG_PTR)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			POB_CALLBACK_ENTRY pCallbackEntry = *(POB_CALLBACK_ENTRY*)Irp->AssociatedIrp.SystemBuffer;
			DisableObCallbackNotify(pCallbackEntry);
			status = STATUS_SUCCESS;
			break;
		}

		case IOCTL_ARK_ENABLE_OB_CALLBACK:
		{
			if (Irp->AssociatedIrp.SystemBuffer == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (dic.InputBufferLength < sizeof(ULONG_PTR)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			POB_CALLBACK_ENTRY pCallbackEntry = *(POB_CALLBACK_ENTRY*)Irp->AssociatedIrp.SystemBuffer;
			EnableObCallbackNotify(pCallbackEntry);
			status = STATUS_SUCCESS;
			break;
		}
	}


	CompleteIrp(Irp, status, len);
	return status;
}

NTSTATUS AntiRootkitShutdown(PDEVICE_OBJECT, PIRP Irp) {
	return CompleteIrp(Irp, STATUS_SUCCESS);
}

NTSTATUS AntiRootkitRead(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0)
		return CompleteIrp(Irp, STATUS_INVALID_PARAMETER);

	// DO_DIRECT_IO 访问例子
	//auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	//if (!buffer)
	//	return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);

	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}


NTSTATUS AntiRootkitWrite(PDEVICE_OBJECT, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Write.Length;

	do
	{
		if (len < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = static_cast<ThreadData*>(Irp->UserBuffer);
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		__try {
			if (data->Priority < 1 || data->Priority>31) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			PETHREAD thread;
			status = PsLookupThreadByThreadId(UlongToHandle(data->ThreadId), &thread);
			if (!NT_SUCCESS(status)) {
				break;
			}
			auto oldPriority = KeSetPriorityThread(thread, data->Priority);
			ObReferenceObject(thread);

			KdPrint(("Priority change for thread %d from %d to %d succeeded!\n", data->ThreadId,
				oldPriority, data->Priority));

			TraceLoggingWrite(g_Provider, "Boosting",
				TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
				TraceLoggingUInt32(data->ThreadId, "ThreadId"),
				TraceLoggingInt32(oldPriority, "OldPriority"),
				TraceLoggingInt32(data->Priority, "NewPriority"));

			len = sizeof(ThreadData);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			status = STATUS_ACCESS_VIOLATION;
		}
		
	} while (false);

	return CompleteIrp(Irp, status, len);
}




