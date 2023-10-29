#pragma once

#include "typesdefs.h"

NTSTATUS NTAPI HookNtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_ PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
);

extern "C"
NTSTATUS NTAPI NtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_ PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
);

struct BypassAntiKernelDbg {
	static PVOID GetExportSymbolAddress(PCWSTR symbolName);
	static NTSTATUS Bypass();
	static NTSTATUS Unbypass();
	using PNtQuerySystemInformation = decltype(&NtQuerySystemInformation);
	static inline PNtQuerySystemInformation g_pNtQuerySystemInformation{};
};