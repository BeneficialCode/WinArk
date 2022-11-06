#include "pch.h"
#include "ApiSets.h"
#include <ntpebteb.h>

#define API_SET_SCHEMA_ENTRY_FLAGS_SEALED 1

static bool GetProcessPeb(HANDLE hProcess, PPEB peb) {
	PROCESS_BASIC_INFORMATION info;
	if (!NT_SUCCESS(::NtQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), nullptr)))
		return false;

	return ::ReadProcessMemory(hProcess, info.PebBaseAddress, peb, sizeof(*peb), nullptr);
}

const std::vector<ApiSetEntry>& ApiSets::GetApiSets() const {
	return _entries;
}

bool ApiSets::IsFileExists(const wchar_t* name) const {
	return _files.find(name) != _files.end();
}

void ApiSets::Build(HANDLE hProcess) {
	PEB peb;
	GetProcessPeb(hProcess, &peb);

	API_SET_NAMESPACE apiSetMap;
	::ReadProcessMemory(hProcess, peb.ApiSetMap, &apiSetMap, sizeof(API_SET_NAMESPACE), nullptr);

	auto apiSetMapAsNumber = reinterpret_cast<ULONG_PTR>(peb.ApiSetMap);
	
	PAPI_SET_NAMESPACE_ENTRY pNsEntry = reinterpret_cast<PAPI_SET_NAMESPACE_ENTRY>(apiSetMapAsNumber + apiSetMap.EntryOffset);

	_entries.reserve(apiSetMap.Count);

	for (ULONG i = 0; i < apiSetMap.Count; i++) {
		ApiSetEntry entry;
		API_SET_NAMESPACE_ENTRY nsEntry;
		::ReadProcessMemory(hProcess, pNsEntry, &nsEntry, sizeof(API_SET_NAMESPACE_ENTRY), nullptr);

		std::wstring name;
		SIZE_T size = static_cast<int>(nsEntry.NameLength / sizeof(WCHAR));
		name.resize(size);
		void* pName = reinterpret_cast<void*>(apiSetMapAsNumber + nsEntry.NameOffset);
		::ReadProcessMemory(hProcess, pName, name.data(), nsEntry.NameLength, nullptr);
		entry.Name = name;
		entry.Sealed = (nsEntry.Flags & API_SET_SCHEMA_ENTRY_FLAGS_SEALED) != 0;

		PAPI_SET_VALUE_ENTRY pValueEntry = reinterpret_cast<PAPI_SET_VALUE_ENTRY>(apiSetMapAsNumber + nsEntry.ValueOffset);
		
		for (ULONG j = 0; j < nsEntry.ValueCount; j++) {
			API_SET_VALUE_ENTRY valueEntry;
			::ReadProcessMemory(hProcess, pValueEntry, &valueEntry, sizeof(valueEntry), nullptr);
			name.clear();
			size = valueEntry.ValueLength / sizeof(WCHAR);
			name.resize(size);
			void* pValue = reinterpret_cast<void*>(apiSetMapAsNumber + valueEntry.ValueOffset);
			::ReadProcessMemory(hProcess, pValue, name.data(), valueEntry.ValueLength, nullptr);
			std::wstring value(name);
			entry.Values.push_back(value);
			if (valueEntry.NameLength != 0) {
				name.clear();
				auto pValueName = reinterpret_cast<void*>(apiSetMapAsNumber + valueEntry.NameOffset);
				size = valueEntry.NameLength / sizeof(WCHAR);
				name.resize(size);
				::ReadProcessMemory(hProcess, pValueName, name.data(), valueEntry.NameLength, nullptr);
				std::wstring alias(name);
				entry.Aliases.push_back(alias);
			}
			pValueEntry++;
		}
		pNsEntry++;
		_entries.push_back(entry);
	}
}

void ApiSets::SearchFiles() {
	WCHAR path[MAX_PATH];

	::GetSystemDirectory(path, MAX_PATH);

	SearchDirectory(path, L"api-ms-win-", _files);
}

void ApiSets::SearchDirectory(const std::wstring& directory, const std::wstring& pattern, ApiSets::FileSet& files) {
	WIN32_FIND_DATA fd;
	std::wstring fileName = directory + L"\\*";
	wil::unique_hfind hFind(::FindFirstFileEx(fileName.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0));
	if (!hFind)
		return;

	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			SearchDirectory(directory + L"\\" + fd.cFileName, pattern, files);
			continue;
		}

		if (::_wcsnicmp(fd.cFileName, pattern.c_str(), pattern.length()) == 0)
			files.insert(fd.cFileName);
	} while (::FindNextFile(hFind.get(),&fd));
}