#pragma once

enum class RegistryAccessMask {
	None = 0,
	Write = KEY_WRITE,
	Read = KEY_READ,
};
DEFINE_ENUM_FLAG_OPERATORS(RegistryAccessMask);

struct RegistryKey {
	explicit RegistryKey(HANDLE hKey = nullptr, bool own = true);

	// 拷贝构造函数
	RegistryKey(const RegistryKey&) = delete;
	// 赋值构造函数
	RegistryKey& operator=(const RegistryKey&) = delete;
	// 移动构造函数
	RegistryKey(RegistryKey&&);
	// 移动赋值运算符
	RegistryKey& operator=(RegistryKey&&);

	~RegistryKey() {
		Close();
	}

	HANDLE Detach();
	void Attach(HANDLE hKey, bool own = true);

	operator bool() const {
		return _hKey != nullptr;
	}

	HANDLE* AddressOf() {
		return &_hKey;
	}

	operator HANDLE() const {
		return _hKey;
	}

	HANDLE Get() const {
		return _hKey;
	}

	NTSTATUS Close();
	NTSTATUS Create(HANDLE hKeyParent, PUNICODE_STRING keyName, RegistryAccessMask accessMask = RegistryAccessMask::Write | RegistryAccessMask::Read,
		ULONG opts = REG_OPTION_NON_VOLATILE);
	NTSTATUS Open(HANDLE hKeyParent, PUNICODE_STRING keyName, RegistryAccessMask accessMask = RegistryAccessMask::Write | RegistryAccessMask::Read);

	NTSTATUS SetValue(PUNICODE_STRING name, ULONG type, PVOID pValue, ULONG bytes);
	NTSTATUS SetBinaryValue(PUNICODE_STRING name, PVOID pValue, ULONG bytes);
	NTSTATUS SetDWORDValue(PUNICODE_STRING name, const ULONG value);
	NTSTATUS SetQWORDValue(PUNICODE_STRING name, const ULONGLONG value);
	NTSTATUS SetMultiStringValue(PUNICODE_STRING name, PCWSTR pValue);
	NTSTATUS SetStringValue(PUNICODE_STRING name, PCWSTR pValue, DWORD type = REG_SZ);

	NTSTATUS QueryDWORDValue(PUNICODE_STRING name, ULONG value);

	NTSTATUS DeleteKey();

private:
	void CheckPredefinedKey();

	HANDLE _hKey;
	bool _own;
};