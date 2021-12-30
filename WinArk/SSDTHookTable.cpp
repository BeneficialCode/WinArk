#include "stdafx.h"
#include "SSDTHookTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>

CSSDTHookTable::CSSDTHookTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	_kernelBase = Helpers::GetKernelBase();
	std::string name = Helpers::GetNtosFileName();
	std::wstring osFileName(name.begin(), name.end());
	_fileMapVA = ::LoadLibraryEx(osFileName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);

	void* kernelBase = Helpers::GetKernelBase();
	DWORD size = Helpers::GetKernelImageSize();
	char symPath[MAX_PATH];
	::GetCurrentDirectoryA(MAX_PATH, symPath);
	std::string pdbPath = "\\Symbols";
	pdbPath = symPath + pdbPath;

	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("ntk") != std::string::npos) {
			name = filename;
			break;
		}
	}
	std::string pdbFile = pdbPath + "\\" + name;
	SymbolHandler handler;
	handler.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)kernelBase, size);

	ULONG_PTR address = 0;
	address = handler.GetSymbolFromName("PiDDBCacheTable")->GetSymbolInfo()->Address;
	DriverHelper::EnumPiDDBCacheTable(address);

	auto symbol = handler.GetSymbolFromName("MmLastUnloadedDriver");
	PULONG pCount = (PULONG)symbol->GetSymbolInfo()->Address;
	ULONG count = DriverHelper::GetUnloadedDriverCount(&pCount);
	UnloadedDriverInfo info;
	info.Count = count;
	symbol = handler.GetSymbolFromName("MmUnloadedDrivers");
	info.pMmUnloadedDrivers = (void*)symbol->GetSymbolInfo()->Address;
	DriverHelper::EnumUnloadedDrivers(&info);

	symbol = handler.GetSymbolFromName("KeServiceDescriptorTable");
	PULONG KeServiceDescriptorTable = (PULONG)symbol->GetSymbolInfo()->Address;
	//ULONG offset = DriverHelper::GetShadowServiceTableOffset(&KeServiceDescriptorTable);
	//_KiServiceTable = (PULONG)(offset + (DWORD64)kernelBase);
	_KiServiceTable = (PULONG)handler.GetSymbolFromName("KiServiceTable")->GetSymbolInfo()->Address;
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	osFileName = L"\\" + osFileName;
	::wcscat_s(path, osFileName.c_str());
	PEParser parser(path);
	_imageBase = parser.GetImageBase();
	//GetSSDTEntry();
	//Refresh();
}

LRESULT CSSDTHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CSSDTHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	if (_fileMapVA)
		::FreeLibrary((HMODULE)_fileMapVA);
	return 0;
}
LRESULT CSSDTHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CSSDTHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CSSDTHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CSSDTHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CSSDTHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CSSDTHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CSSDTHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CSSDTHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CSSDTHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

int CSSDTHookTable::ParseTableEntry(CString& s, char& mask, int& select, SystemServiceInfo& info, int column) {

	// Number, Name, OrgAddress, HookType, CurAddress, TargetModule
	switch (column)
	{
		case 0:
			s.Format(L"%d (0x%-x)", info.ServiceNumber, info.ServiceNumber);
			break;
		case 1:
			s = std::wstring(info.ServiceFunctionName.begin(), info.ServiceFunctionName.end()).c_str();
			break;
		case 2:
			s.Format(L"0x%p", info.OriginalAddress);
			break;
		case 3:
			s = std::wstring(info.HookType.begin(), info.HookType.end()).c_str();
			break;
		case 4:
			s.Format(L"0x%p", info.CurrentAddress);
			break;
		case 5:
			s = std::wstring(info.TargetModule.begin(), info.TargetModule.end()).c_str();
			break;
	}

	return s.GetLength();
}

bool CSSDTHookTable::CompareItems(const SystemServiceInfo& s1, const SystemServiceInfo& s2, int col, bool asc) {

	return false;
}

void CSSDTHookTable::Refresh() {
	for (auto entry : _entries) {
		/*void* address = DriverHelper::GetSSDTApiAddress(entry.Number);
		if (address != entry.Original) {

		}*/
	}
}

ULONG_PTR CSSDTHookTable::GetOrignalAddress(DWORD number) {
	if (!_fileMapVA)
		return 0;
	if (!_KiServiceTable) {
		return 0;
	}


	uintptr_t rva = (uintptr_t)_KiServiceTable - (uintptr_t)_kernelBase;

#ifdef _WIN64
	auto pEntry = (char*)_fileMapVA + rva + 8 * number;
	// 0xFFFFFFFF00000000
	ULONGLONG value = *(ULONGLONG*)pEntry;
	
	if ((value & 0xFFFFFFFF00000000) == (_imageBase & 0xFFFFFFFF00000000)) {
		rva = *(ULONGLONG*)pEntry - _imageBase;
	}
	else {
		pEntry = (char*)_fileMapVA + rva + 4 * number;
		auto entry = *(ULONG*)pEntry;
		rva = entry;
	}

#else
	auto pEntry = (char*)_fileMapVA + (DWORD)rva + sizeof(ULONG) * number;
	auto entry = *(ULONG*)pEntry;
	rva = entry - _imageBase;
#endif 

	return rva + (ULONG_PTR)_kernelBase;
}

void CSSDTHookTable::GetSSDTEntry() {
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, sizeof(path));
	wcscat_s(path, L"\\ntdll.dll");

	PEParser parser(path);
	std::vector<ExportedSymbol> exports = parser.GetExports();
	ULONG number;
	for (auto exported : exports) {
		unsigned char* exportData = (unsigned char*)parser.RVA2FA(exported.Address);
		std::string::size_type pos = exported.Name.find("Nt");
		if (pos == exported.Name.npos) {
			continue;
		}
		for (int i = 0; i < 16; i++) {
			if (exportData[i] == 0xC2 || exportData[i] == 0xC3)  //RET
				break;
			if (exportData[i] == 0xB8)  //mov eax,X
			{
				ULONG* address = (ULONG*)(exportData + i + 1);
				number = *address;
				address += 1;
				auto opcode = *(unsigned short*)address;
				unsigned char* p = (unsigned char*)address + 5;
				auto code = *(unsigned char*)p;
				if (code != 0xFF &&// x86
					opcode != 0x04F6  // win10 x64
					&& opcode != 0x050F) { // win7 x64
					break;
				}
				//printf("Address: %llX, Name: %s index: %x\n", exported.Address, exported.Name.c_str(), index);
				_total++;
				SSDTEntry entry;
				entry.Name = exported.Name;
				entry.Number = number;
				_entries.push_back(entry);
			}
		}
	}
	std::sort(_entries.begin(), _entries.end(), [&](auto& s1, auto& s2) {
		return s1.Number < s2.Number;
		});

	std::vector<SSDTEntry> miss;
	int x = 0;// 标识初值
	for (int i = 0; i < _entries.size(); i++) {
		if (_entries[i].Number != x) {
			//printf("Miss number: %d\n", x);
			SSDTEntry entry;
			entry.Number = x;
			entry.Name = "Unexported Nt Function";
			miss.push_back(entry);
			i--;// 如果此位置缺失则继续判断此位置的数
			_total++;
		}
		x++;// 增量
	}

	for (auto entry : miss) {
		_entries.push_back(entry);
	}
	if (miss.size() > 0) {
		std::sort(_entries.begin(), _entries.end(), [&](auto& s1, auto& s2) {
			return s1.Number < s2.Number;
			});
	}

	void* kernelBase = Helpers::GetKernelBase();
	DWORD size = Helpers::GetKernelImageSize();
	char symPath[MAX_PATH];
	::GetCurrentDirectoryA(MAX_PATH, symPath);
	std::string pdbPath = "\\Symbols";
	std::string name;
	pdbPath = symPath + pdbPath;

	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("ntk") != std::string::npos) {
			name = filename;
			break;
		}
	}
	std::string pdbFile = pdbPath + "\\" + name;
	SymbolHandler handler;
	handler.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)kernelBase, size);
	
	for (auto& entry : _entries) {
		ULONG_PTR address = GetOrignalAddress(entry.Number);
		entry.Original = (void*)address;
		//address = (ULONG_PTR)DriverHelper::GetSSDTApiAddress(entry.Number);
		entry.Current = (void*)address;
		SystemServiceInfo sysService;
		sysService.ServiceNumber = entry.Number;
		sysService.OriginalAddress = (uintptr_t)entry.Original;
		DWORD64 offset = 0;
		auto symbol = handler.GetSymbolFromAddress(sysService.OriginalAddress, &offset);
		if (symbol != nullptr) {
			sysService.ServiceFunctionName = symbol->GetSymbolInfo()->Name;
		}
		else
			sysService.ServiceFunctionName = entry.Name;
		sysService.CurrentAddress = (uintptr_t)entry.Current;
		sysService.HookType = "   ---   ";
		if (entry.Original != entry.Current) {
			sysService.Hooked = true;
			sysService.HookType = "hooked";
		}
		sysService.TargetModule = Helpers::GetModuleByAddress(sysService.CurrentAddress);
		m_Table.data.info.push_back(sysService);
		m_Table.data.n = m_Table.data.info.size();
	}
}
