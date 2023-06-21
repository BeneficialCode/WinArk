#pragma once


struct Helpers abstract final {
	static std::wstring GetDosNameFromNtName(PCWSTR name);
	static PVOID GetKernelBase();
	static DWORD GetKernelImageSize();
	static std::string GetNtosFileName();
	static PVOID GetWin32kBase();
	static DWORD GetWin32kImageSize();

	static PVOID GetKernelModuleBase(std::string moduleName);
	static DWORD GetKernelModuleImageSize(std::string moduleName);

	static std::string GetKernelModuleByAddress(ULONG_PTR address);
	static std::string GetKernelModuleNameByAddress(ULONG_PTR address);
	static std::wstring GetUserModuleByAddress(ULONG_PTR address, ULONG pid);
	static std::wstring StringToWstring(const std::string& str);
	static std::string WstringToString(const std::wstring& wstr);
	static std::wstring GetDriverDirFromObjectManager(std::wstring serviceName);
	static bool WriteString(HANDLE hFile, std::wstring const& text);

	static bool SearchPattern(PUCHAR pattern, UCHAR wildcard,
		ULONG_PTR len, const void* base, ULONG_PTR size, PVOID* ppFound);


};