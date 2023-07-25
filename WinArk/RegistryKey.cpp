#include "stdafx.h"
#include "RegistryKey.h"
#include "Registry.h"

RegistryKey::RegistryKey(HKEY hKey, bool own) noexcept :_hKey(hKey), _own(own) {
	ATLASSERT(IsValid());
	CheckPredefinedKey();
}

RegistryKey::RegistryKey(RegistryKey&& other) noexcept {
	_hKey = other._hKey;
	_own = other._own;
	CheckPredefinedKey();
	ATLASSERT(IsValid());
	other._hKey = nullptr;
}

RegistryKey& RegistryKey::operator=(RegistryKey&& other) noexcept {
	Close();
	_hKey = other._hKey;
	_own = other._own;
	CheckPredefinedKey();
	ATLASSERT(IsValid());
	other._hKey = nullptr;
	return *this;
}

void RegistryKey::Close() {
	if (_own && _hKey) {
		::RegCloseKey(_hKey);
	}
	_hKey = nullptr;
}

HKEY RegistryKey::Detach() {
	auto h = _hKey;
	_hKey = nullptr;
	return h;
}

void RegistryKey::Attach(HKEY hKey, bool own) {
	Close();
	_hKey = hKey;
	_own = own;
	ATLASSERT(IsValid());
	CheckPredefinedKey();
}

bool RegistryKey::IsValid() const {
	return Registry::IsKeyValid(_hKey);
}

LSTATUS RegistryKey::Open(HKEY parent, PCWSTR path, DWORD access) {
	ATLASSERT(_hKey == nullptr);
	auto error = ::RegOpenKeyEx(parent, path, 0, access, &_hKey);
	CheckPredefinedKey();
	return error;
}

// 引用声明里的批注,主要是用来简化函数定义中批注的书写,要再写一遍这么多批注有点多余
_Use_decl_annotations_
LSTATUS RegistryKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType) noexcept {
	ATLASSERT(_hKey);
	ATLENSURE_RETURN_VAL(pszValue != NULL, ERROR_INVALID_DATA);
	ATLASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));

	return ::RegSetValueEx(_hKey, pszValueName, 0, dwType,
		reinterpret_cast<const BYTE*>(pszValue),
		(static_cast<DWORD>(_tcslen(pszValue)) + 1) * sizeof(TCHAR));
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept {
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	ATLASSUME(_hKey);

	nBytes = 0;
	pszTemp = pszValue;
	do
	{
		nLength = static_cast<ULONG>(_tcslen(pszTemp)) + 1;
		pszTemp += nLength;
		nBytes += nLength * sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx(_hKey, pszValue, 0, REG_MULTI_SZ,
		reinterpret_cast<const BYTE*>(pszValue), nBytes);
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) noexcept {
	ATLASSUME(_hKey);
	return ::RegSetValueEx(_hKey, pszValueName, 0, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetBinaryValue(LPCTSTR pszValueName, const void* pData, ULONG nBytes) noexcept {
	ATLASSUME(_hKey);
	return ::RegSetValueEx(_hKey, pszValueName, 0, REG_BINARY,
		reinterpret_cast<const BYTE*>(pData), nBytes);
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) noexcept {
	ATLASSUME(_hKey);
	return ::RegSetValueEx(_hKey, pszValueName, 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) noexcept {
	ATLASSUME(_hKey);
	return ::RegSetValueEx(_hKey, pszValueName, 0, REG_QWORD,
		reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetValue(DWORD dwValue, LPCTSTR pszValueName) noexcept {
	ATLASSUME(_hKey);
	return SetDWORDValue(pszValueName, dwValue);
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) noexcept {
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;
	ULONG nChars;

	ATLASSUME(_hKey != nullptr);
	ATLASSUME(pnChars != nullptr);

	nBytes = (*pnChars) * sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(_hKey, pszValueName, nullptr, &dwType,
		reinterpret_cast<LPBYTE>(pszValue), &nBytes);

	if (lRes != ERROR_SUCCESS) {
		return lRes;
	}

	if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) {
		return ERROR_INVALID_DATA;
	}

	nChars = nBytes / sizeof(TCHAR);
	if (pszValue != nullptr) {
		if (nBytes != 0) {
			if ((nBytes % sizeof(TCHAR) != 0) || (pszValue[nChars - 1] != 0)) {
				return ERROR_INVALID_DATA;
			}
		}
		else {
			pszValue[0] = _T('\0');
		}
	}

	*pnChars = nChars;
	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) noexcept {
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;
	ULONG nChars;

	ATLASSUME(_hKey);
	ATLASSERT(pnChars);

	if (pszValue != nullptr && *pnChars < 2)
		return ERROR_INSUFFICIENT_BUFFER;

	nBytes = (*pnChars) * sizeof(WCHAR);

	lRes = ::RegQueryValueEx(_hKey, pszValueName, nullptr, &dwType,
		reinterpret_cast<LPBYTE>(pszValue), &nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_MULTI_SZ)
		return ERROR_INVALID_DATA;
	nChars = nBytes / sizeof(WCHAR);
	if (pszValue != nullptr && (nBytes % sizeof(WCHAR) != 0 ||
		nChars < 1 || pszValue[nChars - 1] != 0 ||
		(nChars > 1 && pszValue[nChars - 2] != 0)))
		return ERROR_INVALID_DATA;

	*pnChars = nChars;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LONG RegistryKey::QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) noexcept {
	LONG lRes;
	DWORD dwType;

	ATLASSERT(pnBytes);
	ATLASSERT(_hKey);

	lRes = ::RegQueryValueEx(_hKey, pszValueName, nullptr, &dwType,
		static_cast<LPBYTE>(pValue), pnBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_BINARY)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) noexcept {
	ATLASSERT(_hKey != nullptr);

	return (::RegQueryValueEx(_hKey, pszValueName, nullptr, pdwType, static_cast<LPBYTE>(pData), pnBytes));
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) noexcept {
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(_hKey != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx(_hKey, pszValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);

	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) noexcept {
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(_hKey);

	nBytes = sizeof(ULONGLONG);
	lRes = ::RegQueryValueEx(_hKey, pszValueName, nullptr, &dwType,
		reinterpret_cast<LPBYTE>(&qwValue), &nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::DeleteValue(LPCTSTR lpszValue) noexcept {
	ATLASSERT(_hKey);
	return ::RegDeleteValue(_hKey, (LPTSTR)lpszValue);
}

void RegistryKey::CheckPredefinedKey() {
	if ((DWORD_PTR)_hKey & 0xf000000000000000)
		_own = false;
}