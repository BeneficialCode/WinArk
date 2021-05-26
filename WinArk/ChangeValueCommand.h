#pragma once
#include "stdafx.h"
#include "AppCommandBase.h"
#include "RegHelper.h"
#include "RegistryManager.h"

// Ä£°åÀà
template<typename T>
class ChangeValueCommand : public AppCommandBase {
public:
	ChangeValueCommand(const CString& path, const CString& name, const T& value, DWORD type) :
		AppCommandBase(L"Change Value"),
		_path(path), _name(name), _value(value), _type(type) {}

	ChangeValueCommand(const CString& path, const CString& name, BinaryValue& value) :
		AppCommandBase(L"Change Value"),
		_path(path), _name(name), _value(std::move(value)), _type(REG_BINARY) {
	}


	bool Execute() override;
	bool Undo() override {
		return Execute();
	}

private:
	bool ChangeValue(CRegKey& key, const CString& value);
	bool ChangeValue(CRegKey& key, ULONGLONG value);
	bool ChangeValue(CRegKey& key, const BinaryValue& value);

private:
	T _value;
	CString _path, _name;
	DWORD _type;
};

template<typename T>
bool ChangeValueCommand<T>::Execute() {
	CRegKey key((HKEY)RegHelper::OpenKey(_path, KEY_WRITE | KEY_READ));
	if (!key)
		return false;
	return ChangeValue(key, _value);
}

template<typename T>
bool ChangeValueCommand<T>::ChangeValue(CRegKey& key, const CString& value) {
	WCHAR oldValue[2048] = { 0 };
	ULONG chars = 2048;
	if (_type == REG_MULTI_SZ) {
		auto status = key.QueryMultiStringValue(_name, oldValue, &chars);
		if (status != ERROR_SUCCESS)
			return false;

		status = key.SetMultiStringValue(_name, _value);
		if (status == ERROR_SUCCESS) {
			_value = CString(oldValue, chars);
			return true;
		}
	}
	else {
		auto status = key.QueryStringValue(_name, oldValue, &chars);
		if (status != ERROR_SUCCESS && !_name.IsEmpty())
			return false;

		status = key.SetStringValue(_name, _value, _type);
		if (status == ERROR_SUCCESS) {
			_value = oldValue;
			return true;
		}
	}
	return false;
}

template<typename T>
inline bool ChangeValueCommand<T>::ChangeValue(CRegKey& key, ULONGLONG value) {
	ULONGLONG oldValue = 0;
	ATLASSERT(_type == REG_DWORD || _type == REG_QWORD);
	auto status = (_type == REG_DWORD ? key.QueryDWORDValue(_name, (DWORD&)oldValue) : key.QueryQWORDValue(_name, oldValue));
	if (status != ERROR_SUCCESS)
		return false;

	status = _type == REG_DWORD ? key.SetDWORDValue(_name, (DWORD)_value) : key.SetQWORDValue(_name, _value);
	if (status == ERROR_SUCCESS) {
		_value = oldValue;
		return true;
	}
	return false;
}

template<typename T>
inline bool ChangeValueCommand<T>::ChangeValue(CRegKey& key, const BinaryValue& value) {
	ULONG size;
	auto status = key.QueryBinaryValue(_name, nullptr, &size);
	if (status != ERROR_SUCCESS)
		return false;

	ATLASSERT(size > 0);
	auto buffer = std::make_unique<BYTE[]>(size);
	status = key.QueryBinaryValue(_name, buffer.get(), &size);
	if (status != ERROR_SUCCESS)
		return false;

	// make the change

	status = key.SetBinaryValue(_name, value.Buffer.get(), value.Size);
	if (status != ERROR_SUCCESS)
		return false;

	_value.Buffer = std::move(buffer);
	_value.Size = size;

	return true;
}