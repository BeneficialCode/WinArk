#pragma once


struct Helpers abstract final {
	static USHORT GetKeyObjectTypeIndex();
	static CString GetObjectName(HANDLE hObject, DWORD pid);
	static CString GetErrorText(DWORD error = ::GetLastError());
	static inline const CString DefaultValueName{ L"(Default)" };
	static PCWSTR GetSystemDir();
	static PCWSTR GetWindowsDir();
	static std::wstring GetDosNameFromNtName(PCWSTR name);
};