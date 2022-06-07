#include "pch.h"
#include "Helpers.h"

#include "SecurityHelper.h"
#include <wil\resource.h>
#include "KernelModuleTracker.h"
#include "ProcessModuleTracker.h"


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

DWORD Helpers::GetKernelImageSize() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return 0;
	}

	auto info = (RTL_PROCESS_MODULES*)buffer.get();
	return info->Modules[0].ImageSize;
}

std::string Helpers::GetNtosFileName() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return "";
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

DWORD Helpers::GetWin32kImageSize() {
	ULONG size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	status = ::NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
		buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status)) {
		return 0;
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
			return entry->ImageSize;
		}
		entry += 1;
		if (entry == nullptr)
			break;
	}

	return 0;
}

std::string Helpers::GetKernelModuleByAddress(ULONG_PTR address) {
	WinSys::KernelModuleTracker m_Tracker;

	auto count = m_Tracker.EnumModules();
	auto modules = m_Tracker.GetModules();
	for (decltype(count) i = 0; i < count; i++) {
		auto m = modules[i];
		ULONG_PTR limit = (ULONG_PTR)((char*)m->ImageBase + m->ImageSize);
		if (address > (ULONG_PTR)m->ImageBase && address < limit) {
			return m->FullPath;
		}
	}

	return "";
}

std::string Helpers::GetUserModuleByAddress(ULONG_PTR address, ULONG pid) {
	std::string moduleName = "";
	if (pid == 0) {
		return GetKernelModuleByAddress(address);
	}
	WinSys::ProcessModuleTracker m_Tracker(pid);
	auto count = m_Tracker.EnumModules();
	auto modules = m_Tracker.GetModules();
	for (decltype(count) i = 0; i < count; i++) {
		auto m = modules[i];
		ULONG_PTR limit = (ULONG_PTR)((char*)m->Base + m->ModuleSize);
		if (address > (ULONG_PTR)m->Base && address < limit) {
			moduleName = WstringToString(m->Path);
		}
	}

	if (moduleName == "" && address != 0)
		return GetKernelModuleByAddress(address);
	return moduleName;
}

std::wstring Helpers::StringToWstring(const std::string& str) {
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	len += 1;
	std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(len);
	memset(buffer.get(), 0, sizeof(wchar_t) * len);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer.get(), len);
	std::wstring wstr(buffer.get());
	return wstr;
}

std::string Helpers::WstringToString(const std::wstring& wstr) {
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0, nullptr, nullptr);
	len += 1;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(len);
	memset(buffer.get(), 0, sizeof(char) * len);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer.get(), len, nullptr, nullptr);
	std::string str(buffer.get());
	return str;
}

std::wstring Helpers::GetDriverDirFromObjectManager(std::wstring serviceName) {
	DWORD size = 1 << 16;
	wil::unique_virtualalloc_ptr<> buffer(
		::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	NTSTATUS status;
	HANDLE hDirectory = INVALID_HANDLE_VALUE;
	UNICODE_STRING directoryName;
	std::wstring folder;
	folder = L"\\driver";
	RtlInitUnicodeString(&directoryName, folder.c_str());
	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &directoryName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	status = ::NtOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &objAttr);
	if (!NT_SUCCESS(status)) {
		return L"";
	}
	ULONG index = 0;
	ULONG returned;
	status = ::NtQueryDirectoryObject(hDirectory, buffer.get(), 1 << 16, FALSE, TRUE, &index, &returned);
	if (NT_SUCCESS(status)) {
		auto info = (OBJECT_DIRECTORY_INFORMATION*)buffer.get();
		for (int i = 0; i < index; i++) {
			std::wstring name(info[i].Name.Buffer, info[i].Name.Length);
			if (name.find(serviceName) != name.npos) {
				return folder;
			}
		}
	}

	folder = L"\\filesystem";
	RtlInitUnicodeString(&directoryName, folder.c_str());
	InitializeObjectAttributes(&objAttr, &directoryName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	status = ::NtOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &objAttr);
	if (!NT_SUCCESS(status)) {
		return L"";
	}
	status = ::NtQueryDirectoryObject(hDirectory, buffer.get(), 1 << 16, FALSE, TRUE, &index, &returned);
	if (NT_SUCCESS(status)) {
		auto info = (OBJECT_DIRECTORY_INFORMATION*)buffer.get();
		for (int i = 0; i < index; i++) {
			std::wstring name(info[i].Name.Buffer, info[i].Name.Length);
			if (name.find(serviceName) != name.npos) {
				return folder;
			}
		}
	}
	return L"";
}