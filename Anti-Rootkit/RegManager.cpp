#include<wdm.h>
#include"RegManager.h"

void RegCreateKey(LPWSTR KeyName)
{
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName;
	NTSTATUS status;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, KeyName);
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwCreateKey(&hRegister,
		KEY_ALL_ACCESS,
		&objectAttributes, 0, NULL,
		REG_OPTION_NON_VOLATILE,
		NULL);
	if (NT_SUCCESS(status))
	{
		ZwClose(hRegister);
		KdPrint(("ZwCreateKey success!\n"));
	}
	else {
		KdPrint(("ZwCreateKey failed!\n"));
	}
}

void RegRenameKey(LPWSTR OldKeyName, LPWSTR NewKeyName) {
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE hRegister;
	NTSTATUS status;
	UNICODE_STRING usOldKeyName, usNewKeyName;
	RtlInitUnicodeString(&usOldKeyName, OldKeyName);
	RtlInitUnicodeString(&usNewKeyName, NewKeyName);
	InitializeObjectAttributes(&objectAttributes,
		&usOldKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		status = ZwRenameKey(hRegister, &usNewKeyName);
		ZwFlushKey(hRegister);
		ZwClose(hRegister);
		KdPrint(("ZwRenameKey successful!\n"));
	}
	else {
		KdPrint(("ZwRenameKey failed!\n"));
	}
}
void RegDeleteKey(LPWSTR KeyName) {
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName;
	NTSTATUS status;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, KeyName);
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		status = ZwDeleteKey(hRegister);
		ZwClose(hRegister);
		KdPrint(("ZwDeleteKey success!\n"));
	}
	else {
		KdPrint(("ZwDeleteKey failed!\n"));
	}
}

void RegSetValueKey(LPWSTR KeyName, LPWSTR ValueName, ULONG DataType, PVOID DataBuffer, ULONG DataLength) {
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName, usValueName;
	NTSTATUS status;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, KeyName);
	RtlInitUnicodeString(&usValueName, ValueName);
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		status = ZwSetValueKey(hRegister, &usValueName,
			0, DataType, DataBuffer, DataLength);
		ZwFlushKey(hRegister);
		ZwClose(hRegister);
		KdPrint(("ZwSetValueKey success!\n"));
	}
	else {
		KdPrint(("ZwSetValueKey failed!\n"));
	}
}
NTSTATUS RegQueryValueKey(LPWSTR KeyName, LPWSTR ValueName, PKEY_VALUE_PARTIAL_INFORMATION *pkvpi) {
	ULONG ulSize;
	NTSTATUS status;
	PKEY_VALUE_PARTIAL_INFORMATION pvpi;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE hRegister;
	UNICODE_STRING usKeyName;
	UNICODE_STRING usValueName;
	RtlInitUnicodeString(&usKeyName, KeyName);
	RtlInitUnicodeString(&usValueName, ValueName);
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[RegQueryValueKey]ZwOpenKey failed!\n"));
		return status;
	}
	status = ZwQueryValueKey(hRegister,
		&usValueName,
		KeyValuePartialInformation,
		NULL,
		0,
		&ulSize);
	if (status == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0) {
		KdPrint(("ZwQueryValueKey 1 failed\n"));
		return STATUS_UNSUCCESSFUL;
	}
	pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
	status = ZwQueryValueKey(hRegister,
		&usValueName,
		KeyValuePartialInformation,
		pvpi,
		ulSize,
		&ulSize);
	
	if (!NT_SUCCESS(status)) {
		KdPrint(("ZwQueryValueKey 2 failed!\n"));
	}
	// 这里未进行释放
	*pkvpi = pvpi;
	KdPrint(("ZwQueryValueKey success!\n"));
	return STATUS_SUCCESS;
}

void RegDeleteValueKey(LPWSTR KeyName, LPWSTR ValueName) {
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName, usValueName;
	NTSTATUS status;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, KeyName);
	RtlInitUnicodeString(&usValueName, ValueName);
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		status = ZwDeleteValueKey(hRegister, &usValueName);
		ZwFlushKey(hRegister);
		ZwClose(hRegister);
		KdPrint(("ZwDeleteValueKey success!\n"));
	}
	else {
		KdPrint(("ZwDeleteValueKey failed!\n"));
	}
}

void EnumSubKeyTest() {
	WCHAR MY_KEY_NAME[] = L"\\Registry";
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	OBJECT_ATTRIBUTES objectAttributes;
	NTSTATUS status;
	ULONG ulSize, i;
	UNICODE_STRING uniKeyName;
	PKEY_FULL_INFORMATION pfi;
	RtlInitUnicodeString(&RegUnicodeString, MY_KEY_NAME);
	InitializeObjectAttributes(&objectAttributes,
		&RegUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		KdPrint(("Open register successfully!\n"));
	}
	ZwQueryKey(hRegister, KeyFullInformation, NULL, 0, &ulSize);
	pfi = (PKEY_FULL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
	ZwQueryKey(hRegister, KeyFullInformation, pfi, ulSize, &ulSize);
	for (i = 0; i < pfi->SubKeys; i++) {
		PKEY_BASIC_INFORMATION pbi;
		ZwEnumerateKey(hRegister, i, KeyBasicInformation, NULL, 0, &ulSize);
		pbi = (PKEY_BASIC_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ZwEnumerateKey(hRegister, i, KeyBasicInformation, pbi, ulSize, &ulSize);
		uniKeyName.Length = (USHORT)pbi->NameLength;
		uniKeyName.MaximumLength = (USHORT)pbi->NameLength;
		uniKeyName.Buffer = pbi->Name;
		KdPrint(("The %d sub item name: %wZ\n", i, &uniKeyName));
		ExFreePool(pbi);
	}
	ExFreePool(pfi);
	ZwClose(hRegister);
}

void EnumSubValueTest() {
	WCHAR MY_KEY_NAME[] = L"\\Registry\\Machine\\Software\\Microsoft\\.NETFramework";
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	OBJECT_ATTRIBUTES objectAttributes;
	ULONG ulSize, i;
	UNICODE_STRING uniKeyName;
	PKEY_FULL_INFORMATION pfi;
	NTSTATUS status;
	RtlInitUnicodeString(&RegUnicodeString, MY_KEY_NAME);
	InitializeObjectAttributes(&objectAttributes,
		&RegUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(status)) {
		KdPrint(("Open register successfully!\n"));
	}
	ZwQueryKey(hRegister, KeyFullInformation, NULL, 0, &ulSize);
	pfi = (PKEY_FULL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
	ZwQueryKey(hRegister, KeyFullInformation, pfi, ulSize, &ulSize);
	for (i = 0; i < pfi->Values; i++) {
		PKEY_VALUE_BASIC_INFORMATION pvbi;
		ZwEnumerateValueKey(hRegister, i, KeyValueBasicInformation, NULL, 0, &ulSize);
		pvbi = (PKEY_VALUE_BASIC_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ZwEnumerateValueKey(hRegister, i, KeyValueBasicInformation, pvbi, ulSize, &ulSize);
		uniKeyName.Length = (USHORT)pvbi->NameLength;
		uniKeyName.MaximumLength = (USHORT)pvbi->NameLength;
		uniKeyName.Buffer = pvbi->Name;
		KdPrint(("The %d sub value name: %wZ\n", i, &uniKeyName));
		if (pvbi->Type == REG_SZ) {
			KdPrint(("The sub value type : REG_SZ\n"));
		}
		else if(pvbi->Type == REG_MULTI_SZ){
			KdPrint(("The sub value type: REG_MULTI_SZ"));
		}
		else if (pvbi->Type == REG_BINARY) {
			KdPrint(("The sub value type: REG_BINARY"));
		}
		ExFreePool(pvbi);
	}
	ExFreePool(pfi);
	ZwClose(hRegister);
}