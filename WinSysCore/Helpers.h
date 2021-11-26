#pragma once


struct Helpers abstract final {
	static std::wstring GetDosNameFromNtName(PCWSTR name);
	static PVOID GetKernelBase();
	static std::string GetNtosFileName();
	static PVOID GetWin32kBase();
	static std::string GetModuleByAddress(ULONG_PTR address);
};