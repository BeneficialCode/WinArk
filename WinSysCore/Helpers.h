#pragma once


struct Helpers abstract final {
	static std::wstring GetDosNameFromNtName(PCWSTR name);
	static PVOID GetKernelBase();
	static DWORD GetKernelImageSize();
	static std::string GetNtosFileName();
	static PVOID GetWin32kBase();
	static DWORD GetWin32kImageSize();
	static std::string GetModuleByAddress(ULONG_PTR address);
	static std::wstring StringToWstring(const std::string& str);
	static std::string WstringToString(const std::wstring& wstr);
};