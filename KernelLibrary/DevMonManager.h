#pragma once

#include "FastMutex.h"
#include "SpinLock.h"
#include "CyclicBuffer.h"

const int MaxMonitoredDevices = 32;
const int MaxMonitoredDrivers = 16;
const int MaxDataSize = 1 << 13;

extern "C" POBJECT_TYPE * IoDriverObjectType;

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

NTSTATUS GetDataFromIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION stack, IrpMajorCode code, PVOID buffer, ULONG size, bool output = false);
NTSTATUS DriverMonGenericDispatch(PDEVICE_OBJECT, PIRP);
void GenericDriverUnload(PDRIVER_OBJECT DriverObject);


struct MonitoredDevice {
	// driver filter
	UNICODE_STRING DeviceName;
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT LowerDeviceObject;
	// driver hook
	WCHAR DriverName[64];
	PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
	PDRIVER_OBJECT DriverObject;
	PDRIVER_UNLOAD DriverUnload;
};

struct DeviceExtension {
	IO_REMOVE_LOCK RemoveLock;
	PDEVICE_OBJECT LowerDeviceObject;
};

class DevMonManager {
public:
	void Init(PDRIVER_OBJECT DriverObject);

	NTSTATUS AddDevice(PCWSTR name);
	int FindDevice(PCWSTR name);
	int FindDevice(PDEVICE_OBJECT DeviceObject);
	bool RemoveDevice(PCWSTR name);
	bool RemoveDevice(PDEVICE_OBJECT LowerDeviceObject);
	void RemoveAllDevices();

	NTSTATUS AddDriver(PCWSTR dirverName, PVOID* driverObject);
	static NTSTATUS RemoveDriver(int index);

	PDEVICE_OBJECT CDO;
private:
	bool RemoveDevice(int index);

private:
	MonitoredDevice Devices[MaxMonitoredDevices];
	int MonitoredDeviceCount;
	// A fast mutex to protect the array
	FastMutex Lock;
	PDRIVER_OBJECT DriverObject;
};

struct DriverMonGlobals {
	MonitoredDevice Drivers[MaxMonitoredDevices];
	CyclicBuffer<SpinLock>* DataBuffer;
	PKEVENT NotifyEvent;
	short Count;
	bool IsMonitoring;
};


