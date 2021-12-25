#include "stdafx.h"
#include "ShadowSSDTTable.h"
#include <Helpers.h>
#include <PEParser.h>
#include <DriverHelper.h>
#include <SymbolHandler.h>
#include <filesystem>

CShadowSSDTHookTable::CShadowSSDTHookTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	_win32kBase = Helpers::GetWin32kBase();
	_fileMapVA = ::LoadLibraryEx(L"win32k.sys", nullptr, DONT_RESOLVE_DLL_REFERENCES);
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	::wcscat_s(path, L"\\win32k.sys");
	PEParser parser(path);

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
	auto symbol = handler.GetSymbolFromName("KeServiceDescriptorTableShadow");
	PULONG KeServiceDescriptorTableShadow = (PULONG)symbol->GetSymbolInfo()->Address;
	ULONG offset = DriverHelper::GetShadowServiceTableOffset(&KeServiceDescriptorTableShadow);
	_serviceTableBase = (PULONG)(offset + (char*)kernelBase);
	_imageBase = parser.GetImageBase();
	ULONG_PTR base = (ULONG_PTR)parser.GetBaseAddress();
	_limit = DriverHelper::GetServiceLimit(&_serviceTableBase);

	GetShadowSSDTEntry();
}

LRESULT CShadowSSDTHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CShadowSSDTHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	if (_fileMapVA)
		::FreeLibrary((HMODULE)_fileMapVA);
	return 0;
}
LRESULT CShadowSSDTHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CShadowSSDTHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CShadowSSDTHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

int CShadowSSDTHookTable::ParseTableEntry(CString& s, char& mask, int& select, ShadowSystemServiceInfo& info, int column) {
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

bool CShadowSSDTHookTable::CompareItems(const ShadowSystemServiceInfo& s1, const ShadowSystemServiceInfo& s2, int col, bool asc) {

	return false;
}

ULONG_PTR CShadowSSDTHookTable::GetOrignalAddress(DWORD number) {
	if (!_fileMapVA)
		return 0;
	if (!_serviceTableBase)
		return 0;

	uintptr_t rva = (uintptr_t)_serviceTableBase - (uintptr_t)_win32kBase;

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

	return rva + (ULONG_PTR)_win32kBase;
}

void CShadowSSDTHookTable::GetShadowSSDTEntry() {
	void* win32kBase = Helpers::GetWin32kBase();
	DWORD size = Helpers::GetWin32kImageSize();
	char symPath[MAX_PATH];
	::GetCurrentDirectoryA(MAX_PATH, symPath);
	std::string pdbPath = "\\Symbols";
	std::string name;
	pdbPath = symPath + pdbPath;

	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("win32") != std::string::npos) {
			name = filename;
			break;
		}
	}
	std::string pdbFile = pdbPath + "\\" + name;
	SymbolHandler handler;
	handler.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)win32kBase, size);

	for (decltype(_limit) i = 0; i < _limit; i++) {
		ShadowSystemServiceInfo info;
		info.ServiceNumber = i;
		ULONG_PTR address = GetOrignalAddress(i);
		info.OriginalAddress = address;
		address = (ULONG_PTR)DriverHelper::GetShadowSSDTApiAddress(i);
		info.CurrentAddress = address;
		DWORD64 offset = 0;
		auto symbol = handler.GetSymbolFromAddress(info.OriginalAddress, &offset);
		if (symbol != nullptr) {
			info.ServiceFunctionName = symbol->GetSymbolInfo()->Name;
		}
		else
			info.ServiceFunctionName = "NtUnknown";
		info.HookType = "   ---   ";
		if (info.OriginalAddress != info.CurrentAddress) {
			info.Hooked = true;
			info.HookType= "   hooked   ";
		}
		info.TargetModule = Helpers::GetModuleByAddress(info.CurrentAddress);
		m_Table.data.info.push_back(info);
		m_Table.data.n = m_Table.data.info.size();
	}
}