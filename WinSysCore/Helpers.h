#pragma once


struct Helpers abstract final {
	static std::wstring GetDosNameFromNtName(PCWSTR name);
	static PVOID GetKernelBase();
	static DWORD GetKernelImageSize();
	static std::string GetNtosFileName();
	static PVOID GetWin32kBase();
	static DWORD GetWin32kImageSize();
	static std::string GetKernelModuleByAddress(ULONG_PTR address);
	static std::wstring GetUserModuleByAddress(ULONG_PTR address, ULONG pid);
	static std::wstring StringToWstring(const std::string& str);
	static std::string WstringToString(const std::wstring& wstr);
	static std::wstring GetDriverDirFromObjectManager(std::wstring serviceName);
	static bool WriteString(HANDLE hFile, std::wstring const& text);
};