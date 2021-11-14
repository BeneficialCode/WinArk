#include "pch.h"
#include "Helpers.h"

#include "SecurityHelper.h"
#include <wil\resource.h>


std::wstring Helpers::GetDosNameFromNtName(PCWSTR name) {
	static std::vector<std::pair<std::wstring, std::wstring>> deviceNames;
	static bool first = true;
	if (first) {
		auto drives = ::GetLogicalDrives();
		int drive = 0;
		while (drives) {
			if (drives & 1) {
				// drive exists
				WCHAR driveName[] = L"X:";
				driveName[0] = (WCHAR)(drive + 'A');
				WCHAR path[MAX_PATH];
				if (::QueryDosDevice(driveName, path, MAX_PATH)) {
					deviceNames.push_back({ path, driveName });
				}
			}
			drive++;
			drives >>= 1;
		}
		first = false;
	}

	for (auto& [ntName, dosName] : deviceNames) {
		if (::_wcsnicmp(name, ntName.c_str(), ntName.size()) == 0)
			return dosName + (name + ntName.size());
	}
	return L"";
}

PVOID Helpers::GetKernelBase() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return nullptr;
	}

	auto info = (RTL_PROCESS_MODULES*)buffer.get();

	return info->Modules[0].ImageBase;
}

std::string Helpers::GetNtosFileName() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return nullptr;
	}

	auto info = (RTL_PROCESS_MODULES*)buffer.get();

	std::string name;
	name = std::string((PCSTR)((BYTE*)info->Modules[0].FullPathName + info->Modules[0].OffsetToFileName));
	return name;
}

PVOID Helpers::GetWin32kBase() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return nullptr;
	}

	auto info = (RTL_PROCESS_MODULES*)buffer.get();
	auto entry = info->Modules;
	std::string win32kName = "win32k.sys";
	for (;;) {
		if (entry->ImageBase == 0)
			break;
		std::string name;
		name = std::string((PCSTR)((BYTE*)entry->FullPathName + entry->OffsetToFileName));
		if (name == win32kName) {
			return entry->ImageBase;
		}
		entry += 1;
		if (entry == nullptr)
			break;
	}

	return nullptr;
}