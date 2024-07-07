#include "stdafx.h"
#include "SSDTHookTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>
#include "RegHelpers.h"
#include "SymbolHelper.h"
#include "ClipboardHelper.h"

CSSDTHookTable::CSSDTHookTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	_kernelBase = Helpers::GetKernelBase();
	std::string name = Helpers::GetNtosFileName();
	std::wstring osFileName = RegHelpers::GetSystemDir();
	osFileName = osFileName + L"\\" + Helpers::StringToWstring(name);
	_fileMapVA = ::LoadLibraryEx(osFileName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);

	_KiServiceTable = (PULONG)SymbolHelper::GetKernelSymbolAddressFromName("KiServiceTable");
	PULONG address = (PULONG)SymbolHelper::GetKernelSymbolAddressFromName("KeServiceDescriptorTableShadow");
	// KiServiceLimit
	_limit = DriverHelper::GetServiceLimit(&address);
	DriverHelper::InitNtServiceTable(&address);

	PEParser parser(osFileName.c_str());
	_imageBase = parser.GetImageBase();

	
}

LRESULT CSSDTHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	GetSSDTEntry();
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
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_SSDT_CONTEXT);
	hSubMenu = menu.GetSubMenu(0);
	POINT pt;
	::GetCursorPos(&pt);
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);

	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
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
	for (ULONG i = 0; i < _limit; i++) {
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
	ULONG_PTR imageBase = (ULONG_PTR)_fileMapVA;
#ifdef _WIN64
	// On 64-bit Windows systems, the stored values in SSDT are not absolute address
	auto CheckAddressMethod = [&]()->bool {
		auto pEntry = (char*)_fileMapVA + rva + 8 * number;
		ULONGLONG value = *(ULONGLONG*)pEntry;
		uintptr_t v1 = value - _imageBase;
		uintptr_t v2 = value - imageBase;
		if (v1 > DWORD_MAX && v2 > DWORD_MAX) {
			return true;
		}
		return false;
	};
	static bool use4bytes = CheckAddressMethod();
	if (use4bytes) {
		auto pEntry = (ULONG*)((char*)_fileMapVA + rva);
		auto entry = pEntry[number];
		rva = entry;
	}
	else {
		auto pEntry = (ULONGLONG*)((char*)_fileMapVA + rva);
		ULONGLONG value = pEntry[number];
		uintptr_t v1 = value - _imageBase;
		uintptr_t v2 = value - imageBase;
		if (v1 < DWORD_MAX)
			rva = v1;
		else
			rva = v2;
	}
#else
	auto pEntry = (ULONG*)((char*)_fileMapVA + rva);
	ULONG value = pEntry[number];
	ULONG v1 = value - _imageBase;
	ULONG v2 = value - imageBase;
	ULONG high1 = (value & 0xF000000);
	ULONG high2 = (imageBase & 0xF000000);
	bool isNeedFix = false;
	if (high1 == high2) {
		isNeedFix = true;
	}
	if (v1 < DWORD_MAX && !isNeedFix) {
		rva = v1;
	}
	else
		rva = v2;
#endif 

	return rva + (ULONG_PTR)_kernelBase;
}

void CSSDTHookTable::GetSSDTEntry() {

	for (decltype(_limit) i = 0; i < _limit; i++) {
		SystemServiceInfo info;
		info.ServiceNumber = i;
		ULONG_PTR address = GetOrignalAddress(i);
		info.OriginalAddress = address;
		address = (ULONG_PTR)DriverHelper::GetSSDTApiAddress(info.ServiceNumber);
		info.CurrentAddress = address;

		DWORD64 offset = 0;
		auto symbol = SymbolHelper::GetSymbolFromAddress(info.OriginalAddress, &offset);
		if (symbol != nullptr) {
			info.ServiceFunctionName = symbol->GetSymbolInfo()->Name;
		}
		else
			info.ServiceFunctionName = "Unknown";
		info.HookType = "   ---   ";
		if (info.OriginalAddress != info.CurrentAddress) {
			info.Hooked = true;
			info.HookType = "  hooked";
		}
		info.TargetModule = Helpers::GetKernelModuleByAddress(info.CurrentAddress);
		m_Table.data.info.push_back(info);
		m_Table.data.n = m_Table.data.info.size();
	}
}

LRESULT CSSDTHookTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	return TRUE;
}

LRESULT CSSDTHookTable::OnSSDTCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];
	CString text = GetSingleSSDTInfo(info).c_str();
	ClipboardHelper::CopyText(m_hWnd, text);
	return TRUE;
}

LRESULT CSSDTHookTable::OnSSDTExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleSSDTInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CSSDTHookTable::GetSingleSSDTInfo(SystemServiceInfo& info) {
	CString text;
	CString s;

	s.Format(L"%d (0x%-x)", info.ServiceNumber, info.ServiceNumber);
	s += L"\t";
	text += s;

	s = Helpers::StringToWstring(info.ServiceFunctionName).c_str();
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.OriginalAddress);
	s += L"\t";
	text += s;

	s = Helpers::StringToWstring(info.HookType).c_str();
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.CurrentAddress);
	s += L"\t";
	text += s;

	s = Helpers::StringToWstring(info.TargetModule).c_str();
	s += L"\t";
	text += s;

	text += L"\r\n";

	return text.GetString();
}
