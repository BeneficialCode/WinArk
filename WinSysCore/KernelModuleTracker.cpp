#include "pch.h"
#include "KernelModuleTracker.h"
#include <PEParser.h>
#include "Helpers.h"

using namespace WinSys;

uint32_t WinSys::KernelModuleTracker::EnumModules() {
	_modules.clear();
	DWORD size = 1 << 18;
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size,MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	PVOID SystemRangeStart = 0;
	NtQuerySystemInformation(SystemRangeStartInformation,
		&SystemRangeStart,
		sizeof(SystemRangeStart),
		NULL);
	

	NTSTATUS status;
	status = ::NtQuerySystemInformation(SystemModuleInformationEx, buffer.get(), size, nullptr);
	if (!NT_SUCCESS(status))
		return 0;

	if (_modules.empty()) {
		_modules.reserve(256);
	}

	auto p = (RTL_PROCESS_MODULE_INFORMATION_EX*)buffer.get();
	CHAR winDir[MAX_PATH];
	::GetWindowsDirectoryA(winDir, _countof(winDir));
	static const std::string root("\\SystemRoot\\");
	static const std::string global("\\??\\");
	
	for (;;) {
		if (p->BaseInfo.ImageBase == 0)
			break;
		auto m = std::make_shared<KernelModuleInfo>();
		m->Flags = p->BaseInfo.Flags;
		m->FullPath = (const char*)p->BaseInfo.FullPathName;
		m->NtPath = m->FullPath;
		if (m->FullPath.find(global) == 0)
			m->FullPath = m->FullPath.substr(global.size());
		if (m->FullPath.find(root) == 0)
			m->FullPath = winDir + m->FullPath.substr(root.size() - 1);

		if (p->BaseInfo.ImageBase < SystemRangeStart) {
			if (p->NextOffset == 0)
				break;
			p = (RTL_PROCESS_MODULE_INFORMATION_EX*)((BYTE*)p + p->NextOffset);
			continue;
		}

		std::wstring path = Helpers::StringToWstring(m->FullPath);
		PEParser parser(path.c_str());
		if (parser.IsValid()) {
			auto type = parser.GetSubsystemType();
			if (type != SubsystemType::Native) {
				if (p->NextOffset == 0)
					break;
				p = (RTL_PROCESS_MODULE_INFORMATION_EX*)((BYTE*)p + p->NextOffset);
				continue;
			}
		}
		
		m->ImageBase = p->BaseInfo.ImageBase;
		m->MappedBase = p->BaseInfo.MappedBase;
		m->ImageSize = p->BaseInfo.ImageSize;
		m->InitOrderIndex = p->BaseInfo.InitOrderIndex;
		m->LoadOrderIndex = p->BaseInfo.LoadOrderIndex;
		m->LoadCount = p->BaseInfo.LoadCount;
		m->hSection = p->BaseInfo.Section;
		m->DefaultBase = p->DefaultBase;
		m->ImageChecksum = p->ImageChecksum;
		m->TimeDateStamp = p->TimeDateStamp;
		m->Name = std::string((PCSTR)((BYTE*)p->BaseInfo.FullPathName + p->BaseInfo.OffsetToFileName));

		_modules.push_back(std::move(m));

		if (p->NextOffset == 0)
			break;
		p = (RTL_PROCESS_MODULE_INFORMATION_EX*)((BYTE*)p + p->NextOffset);
	}

	return uint32_t(_modules.size());
}

const std::vector<std::shared_ptr<KernelModuleInfo>>& WinSys::KernelModuleTracker::GetModules() const {
	return _modules;
}
