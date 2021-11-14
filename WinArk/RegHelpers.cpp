#include "stdafx.h"
#include "RegHelpers.h"
#include "NtDll.h"
#include "SecurityHelper.h"
#include <wil\resource.h>

#define ObjectNameInformation	(0x2)
#define ObjectTypesInformation	(0x3)



USHORT RegHelpers::GetKeyObjectTypeIndex() {
	static USHORT keyIndex = 0;
	if (keyIndex == 0) {
		const ULONG len = 1 << 14; // 16k
		auto buffer = std::make_unique<BYTE[]>(len);
		if (!NT_SUCCESS(NtQueryObject(nullptr,
			static_cast<OBJECT_INFORMATION_CLASS>(ObjectTypesInformation),
			buffer.get(), len, nullptr)))
			return 0;

		auto p = reinterpret_cast<OBJECT_TYPES_INFORMATION*>(buffer.get());
		auto raw = &p->TypeInformation[0];
		for (ULONG i = 0; i < p->NumberOfTypes; i++) {
			if (raw->TypeName.Buffer == CString(L"Key")) {
				keyIndex = raw->TypeIndex;
				break;
			}
			auto temp = (BYTE*)raw + sizeof(OBJECT_TYPE_INFORMATION) + raw->TypeName.MaximumLength;
			temp += sizeof(PVOID) - 1;
			raw = reinterpret_cast<OBJECT_TYPE_INFORMATION*>((ULONG_PTR)temp / sizeof(PVOID) * sizeof(PVOID));
		}
	}
	return keyIndex;
}

CString RegHelpers::GetObjectName(HANDLE hObeject, DWORD pid) {
	auto h = SecurityHelper::DupHandle(hObeject, pid, KEY_QUERY_VALUE);
	if (h) {
		BYTE buffer[2048];
		auto status = NtQueryObject(h, 
			static_cast<OBJECT_INFORMATION_CLASS>(ObjectNameInformation),
			buffer, sizeof(buffer), nullptr);
		::CloseHandle(h);
		if (STATUS_SUCCESS == status) {
			auto str = reinterpret_cast<UNICODE_STRING*>(buffer);
			return CString(str->Buffer, str->Length / sizeof(WCHAR));
		}
	}
	return L"";
}


CString RegHelpers::GetErrorText(DWORD error) {
	ATLASSERT(error);
	PWSTR buffer;
	CString msg;
	if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&buffer, 0, nullptr)) {
		msg = buffer;
		::LocalFree(buffer);
		msg.Trim(L"\n\r");
	}
	return msg;
}

PCWSTR RegHelpers::GetSystemDir() {
	static WCHAR dir[MAX_PATH];
	if (dir[0] == 0)
		::GetSystemDirectory(dir, _countof(dir));

	return dir;
}

PCWSTR RegHelpers::GetWindowsDir() {
	static WCHAR dir[MAX_PATH];
	if (dir[0] == 0)
		::GetWindowsDirectory(dir, _countof(dir));
	return dir;
}

std::wstring RegHelpers::GetDosNameFromNtName(PCWSTR name) {
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

CString RegHelpers::ToBinary(ULONGLONG value) {
	CString svalue;

	while (value) {
		svalue = ((value & 1) ? L"1" : L"0") + svalue;
		value >>= 1;
	}
	if (svalue.IsEmpty())
		svalue = L"0";
	return svalue;
}