#include "pch.h"
#include <tdikrnl.h>

#define KTDI_TAG 'NACS'

NTSTATUS tdi_open_transport_address(PUNICODE_STRING devName, ULONG addr, USHORT port, 
	BOOLEAN shared, PHANDLE addressHandle, PFILE_OBJECT* addressFileObject) {
	OBJECT_ATTRIBUTES attr;
	PFILE_FULL_EA_INFORMATION eaBuffer;
	ULONG eaSize;
	PTA_IP_ADDRESS localAddr;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	InitializeObjectAttributes(&attr, devName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
	
	eaSize = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[1]) +
		TDI_TRANSPORT_ADDRESS_LENGTH + 1 + sizeof(TA_IP_ADDRESS);

	eaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, eaSize, KTDI_TAG);
	if (eaBuffer == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	eaBuffer->NextEntryOffset = 0;
	eaBuffer->Flags = 0;
	eaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
	eaBuffer->EaValueLength = sizeof(TA_IP_ADDRESS);

	RtlCopyMemory(eaBuffer->EaName, TdiTransportAddress, eaBuffer->EaNameLength + 1);

	localAddr = (PTA_IP_ADDRESS)(eaBuffer->EaName + eaBuffer->EaNameLength + 1);

	localAddr->TAAddressCount = 1;
	localAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
	localAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	localAddr->Address[0].Address[0].sin_port = port;
	localAddr->Address[0].Address[0].in_addr = addr;

	RtlZeroMemory(localAddr->Address[0].Address[0].sin_zero, 
		sizeof(localAddr->Address[0].Address[0].sin_zero));

	status = ZwCreateFile(addressHandle,
		GENERIC_READ | GENERIC_WRITE,
		&attr,
		&iosb,
		nullptr,
		FILE_ATTRIBUTE_NORMAL,
		shared ? FILE_SHARE_READ | FILE_SHARE_WRITE : 0,
		FILE_OPEN,
		0,
		eaBuffer,
		eaSize);

	ExFreePool(eaBuffer);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = ObReferenceObjectByHandle(*addressHandle, FILE_ALL_ACCESS, nullptr,
		KernelMode, (PVOID*)addressFileObject, nullptr);

	if (!NT_SUCCESS(status)) {
		ZwClose(*addressHandle);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS tdi_set_event_handler(PFILE_OBJECT addressFileObject, LONG eventType, PVOID eventHandler, 
	PVOID eventContext) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(addressFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = TdiBuildInternalDeviceControlIrp(TDI_SET_EVENT_HANDLER, devObj, addressFileObject, &event, &iosb);
	if (irp == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	TdiBuildSetEventHandler(irp, devObj, addressFileObject, nullptr, nullptr, eventType, eventHandler,
		eventContext);

	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	return status;
}

NTSTATUS tdi_disconnect(PFILE_OBJECT connectionFileObject, ULONG flags) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(connectionFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = TdiBuildInternalDeviceControlIrp(TDI_DISCONNECT, devObj, connectionFileObject, &event, &iosb);

	if (irp == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	TdiBuildDisconnect(irp, devObj, connectionFileObject, nullptr, nullptr, nullptr,
		flags, nullptr, nullptr);

	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	return status;
}

NTSTATUS tdi_unset_event_handler(PFILE_OBJECT addressFileObject, LONG eventType) {
	return tdi_set_event_handler(addressFileObject, eventType, nullptr, nullptr);
}

NTSTATUS tdi_disassociate_address(PFILE_OBJECT connectionFileObject) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(connectionFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = TdiBuildInternalDeviceControlIrp(TDI_DISASSOCIATE_ADDRESS, devObj, connectionFileObject, &event, &iosb);
	if (irp == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	TdiBuildDisassociateAddress(irp, devObj, connectionFileObject, nullptr, nullptr);

	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	return status;
}

NTSTATUS tdi_open_connection_endpoint(PUNICODE_STRING devName, PVOID connectionContext, BOOLEAN shared, 
	PHANDLE connectionHandle, PFILE_OBJECT* connectionFileObject) {
	OBJECT_ATTRIBUTES attr;
	PFILE_FULL_EA_INFORMATION eaBuffer;
	ULONG eaSize;
	PVOID* context;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	InitializeObjectAttributes(&attr, devName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

	eaSize = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[1]) + TDI_CONNECTION_CONTEXT_LENGTH + 1 + sizeof(void*);

	eaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, eaSize, KTDI_TAG);

	if (eaBuffer == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	eaBuffer->NextEntryOffset = 0;
	eaBuffer->Flags = 0;
	eaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
	eaBuffer->EaValueLength = sizeof(void*);

	RtlCopyMemory(eaBuffer->EaName, TdiConnectionContext, eaBuffer->EaNameLength + 1);

	context = (PVOID*)&(eaBuffer->EaName[eaBuffer->EaNameLength + 1]);

	*context = connectionContext;

	status = ZwCreateFile(
		connectionHandle,
		GENERIC_READ | GENERIC_WRITE,
		&attr,
		&iosb,
		nullptr,
		FILE_ATTRIBUTE_NORMAL,
		shared ? FILE_SHARE_READ | FILE_SHARE_WRITE : 0,
		FILE_OPEN,
		0,
		eaBuffer,
		eaSize
	);

	ExFreePool(eaBuffer);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = ObReferenceObjectByHandle(*connectionHandle, FILE_ALL_ACCESS, nullptr,
		KernelMode, (PVOID*)connectionFileObject, nullptr);

	if (!NT_SUCCESS(status)) {
		ZwClose(*connectionHandle);
		return status;
	}

	
	return STATUS_SUCCESS;
}

NTSTATUS tdi_associate_address(PFILE_OBJECT connectionFileObject, HANDLE addressHandle) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(connectionFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = TdiBuildInternalDeviceControlIrp(TDI_ASSOCIATE_ADDRESS, devObj, connectionFileObject, &event, &iosb);

	if (irp == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	TdiBuildAssociateAddress(irp, devObj, connectionFileObject, nullptr, nullptr, addressHandle);

	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	return status;
}

NTSTATUS tdi_connect(PFILE_OBJECT connectionFileObject, ULONG addr, USHORT port) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PTDI_CONNECTION_INFORMATION remoteInfo;
	PTA_IP_ADDRESS remoteAddr;
	PTDI_CONNECTION_INFORMATION returnInfo;
	PTA_IP_ADDRESS returnAddr;
	PIRP irp;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(connectionFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	SIZE_T bytes = 2 * sizeof(TDI_CONNECTION_INFORMATION) + 2 * sizeof(TA_IP_ADDRESS);
	remoteInfo = (PTDI_CONNECTION_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, bytes, KTDI_TAG);

	if (remoteInfo == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(remoteInfo, bytes);

	remoteInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
	remoteInfo->RemoteAddress = (PUCHAR)remoteInfo + sizeof(TDI_CONNECTION_INFORMATION);

	remoteAddr = (PTA_IP_ADDRESS)remoteInfo->RemoteAddress;

	remoteAddr->TAAddressCount = 1;
	remoteAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
	remoteAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	remoteAddr->Address[0].Address[0].sin_port = port;
	remoteAddr->Address[0].Address[0].in_addr = addr;

	returnInfo = (PTDI_CONNECTION_INFORMATION)((PUCHAR)remoteInfo + sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));

	returnInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
	returnInfo->RemoteAddress = (PUCHAR)returnInfo + sizeof(TDI_CONNECTION_INFORMATION);

	returnAddr = (PTA_IP_ADDRESS)returnInfo->RemoteAddress;

	returnAddr->TAAddressCount = 1;
	returnAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
	returnAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

	irp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT, devObj, connectionFileObject, &event, &iosb);
	if (irp == nullptr) {
		ExFreePool(remoteInfo);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	ExFreePool(remoteInfo);
	return status;
}

NTSTATUS tdi_query_address(PFILE_OBJECT addressFileObject, PULONG addr, PUSHORT port) {
	PDEVICE_OBJECT devObj;
	KEVENT event;
	PTRANSPORT_ADDRESS localInfo;
	PTA_IP_ADDRESS localAddr;
	PIRP irp;
	PMDL mdl;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	devObj = IoGetRelatedDeviceObject(addressFileObject);

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	ULONG length = sizeof(TDI_ADDRESS_INFO) * 10;
	localInfo = (PTRANSPORT_ADDRESS)ExAllocatePoolWithTag(NonPagedPool, length, KTDI_TAG);
	if (localInfo == nullptr) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(localInfo, length);

	irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION, devObj, addressFileObject, &event, &iosb);

	if (irp == nullptr) {
		ExFreePool(localInfo);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	mdl = IoAllocateMdl(localInfo, length, FALSE, FALSE, nullptr);
	if (mdl == nullptr) {
		IoFreeIrp(irp);
		ExFreePool(localInfo);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	__try {
		MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);
		status = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		IoFreeMdl(mdl);
		IoFreeIrp(irp);
		ExFreePool(localInfo);
		status = STATUS_INVALID_USER_BUFFER;
	}

	if (!NT_SUCCESS(status)) {
		return status;
	}

	TdiBuildQueryInformation(irp, devObj, addressFileObject, nullptr, nullptr, TDI_QUERY_ADDRESS_INFO, mdl);
	status = IoCallDriver(devObj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = iosb.Status;
	}

	localAddr = (PTA_IP_ADDRESS)&localInfo->Address[0];
	if (addr) {
		*addr = localAddr->Address[0].Address[0].in_addr;
	}

	if (port) {
		*port = localAddr->Address[0].Address[0].sin_port;
	}

	ExFreePool(localInfo);

	return status;
}