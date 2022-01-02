#include <ntifs.h>
#include <intrin.h>
// get errors for deprecated functions
#include <dontuse.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include"util.h"
#include"AntiRootkit.h"
#include"RegManager.h"
#include"ProcManager.h"
#include "..\KernelLibrary\khook.h"
#include "..\KernelLibrary\SysMon.h"
#include "..\KernelLibrary\Logging.h"



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

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByName(
	_In_ PUNICODE_STRING ObjectName,
	_In_ ULONG Attributes,
	_In_opt_ PACCESS_STATE AccessState,
	_In_ POBJECT_TYPE ObjectType,
	_In_ KPROCESSOR_MODE AccessMode,
	_Inout_opt_ PVOID ParseContext,
	_Out_ PVOID *Object
);

// 自身逻辑“最小化”
// 标识哪些逻辑属于关键逻辑，非关键逻辑
// 自身模块加载时，会遇到一些操作失败的情况，如创建线程失败，写入文件失败等。如果遇到一个小错误，
// 就认为全部执行失败，那么是不合理的。
// 系统处于低资源，中毒状态下，应保证驱动能在恶劣环境下，最大程度地完成逻辑处理
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	// 驱动被自加载了，会出现没有实体的驱动对象对应
	if (DriverObject == nullptr) {
		return STATUS_UNSUCCESSFUL;
	}

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

	// Create a device object
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\AntiRootkit");
	// 定义变量时初始化这些变量
	PDEVICE_OBJECT DeviceObject = nullptr;

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
		return status;
	}

	// get I/O Manager's help
	// Large buffers may be expensive to copy
	//DeviceObject->Flags |= DO_BUFFERED_IO;

	// Large buffers
	//DeviceObject->Flags |= DO_DIRECT_IO;
	
	status = IoRegisterShutdownNotification(DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to register shutdown notify (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject); // important!
		return status;
	}
	
	// Create a symbolic link to the device object
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\AntiRootkit");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		TraceLoggingWrite(g_Provider,
			"Error",
			TraceLoggingLevel(TRACE_LEVEL_ERROR),
			TraceLoggingValue("Symbolic link creation failed", "Message"),
			TraceLoggingNTStatus(status, "Status", "Returned status"));

		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	g_DeviceObject = DeviceObject;

	// 所有的资源申请，请考虑失败的情况下，会引发什么问题
	g_RegisterPath.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool,
		RegistryPath->Length, DRIVER_TAG);
	if (g_RegisterPath.Buffer == nullptr) {
		KdPrint(("Failed to allocate memory\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	g_RegisterPath.MaximumLength = RegistryPath->Length;
	RtlCopyUnicodeString(&g_RegisterPath, (PUNICODE_STRING)RegistryPath);

	//KdPrint(("Copied registry path: %wZ\n", &g_RegisterPath));

	//test();
	// More generally, if DriverEntry returns any failure status,the Unload routine is not called.
	return STATUS_SUCCESS;
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
	IoCompleteRequest(Irp, 0);
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
			
			EnumProcessNotify((PEX_CALLBACK)info->pRoutine, info->Count,(KernelCallbackInfo*)Irp->AssociatedIrp.SystemBuffer);
			status = STATUS_SUCCESS;
			len = sizeof(void*) * info->Count + sizeof(ULONG);
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
						LogInfo("%wZ,%d,%d\n", entry->DriverName, entry->LoadStatus, entry->TimeDateStamp);
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
				PUNLOADED_DRIVER MmUnloadDrivers = nullptr;
				MmUnloadDrivers = *(PUNLOADED_DRIVER*)info->pMmUnloadedDrivers;
				LogInfo("MmUnloadDriversAddress %p\n", MmUnloadDrivers);
				for (ULONG i = 0; i < info->Count; ) {
					LogInfo("%wZ,%p,%p,%X\n", MmUnloadDrivers[i].Name, MmUnloadDrivers[i].StartAddress,
						MmUnloadDrivers[i].EndAddress,
						MmUnloadDrivers[i].CurrentTime);
					// 首先得放得下结构体
					if (size < sizeof(UnloadedDriverData)) {
						status = STATUS_INFO_LENGTH_MISMATCH;
						break;
					}
					// 减小size part1
					size -= sizeof(UnloadedDriverData);
					// 填充数据
					pData->CurrentTime = MmUnloadDrivers[i].CurrentTime;
					pData->EndAddress = MmUnloadDrivers[i].EndAddress;
					pData->StartAddress = MmUnloadDrivers[i].StartAddress;
					pData->StringLen = MmUnloadDrivers[i].Name.Length;
					// 检查是否能存放字符串
					if (size < MmUnloadDrivers[i].Name.Length + sizeof(WCHAR)) {
						status = STATUS_INFO_LENGTH_MISMATCH;
						break;
					}
					// 存储字符串
					pData->StringOffset = sizeof(UnloadedDriverData);
					auto pString = (WCHAR*)((PUCHAR)pData + pData->StringOffset);
					memcpy(pString, MmUnloadDrivers[i].Name.Buffer, pData->StringLen);
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
			ULONG count = RtlNumberGenericTableElementsAvl(PiDDBCacheTable);
			if (PiDDBCacheTable != nullptr) {
				for (auto p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, TRUE);
					p != nullptr;
					p = RtlEnumerateGenericTableAvl(PiDDBCacheTable, FALSE)) {
					// Process the element pointed to by p
					PiDDBCacheEntry* entry = (PiDDBCacheEntry*)p;
					//LogInfo("%wZ,%d,%d\n", entry->DriverName, entry->LoadStatus, entry->TimeDateStamp);
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
			PUNLOADED_DRIVER MmUnloadDrivers = nullptr;
			MmUnloadDrivers = *(PUNLOADED_DRIVER*)info->pMmUnloadedDrivers;
			for (ULONG i = 0; i < info->Count; i++) {
				// part1 结构体大小
				size += sizeof(UnloadedDriverData);
				// part2 字符串长度+L'\0'
				size += MmUnloadDrivers[i].Name.Length + sizeof(WCHAR);
			}
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = size;
			status = STATUS_SUCCESS;
			len = sizeof(ULONG);
			break;
		}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
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

KEVENT kEvent;

KSTART_ROUTINE MyThreadFunc;
void MyThreadFunc(IN PVOID context) {
	PUNICODE_STRING str = (PUNICODE_STRING)context;
	KdPrint(("Kernel thread running: %wZ\n", str));
	MyGetCurrentTime();
	KdPrint(("Wait 3s!\n"));
	MySleep(3000);
	MyGetCurrentTime();
	KdPrint(("Kernel thread exit!\n"));
	KeSetEvent(&kEvent, 0, true);
	PsTerminateSystemThread(STATUS_SUCCESS);
}

void CreateThreadTest() {
	HANDLE hThread;
	UNICODE_STRING ustrTest = RTL_CONSTANT_STRING(L"This is a string for test!");
	NTSTATUS status;

	KeInitializeEvent(&kEvent, 
		SynchronizationEvent,	// when this event is set, 
		// it releases at most one thread (auto reset)
		FALSE);

	status = PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, MyThreadFunc, (PVOID)&ustrTest);
	if (!NT_SUCCESS(status)) {
		KdPrint(("PsCreateSystemThread failed!"));
		return;
	}
	ZwClose(hThread);
	KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
	KdPrint(("CreateThreadTest over!\n"));
}

void test() {
	//ULONG ul = 1234, ul0 = 0;
	//PKEY_VALUE_PARTIAL_INFORMATION pkvi;
	//RegCreateKey(L"\\Registry\\Machine\\Software\\AppDataLow\\Tencent\\{61B942F7-A946-4585-B624-B2C0228FFE8C}");
	//RegSetValueKey(L"\\Registry\\Machine\\Software\\AppDataLow\\Tencent\\{61B942F7-A946-4585-B624-B2C0228FFE8C}", L"key", REG_DWORD, &ul, sizeof(ul));
	//RegQueryValueKey(L"\\Registry\\Machine\\Software\\AppDataLow\\Tencent\\{61B942F7-A946-4585-B624-B2C0228FFE8C}", L"key", &pkvi);
	EnumSubKeyTest();
	/*memcpy(&ul0, pkvi->Data, pkvi->DataLength);
	KdPrint(("key: %d\n", ul0));*/
	/*ExFreePool(pkvi);*/
}




