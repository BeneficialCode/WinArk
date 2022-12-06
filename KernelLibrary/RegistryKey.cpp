#include "pch.h"
#include "RegistryKey.h"
#include "ObjectAttributes.h"


RegistryKey::RegistryKey(HANDLE hKey, bool own) :_hKey(hKey), _own(own) {
	CheckPredefinedKey();
}

RegistryKey::RegistryKey(RegistryKey&& other){
	_hKey = other._hKey;
	_own = other._own;
	CheckPredefinedKey();
	other._hKey = nullptr;
}

RegistryKey& RegistryKey::operator=(RegistryKey&& other) {
	Close();
	_hKey = other._hKey;
	_own = other._own;
	CheckPredefinedKey();
	other._hKey = nullptr;
	return *this;
}

HANDLE RegistryKey::Detach() {
	auto h = _hKey;
	_hKey = nullptr;
	return h;
}

void RegistryKey::Attach(HANDLE hKey, bool own) {
	Close();
	_hKey = hKey;
	_own = own;
	CheckPredefinedKey();
}

// 创建
NTSTATUS RegistryKey::Create(HANDLE hKeyParent, PUNICODE_STRING keyName, RegistryAccessMask accessMask,
	ULONG opts) {
	ASSERT(hKeyParent != nullptr);
	NTSTATUS status;
	ObjectAttributes keyAttr(keyName, ObjectAttributesFlags::KernelHandle | ObjectAttributesFlags::Caseinsensive,
		hKeyParent);
	HANDLE hKey = nullptr;
	status = ZwCreateKey(&hKey, static_cast<ULONG>(accessMask), &keyAttr, 0, nullptr, opts, nullptr);
	if (NT_SUCCESS(status)) {
		Close();
		_hKey = hKey;
		CheckPredefinedKey();
	}
	return status;
}

// 打开
NTSTATUS RegistryKey::Open(HANDLE hKeyParent, PUNICODE_STRING keyName, RegistryAccessMask accessMask) {
	ASSERT(_hKey == nullptr);
	NTSTATUS status;
	ObjectAttributes attr(keyName, ObjectAttributesFlags::KernelHandle | ObjectAttributesFlags::Caseinsensive);
	status = ZwOpenKey(&_hKey, static_cast<ULONG>(accessMask), &attr);
	CheckPredefinedKey();
	return status;
}

// 关闭
NTSTATUS RegistryKey::Close() {
	NTSTATUS status = STATUS_SUCCESS;
	if (_hKey && _own) {
		// 以Zw开头的内核函数，IRQL要求是PASSIVE_LEVEL
		status = ZwClose(_hKey);
		_hKey = nullptr;
	}
	return status;
}

NTSTATUS RegistryKey::SetValue(PUNICODE_STRING name, ULONG type, PVOID pValue,ULONG bytes) {
	return ZwSetValueKey(_hKey, name, 0, type, pValue, bytes);
}

NTSTATUS RegistryKey::SetBinaryValue(PUNICODE_STRING name, PVOID pValue, ULONG bytes) {
	return ZwSetValueKey(_hKey, name, 0, REG_BINARY, pValue, bytes);
}

NTSTATUS RegistryKey::SetDWORDValue(PUNICODE_STRING name, const ULONG value) {
	return ZwSetValueKey(_hKey, name, 0, REG_DWORD, (PVOID)&value, sizeof(value));
}

NTSTATUS RegistryKey::SetQWORDValue(PUNICODE_STRING name, const ULONGLONG value) {
	return ZwSetValueKey(_hKey, name, 0, REG_QWORD, (PVOID)&value, sizeof(value));
}

NTSTATUS RegistryKey::SetStringValue(PUNICODE_STRING name, PCWSTR pValue, DWORD type) {
	ASSERT(_hKey);
	ASSERT((type == REG_SZ || type == REG_EXPAND_SZ));

	return ZwSetValueKey(_hKey, name, 0, type, (PVOID)pValue,
		(static_cast<ULONG>(wcslen(pValue) + 1) * sizeof(WCHAR)));
}

NTSTATUS RegistryKey::SetMultiStringValue(PUNICODE_STRING name, PCWSTR pValue) {
	PCWSTR pTemp;
	ULONG bytes;
	ULONG length;

	ASSERT(_hKey);

	bytes = 0;
	pTemp = pValue;
	do
	{
		length = static_cast<ULONG>(wcslen(pValue)) + 1;
		pTemp += length;
		bytes += length * sizeof(WCHAR);
	} while (length!=1);

	return ZwSetValueKey(_hKey, name, 0, REG_MULTI_SZ, (PVOID)pValue, bytes);
}

NTSTATUS RegistryKey::QueryDWORDValue(PUNICODE_STRING name, ULONG value) {
	NTSTATUS status;
	ULONG len;
	status = ZwQueryValueKey(_hKey, name, KeyValuePartialInformation, nullptr, 0, &len);
	if (status == STATUS_BUFFER_TOO_SMALL && len != 0) {
		PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool,
			len, ' yek');
		if (info != nullptr) {
			memset(info, 0, len);
			status = ZwQueryValueKey(_hKey, name, KeyValuePartialInformation, info, len, &len);
			if (status == STATUS_SUCCESS) {
				// 获取value的值
				value = *((ULONG*)info->Data);
			}
			ExFreePoolWithTag(info, ' ger');
			info = nullptr;
		}
	}
	return status;
}


void RegistryKey::CheckPredefinedKey() {
	if ((DWORD_PTR)_hKey & 0xf000000000000000)
		_own = false;
}

NTSTATUS RegistryKey::DeleteKey() {
	return ZwDeleteKey(_hKey);
}

