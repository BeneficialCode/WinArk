#include "pch.h"
#include "MiniFilter.h"
#include "RegistryKey.h"
#include "Logging.h"

#define DRIVER_TAG 'trpD'

NTSTATUS InitMiniFilter(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	g_State.Extentions.Buffer = nullptr;

	NTSTATUS status;
	RegistryKey key;
	HANDLE hKey = nullptr;
	do
	{

		OBJECT_ATTRIBUTES keyAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(RegistryPath, OBJ_KERNEL_HANDLE);
		status = ZwOpenKey(&hKey, KEY_WRITE, &keyAttr);
		if (!NT_SUCCESS(status))
			break;

		UNICODE_STRING subKeyName = RTL_CONSTANT_STRING(L"Instances");
		key.Create(hKey, &subKeyName, RegistryAccessMask::Write);
		if (!NT_SUCCESS(status))
			break;

		//
		// set "DefaultInstance" value. Any name is fine.
		// 
		UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"DefaultInstance");
		WCHAR name[] = L"DelProtectDefaultInstance";
		status = key.SetStringValue(&valueName, name);
		if (!NT_SUCCESS(status))
			break;

		//
		// create "instance" key under "Instances"
		//
		UNICODE_STRING instKeyName;
		RtlInitUnicodeString(&instKeyName, name);
		key.Create(key.Get(), &instKeyName, RegistryAccessMask::Write);
		if (!NT_SUCCESS(status))
			break;

		//
		// write out altitude
		//
		WCHAR altitude[] = L"425342";
		UNICODE_STRING altitudeName = RTL_CONSTANT_STRING(L"Altitude");
		key.SetStringValue(&altitudeName, altitude);
		if (!NT_SUCCESS(status))
			break;

		//
		// write out flags
		// 
		UNICODE_STRING flagsName = RTL_CONSTANT_STRING(L"Flags");
		ULONG flags = 0;
		key.SetDWORDValue(&flagsName, flags);
		if (!NT_SUCCESS(status))
			break;

		key.Close();

		FLT_OPERATION_REGISTRATION const callbacks[] = {
			{ IRP_MJ_CREATE, 0, DelProtectPreCreate, nullptr },
			{ IRP_MJ_SET_INFORMATION, 0, DelProtectPreSetInformation, nullptr },
			{ IRP_MJ_OPERATION_END }
		};

		FLT_REGISTRATION const reg = {
			sizeof(FLT_REGISTRATION),
			FLT_REGISTRATION_VERSION,
			0,					// Flags
			nullptr,			// Context
			callbacks,			// Operation callbacks
			DelProtectUnload,					//  MiniFilterUnload
			DelProtectInstanceSetup,            //  InstanceSetup
			DelProtectInstanceQueryTeardown,    //  InstanceQueryTeardown
			DelProtectInstanceTeardownStart,    //  InstanceTeardownStart
			DelProtectInstanceTeardownComplete, //  InstanceTeardownComplete
		};

		status = FltRegisterFilter(DriverObject, &reg, &g_State.Filter);
	} while (false);

	if (!NT_SUCCESS(status))
		key.DeleteKey();

	if (hKey)
		ZwClose(hKey);

	return status;
}

NTSTATUS DelProtectUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);

	FltUnregisterFilter(g_State.Filter);
	g_State.Lock.Delete();
	if (g_State.Extentions.Buffer != nullptr)
		ExFreePool(g_State.Extentions.Buffer);

	return STATUS_SUCCESS;
}

NTSTATUS DelProtectInstanceSetup(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags,
	DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE VolumeFilesystemType) {
	KdPrint((DRIVER_PREFIX "DelProtectInstanceSetup FS: %u\n", VolumeFilesystemType));

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);

	return VolumeFilesystemType == FLT_FSTYPE_NTFS ? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
}

NTSTATUS DelProtectInstanceQueryTeardown(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	return STATUS_SUCCESS;
}

VOID DelProtectInstanceTeardownStart(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

VOID DelProtectInstanceTeardownComplete(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_TEARDOWN_FLAGS Flags) {
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
}

FLT_PREOP_CALLBACK_STATUS DelProtectPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*) {
	UNREFERENCED_PARAMETER(FltObjects);

	if (Data->RequestorMode == KernelMode)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	auto const& params = Data->Iopb->Parameters.Create;
	auto status = FLT_PREOP_SUCCESS_NO_CALLBACK;

	if (params.Options & FILE_DELETE_ON_CLOSE) {
		// delete operation
		PUNICODE_STRING filename = &FltObjects->FileObject->FileName;
		LogInfo("Delete on close: %wZ\n", filename);

		if (!IsDeleteAllowed(filename)) {
			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			status = FLT_PREOP_COMPLETE;
			LogInfo("(Pre Create) Prevent deletion of %wZ\n", filename);
		}
	}
	return status;
}

FLT_PREOP_CALLBACK_STATUS DelProtectPreSetInformation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID*) {
	UNREFERENCED_PARAMETER(FltObjects);

	if (Data->RequestorMode == KernelMode)
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	auto status = FLT_PREOP_SUCCESS_NO_CALLBACK;
	auto& params = Data->Iopb->Parameters.SetFileInformation;
	if (params.FileInformationClass == FileDispositionInformation ||
		params.FileInformationClass == FileDispositionInformationEx) {
		auto info = (FILE_DISPOSITION_INFORMATION*)params.InfoBuffer;
		if (info->DeleteFile & FILE_DISPOSITION_DELETE) {// also covers FileDispositionInformationEx
			PFLT_FILE_NAME_INFORMATION fi;
			//
			// using FLT_FILE_NAME_NORMALIZED is important here for parsing purposes
			//
			if (NT_SUCCESS(FltGetFileNameInformation(Data, 
				FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_NORMALIZED,
				&fi))) {
				if (!IsDeleteAllowed(&fi->Name)) {
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					LogInfo("(Pre Set Information) Prevent deletion of %wZ\n", &fi->Name);
					status = FLT_PREOP_COMPLETE;
				}
				FltReleaseFileNameInformation(fi);
			}
		}
	}

	return status;
}

bool IsDeleteAllowed(PCUNICODE_STRING filename) {
	UNICODE_STRING ext;
	NTSTATUS status;
	status = FltParseFileName(filename, &ext, nullptr, nullptr);
	if(NT_SUCCESS(status)) {
		if (ext.Buffer == nullptr || ext.Length == 0 
			||g_State.Extentions.Buffer == nullptr) {
			return true;
		}
		UNICODE_STRING suext;
		//
		// allocate space for NULL terminator and a semicolon
		//
		ULONG maxLength = ext.MaximumLength + sizeof(WCHAR) * 2;
		suext.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, maxLength, DRIVER_TAG);
		RtlZeroMemory(suext.Buffer, maxLength);
		suext.MaximumLength = maxLength;
		suext.Length = ext.Length;

		RtlUpcaseUnicodeString(&suext, &ext, FALSE);
		RtlAppendUnicodeToString(&suext, L";");

		//
		// search for the prefix
		//
		bool del =  wcsstr(g_State.Extentions.Buffer, suext.Buffer) == nullptr;
		
		if (suext.Buffer != nullptr) {
			ExFreePool(suext.Buffer);
		}

		return del;
	}

	return true;
}