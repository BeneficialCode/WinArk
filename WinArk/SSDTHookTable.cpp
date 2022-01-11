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
	std::wstring osFileName = Helpers::StringToWstring(name);
	_fileMapVA = ::LoadLibraryEx(osFileName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES| LOAD_LIBRARY_SEARCH_SYSTEM32);

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
	_KiServiceTable = (PULONG)handler.GetSymbolFromName("KiServiceTable")->GetSymbolInfo()->Address;
	auto symbol = handler.GetSymbolFromName("KeServiceDescriptorTableShadow");
	PULONG address = (PULONG)symbol->GetSymbolInfo()->Address;
	// KiServiceLimit
	_limit = DriverHelper::GetServiceLimit(&address);
	DriverHelper::InitNtServiceTable(&address);
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	osFileName = L"\\" + osFileName;
	::wcscat_s(path, osFileName.c_str());
	PEParser parser(path);
	_imageBase = parser.GetImageBase();

	GetSSDTEntry();
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
			s = Helpers::StringToWstring(info.ServiceFunctionName).c_str();
			break;
		case 2:
			s.Format(L"0x%p", info.OriginalAddress);
			break;
		case 3:
			s = Helpers::StringToWstring(info.HookType).c_str();
			break;
		case 4:
			s.Format(L"0x%p", info.CurrentAddress);
			break;
		case 5:
			s = Helpers::StringToWstring(info.TargetModule).c_str();
			break;
	}

	return s.GetLength();
}

bool CSSDTHookTable::CompareItems(const SystemServiceInfo& s1, const SystemServiceInfo& s2, int col, bool asc) {

	return false;
}

void CSSDTHookTable::Refresh() {
	for (int i = 0; i < _limit;i++) {
		void* address = DriverHelper::GetSSDTApiAddress(i);
		if (m_Table.data.info[i].OriginalAddress != (uintptr_t)address) {
			m_Table.data.info[i].Hooked = true;
			m_Table.data.info[i].HookType = "hooked";
		}
		else {
			m_Table.data.info[i].Hooked = false;
			m_Table.data.info[i].HookType = "   ---   ";
		}
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

	ATLTRACE("%s", name.c_str());

	std::string pdbFile = pdbPath + "\\" + name;
	SymbolHandler handler;
	handler.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)kernelBase, size);

	for (decltype(_limit) i = 0; i < _limit; i++) {
		SystemServiceInfo info;
		info.ServiceNumber = i;
		ULONG_PTR address = GetOrignalAddress(i);
		info.OriginalAddress = address;
		address = (ULONG_PTR)DriverHelper::GetSSDTApiAddress(info.ServiceNumber);
		info.CurrentAddress = address;

		DWORD64 offset = 0;
		auto symbol = handler.GetSymbolFromAddress(address, &offset);
		symbol = handler.GetSymbolFromAddress(info.OriginalAddress);
		if (symbol != nullptr) {
			info.ServiceFunctionName = symbol->GetSymbolInfo()->Name;
		}
		else
			info.ServiceFunctionName = "Unknown";
		info.HookType = "   ---   ";
		if (info.OriginalAddress != info.CurrentAddress) {
			info.Hooked = true;
			info.HookType = "hooked";
		}
		info.TargetModule = Helpers::GetModuleByAddress(info.CurrentAddress);
		m_Table.data.info.push_back(info);
		m_Table.data.n = m_Table.data.info.size();
	}
}
