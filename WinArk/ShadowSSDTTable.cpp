#include "stdafx.h"
#include "ShadowSSDTTable.h"
#include <Helpers.h>
#include <PEParser.h>
#include <DriverHelper.h>
#include <SymbolHandler.h>
#include <filesystem>
#include "RegHelpers.h"
#include "SymbolHelper.h"
#include "ClipboardHelper.h"


CShadowSSDTHookTable::CShadowSSDTHookTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	_win32kBase = Helpers::GetWin32kBase();
	std::wstring sysDir = RegHelpers::GetSystemDir();
	std::wstring osFileName = sysDir + L"\\win32k.sys";
	_fileMapVA = ::LoadLibraryEx(osFileName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	PEParser parser(osFileName.c_str());


	PULONG KeServiceDescriptorShadow = (PULONG)SymbolHelper::GetKernelSymbolAddressFromName("KeServiceDescriptorTableShadow");
	_serviceTableBase = DriverHelper::GetShadowServiceTable(&KeServiceDescriptorShadow);
	static PULONG address = (PULONG)((PUCHAR)KeServiceDescriptorShadow + sizeof(SystemServiceTable));
	_imageBase = parser.GetImageBase();
	_limit = DriverHelper::GetServiceLimit(&address);
}

LRESULT CShadowSSDTHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	GetShadowSSDTEntry();
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
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_SHADOW_SSDT_CONTEXT);
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

bool CShadowSSDTHookTable::CompareItems(const ShadowSystemServiceInfo& s1, const ShadowSystemServiceInfo& s2, int col, bool asc) {

	return false;
}

ULONG_PTR CShadowSSDTHookTable::GetOrignalAddress(DWORD number) {
	if (!_fileMapVA)
		return 0;
	if (!_serviceTableBase)
		return 0;

	uintptr_t rva = (uintptr_t)_serviceTableBase - (uintptr_t)_win32kBase;
	ULONGLONG imageBase = (ULONGLONG)_fileMapVA;
#ifdef _WIN64
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

	return rva + (ULONG_PTR)_win32kBase;
}

void CShadowSSDTHookTable::GetShadowSSDTEntry() {
	for (decltype(_limit) i = 0; i < _limit; i++) {
		ShadowSystemServiceInfo info;
		info.ServiceNumber = i;
		ULONG_PTR address = GetOrignalAddress(i);
		info.OriginalAddress = address;
		address = (ULONG_PTR)DriverHelper::GetShadowSSDTApiAddress(i);
		info.CurrentAddress = address;
		DWORD64 offset = 0;
		auto symbol = SymbolHelper::GetSymbolFromAddress(info.OriginalAddress, &offset);
		if (symbol != nullptr) {
			info.ServiceFunctionName = symbol->GetSymbolInfo()->Name;
		}
		else
			info.ServiceFunctionName = "NtUnknown";
		info.HookType = "   ---   ";
		if (info.OriginalAddress != info.CurrentAddress) {
			info.Hooked = true;
			info.HookType = "   hooked   ";
		}
		info.TargetModule = Helpers::GetKernelModuleByAddress(info.CurrentAddress);
		m_Table.data.info.push_back(info);
		m_Table.data.n = m_Table.data.info.size();
	}
}

void CShadowSSDTHookTable::Refresh() {
	for (ULONG i = 0; i < _limit; i++) {
		void* address = DriverHelper::GetShadowSSDTApiAddress(i);
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

LRESULT CShadowSSDTHookTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	return TRUE;
}

LRESULT CShadowSSDTHookTable::OnShadowSSDTCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	CString text = GetSingleShadowSSDTInfo(info).c_str();

	ClipboardHelper::CopyText(m_hWnd, text);
	return 0;
}

LRESULT CShadowSSDTHookTable::OnShadowSSDTExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleShadowSSDTInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CShadowSSDTHookTable::GetSingleShadowSSDTInfo(ShadowSystemServiceInfo& info) {
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