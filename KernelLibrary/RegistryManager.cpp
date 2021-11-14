#include "pch.h"
#include "RegistryManager.h"
#include "ObjectAttributes.h"

// 打开
NTSTATUS RegistryManager::Open(PUNICODE_STRING path,RegistryAccessMask accessMask,ULONG opts) {
	NTSTATUS status;
	ULONG ul;
	ObjectAttributes attr(path, ObjectAttributesFlags::KernelHandle|ObjectAttributesFlags::Caseinsensive);
	status = ZwCreateKey(&_key, static_cast<ULONG>(accessMask), &attr, 0, nullptr, opts, &ul);
	return status;
}

// 关闭
NTSTATUS RegistryManager::Close() {
	NTSTATUS status = STATUS_SUCCESS;
	if (_key != nullptr) {
		// 以Zw开头的内核函数，IRQL要求是PASSIVE_LEVEL
		status = ZwClose(_key);
		_key = nullptr;
	}
	return status;
}

NTSTATUS RegistryManager::SetDWORDValue(PUNICODE_STRING name, const ULONG& value) {
	return ZwSetValueKey(_key, name, 0, REG_DWORD, (PVOID)&value, sizeof(value));
}

NTSTATUS RegistryManager::SetQWORDValue(PUNICODE_STRING name, const ULONGLONG& value) {
	return ZwSetValueKey(_key, name, 0, REG_QWORD, (PVOID)&value, sizeof(value));
}

NTSTATUS RegistryManager::QueryDWORDValue(PUNICODE_STRING name, ULONG& value) {
	NTSTATUS status;
	ULONG len;
	status = ZwQueryValueKey(_key, name, KeyValuePartialInformation, nullptr, 0, &len);
	if (status == STATUS_BUFFER_TOO_SMALL && len != 0) {
		PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool,
			len, ' ger');
		if (info != nullptr) {
			memset(info, 0, len);
			status = ZwQueryValueKey(_key, name, KeyValuePartialInformation, info, len, &len);
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



