#include "pch.h"
#include "DevMonManager.h"
#include "AutoLock.h"
#include "Memory.h"


DriverMonGlobals globals;

void DevMonManager::Init(PDRIVER_OBJECT driver) {
	Lock.Init();
	DriverObject = driver;
}

NTSTATUS DevMonManager::AddDevice(PCWSTR name) {
	AutoLock locker(Lock);
	if (MonitoredDeviceCount == MaxMonitoredDevices)
		return STATUS_TOO_MANY_NAMES;

	for (int i = 0; i < MaxMonitoredDevices; i++) {
		if (Devices[i].DeviceObject == nullptr) {
			UNICODE_STRING targetName;
			RtlInitUnicodeString(&targetName, name);

			PFILE_OBJECT FileObject;
			PDEVICE_OBJECT LowerDeviceObject = nullptr;
			// 打开设备对象 Usually FILE_READ_DATA  Infrequently FILE_WRITE_DATA or FILE_ALL_ACCESS
			auto status = IoGetDeviceObjectPointer(&targetName, FILE_ALL_ACCESS,
				&FileObject, &LowerDeviceObject);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Failed to get device object pointer (%ws) (0x%08X)\n", name, status));
				return status;
			}

			PDEVICE_OBJECT DeviceObject = nullptr;
			WCHAR* buffer = nullptr;

			// 生成设备，然后绑定之
			do
			{
				// A name is not needed,because the target device is named and is the real target for IRPs.
				status = IoCreateDevice(DriverObject, sizeof(DeviceExtension), nullptr,
					LowerDeviceObject->DeviceType, 0, FALSE, &DeviceObject);
				if (!NT_SUCCESS(status))
					break;
				// allocate buffer to copy device name
				buffer = (WCHAR*)ExAllocatePool(PagedPool, targetName.Length);
				if (!buffer) {
					status = STATUS_INSUFF_SERVER_RESOURCES;
					break;
				}

				auto ext = (DeviceExtension*)DeviceObject->DeviceExtension;

				// 最大锁定的分钟数 为 0 表示无限制
				// 最大为解决请求数 为 0 表示无限制
				IoInitializeRemoveLock(&ext->RemoveLock, 'kcol', 0, 0);
				// 拷贝重要标准位
				// We must prepare our new device object fully before actually attaching.
				DeviceObject->Flags |= LowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);
				DeviceObject->DeviceType = LowerDeviceObject->DeviceType;
				DeviceObject->Characteristics |= LowerDeviceObject->Characteristics & (FILE_DEVICE_SECURE_OPEN);

				Devices[i].DeviceName.Buffer = buffer;
				Devices[i].DeviceName.MaximumLength = targetName.Length;
				RtlCopyUnicodeString(&Devices[i].DeviceName, &targetName);
				Devices[i].DeviceObject = DeviceObject;

				// 将一个设备绑定到另一个设备上
				status = IoAttachDeviceToDeviceStackSafe(
					DeviceObject,		// filter device object
					LowerDeviceObject,	// target device object
					&ext->LowerDeviceObject);// result
				if (!NT_SUCCESS(status))
					break;

				Devices[i].LowerDeviceObject = ext->LowerDeviceObject;
				// hardware based devices require this

				// we remove the DO_DEVICE_INITIALIZING flag (set by the I/O system initially) to
				// indicate to the Plug&Play manager that the device is ready for work.
				// 设置这个设备已经启动
				DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
				// The DO_POWER_PAGABLE flag indicates Power IRPs should arrive in IRQL < DISPATCH_LEVEL, and is in fact mandatory.
				DeviceObject->Flags |= DO_POWER_PAGABLE;

				MonitoredDeviceCount++;
			} while (false);

			if (!NT_SUCCESS(status)) {
				if (buffer)
					ExFreePool(buffer);
				if (DeviceObject)
					IoDeleteDevice(DeviceObject); // 绑定失败，删除设备
				Devices[i].DeviceObject = nullptr;
			}
			if (LowerDeviceObject) {
				// dereference - not needed anymore
				ObDereferenceObject(FileObject);
			}
			return status;
		}
	}

	// should never get here
	NT_ASSERT(false);
	return STATUS_UNSUCCESSFUL;
}

bool DevMonManager::RemoveDevice(PCWSTR name) {
	AutoLock locker(Lock);
	int index = FindDevice(name);
	if (index < 0)
		return false;

	return RemoveDevice(index);
}

int DevMonManager::FindDevice(PDEVICE_OBJECT DeviceObject) {
	for (int i = 0; i < MaxMonitoredDevices; i++) {
		auto& device = Devices[i];
		if (device.DeviceObject && device.DeviceObject == DeviceObject) {
			return i;
		}
	}
	return -1;
}

int DevMonManager::FindDevice(PCWSTR name) {
	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, name);
	for (int i = 0; i < MaxMonitoredDevices; i++) {
		auto& device = Devices[i];
		if (device.DeviceObject &&
			RtlEqualUnicodeString(&device.DeviceName, &uname, TRUE)) {
			return i;
		}
	}
	return -1;
}

bool DevMonManager::RemoveDevice(PDEVICE_OBJECT LowerDeviceObject) {
	AutoLock locker(Lock);
	int index = FindDevice(LowerDeviceObject);
	if (index < 0)
		return false;
	return RemoveDevice(index);
}

bool DevMonManager::RemoveDevice(int index) {
	auto& device = Devices[index];
	if (device.DeviceObject == nullptr)
		return false;

	ExFreePool(device.DeviceName.Buffer);
	IoDetachDevice(device.LowerDeviceObject);
	IoDeleteDevice(device.DeviceObject);
	device.DeviceObject = nullptr;

	MonitoredDeviceCount--;
	return true;
}

void DevMonManager::RemoveAllDevices() {
	AutoLock locker(Lock);
	for (int i = 0; i < MaxMonitoredDevices; i++)
		RemoveDevice(i);
}

NTSTATUS DevMonManager::AddDriver(PCWSTR driverName, PVOID* driverObject) {
	int index = -1;
	//
	// find first available slot,make sure driver is not already monitored
	//

	for (int i = 0; i < MaxMonitoredDrivers; ++i) {
		if (globals.Drivers[i].DriverObject == nullptr) {
			if (index < 0)
				index = i;
		}
		else {
			// existing driver,check if not already being monitored
			if (_wcsicmp(globals.Drivers[i].DriverName, driverName) == 0) {
				*driverObject = globals.Drivers[i].DriverObject;
				return STATUS_SUCCESS;
			}
		}
	}

	UNICODE_STRING name;
	RtlInitUnicodeString(&name, driverName);
	PDRIVER_OBJECT driver;
	auto status = ObReferenceObjectByName(&name, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, nullptr, (PVOID*)&driver);
	if (!NT_SUCCESS(status))
		return status;

	::wcscpy_s(globals.Drivers[index].DriverName, driverName);

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		globals.Drivers[i].MajorFunction[i] =
			static_cast<PDRIVER_DISPATCH>(InterlockedExchangePointer((PVOID*)&driver->MajorFunction[i], DriverMonGenericDispatch));
	}

	globals.Drivers[index].DriverUnload =
		static_cast<PDRIVER_UNLOAD>(InterlockedExchangePointer((PVOID*)&driver->DriverUnload, GenericDriverUnload));
	globals.Drivers[index].DriverObject = driver;
	++globals.Count;
	*driverObject = driverObject;

	return STATUS_SUCCESS;
}

NTSTATUS DevMonManager::RemoveDriver(int index) {
	auto& driver = globals.Drivers[index];
	for (int j = 0; j <= IRP_MJ_MAXIMUM_FUNCTION; j++) {
		InterlockedExchangePointer((PVOID*)&driver.DriverObject->MajorFunction[j], driver.MajorFunction[j]);
	}
	InterlockedExchangePointer((PVOID*)&driver.DriverObject->DriverUnload, driver.DriverUnload);

	globals.Count--;
	ObDereferenceObject(driver.DriverObject);
	driver.DriverObject = nullptr;

	return STATUS_SUCCESS;
}

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, IrpMajorCode code, PVOID buffer, ULONG size, bool output) {
	__try {
		switch (code) {
		case IrpMajorCode::READ:
		case IrpMajorCode::WRITE:
			if (Irp->MdlAddress) {
				auto p = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
				if (p) {
					::memcpy(buffer, p, size);
					return STATUS_SUCCESS;
				}
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			if (DeviceObject->Flags & DO_BUFFERED_IO) {
				if (!Irp->AssociatedIrp.SystemBuffer) {
					return STATUS_INVALID_PARAMETER;
				}
				::memcpy(buffer, Irp->AssociatedIrp.SystemBuffer, size);
				return STATUS_SUCCESS;
			}
			if (!Irp->UserBuffer) {
				return STATUS_INVALID_PARAMETER;
			}
			::memcpy(buffer, Irp->UserBuffer, size);
			return STATUS_SUCCESS;

		case IrpMajorCode::DEVICE_CONTROL:
		case IrpMajorCode::INTERNAL_DEVICE_CONTROL:
			auto controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
			if (METHOD_FROM_CTL_CODE(controlCode) == METHOD_NEITHER) {
				if (stack->Parameters.DeviceIoControl.Type3InputBuffer < (PVOID)(1 << 16)) {
					::memcpy(buffer, stack->Parameters.DeviceIoControl.Type3InputBuffer, size);
				}
				else {
					return STATUS_UNSUCCESSFUL;
				}
			}
			else {
				if (!output || METHOD_FROM_CTL_CODE(controlCode) == METHOD_BUFFERED) {
					if (!Irp->AssociatedIrp.SystemBuffer) {
						return STATUS_INVALID_PARAMETER;
					}
					::memcpy(buffer, Irp->AssociatedIrp.SystemBuffer, size);
				}
				else {
					if (!Irp->MdlAddress) {
						return STATUS_INVALID_PARAMETER;
					}
					auto data = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
					if (data) {
						::memcpy(buffer, data, size);
					}
					else {
						return STATUS_UNSUCCESSFUL;
					}
				}
			}
			return STATUS_SUCCESS;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}
	return STATUS_UNSUCCESSFUL;
}


NTSTATUS DriverMonGenericDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto driver = DeviceObject->DriverObject;
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	IrpArrivedInfo* info = nullptr;

	for (int i = 0; i < MaxMonitoredDrivers; i++) {
		if (globals.Drivers[i].DriverObject != driver)
			continue;

		if (globals.IsMonitoring && globals.NotifyEvent) {
			NT_ASSERT(driver == DeviceObject->DriverObject);

			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Driver 0x%p intercepted!\n", driver));

			info = static_cast<IrpArrivedInfo*>(ExAllocatePool(NonPagedPool, MaxDataSize + sizeof(IrpArrivedInfo)));
			if (info) {
				info->Type = DataItemType::IrpArrived;
				KeQuerySystemTime((PLARGE_INTEGER)&info->Time);
				info->Size = sizeof(IrpArrivedInfo);
				info->DeviceObject = DeviceObject;
				info->Irp = Irp;
				info->DriverObject = driver;
				info->MajorFunction = static_cast<IrpMajorCode>(stack->MajorFunction);
				info->MinorFunction = static_cast<IrpMinorCode>(stack->MinorFunction);
				info->ProcessId = HandleToUlong(PsGetCurrentProcessId());
				info->ThreadId = HandleToUlong(PsGetCurrentThreadId());
				info->Irql = KeGetCurrentIrql();
				info->DataSize = 0;

				switch (info->MajorFunction) {
				case IrpMajorCode::WRITE:
					info->Write.Length = stack->Parameters.Write.Length;
					info->Write.Offset = stack->Parameters.Write.ByteOffset.QuadPart;
					if (info->Write.Length > 0) {
						auto dataSize = min(MaxDataSize, info->Write.Length);
						if (NT_SUCCESS(GetDataFromIrp(DeviceObject, Irp, stack, info->MajorFunction, (PUCHAR)info + sizeof(IrpArrivedInfo), dataSize))) {
							info->DataSize = dataSize;
							info->Size += (USHORT)dataSize;
						}
					}
					break;

				case IrpMajorCode::DEVICE_CONTROL:
				case IrpMajorCode::INTERNAL_DEVICE_CONTROL:
					info->DeviceIoControl.IoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
					info->DeviceIoControl.InputBufferLength = stack->Parameters.DeviceIoControl.InputBufferLength;
					info->DeviceIoControl.OutputBufferLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
					if (info->DeviceIoControl.InputBufferLength > 0) {
						auto dataSize = min(MaxDataSize, info->DeviceIoControl.InputBufferLength);
						if (NT_SUCCESS(GetDataFromIrp(DeviceObject, Irp, stack, info->MajorFunction, (PUCHAR)info + sizeof(IrpArrivedInfo), dataSize))) {
							info->DataSize = dataSize;
							info->Size += (USHORT)dataSize;
						}
					}
					break;
				}

				globals.DataBuffer->Write(info, info->Size);

			}
		}

		//auto userBuffer = Irp->UserBuffer;
		//auto status = globals.Drivers[i].MajorFunction[stack->MajorFunction](DeviceObject, Irp);
		//if (info) {
		//	// IRP completed synnchronously
		//	// build completion message

		//	int size = sizeof(IrpCompletedInfo);
		//	int extraSize = 0;

		//	if (status != STATUS_PENDING && NT_SUCCESS(status)) {
		//		
		//	}
		//}
	}

	NT_ASSERT(false);
	return STATUS_SUCCESS;
}

void GenericDriverUnload(PDRIVER_OBJECT DriverObject) {
	for (int i = 0; i < MaxMonitoredDevices; ++i) {
		if (globals.Drivers[i].DriverObject == DriverObject) {
			if (globals.Drivers[i].DriverUnload)
				globals.Drivers[i].DriverUnload(DriverObject);
			DevMonManager::RemoveDriver(i);
		}
	}
	NT_ASSERT(false);
}