#include "stdafx.h"
#include "UnloadedDriverTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>
#include "FormatHelper.h"
#include "ClipboardHelper.h"
#include "SymbolHelper.h"

LRESULT CUnloadedDriverTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CUnloadedDriverTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CUnloadedDriverTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CUnloadedDriverTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CUnloadedDriverTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(1);
	POINT pt;
	::GetCursorPos(&pt);
	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}

	return 0;
}
LRESULT CUnloadedDriverTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CUnloadedDriverTable::CUnloadedDriverTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CUnloadedDriverTable::ParseTableEntry(CString& s, char& mask, int& select, UnloadedDriverInfo& info, int column) {
	switch (column)
	{
		case 0: // Name
			s = info.DriverName.c_str();
			break;

		case 1: // StartAddress
			s.Format(L"0x%p", info.StartAddress);
			break;

		case 2: // EndAddress
			s.Format(L"0x%p", info.EndAddress);
			break;

		case 3: // CurrentTime
			s = FormatHelper::TimeToString(info.CurrentTime.QuadPart);
			break;
		default:
			break;
	}
	return s.GetLength();
}

bool CUnloadedDriverTable::CompareItems(const UnloadedDriverInfo& s1, const UnloadedDriverInfo& s2, int col, bool asc) {

	return false;
}

void CUnloadedDriverTable::Refresh() {
	static PULONG pCount = (PULONG)SymbolHelper::GetKernelSymbolAddressFromName("MmLastUnloadedDriver");
	ULONG count = DriverHelper::GetUnloadedDriverCount(&pCount);
	UnloadedDriversInfo info;
	info.Count = count;

	static ULONG64 address = SymbolHelper::GetKernelSymbolAddressFromName("MmUnloadedDrivers");
	info.pMmUnloadedDrivers = (void*)address;

	ULONG len = DriverHelper::GetUnloadedDriverDataSize(&info);

	if (len > sizeof UnloadedDriverData) {
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		DriverHelper::EnumUnloadedDrivers(&info, buffer.get(), len);

		UnloadedDriverData* p = (UnloadedDriverData*)buffer.get();
		UnloadedDriverInfo item;
		m_Table.data.info.clear();
		m_Table.data.n = 0;
		for (;;) {
			item.CurrentTime = p->CurrentTime;
			item.DriverName = (WCHAR*)((PUCHAR)p + p->StringOffset);
			item.StartAddress = p->StartAddress;
			item.EndAddress = p->EndAddress;
			m_Table.data.info.push_back(item);
			if (p->NextEntryOffset == 0)
				break;
			p = (UnloadedDriverData*)((PUCHAR)p + p->NextEntryOffset);
			if (p == nullptr)
				break;
		}
	}
	
	m_Table.data.n = m_Table.data.info.size();
}

std::wstring CUnloadedDriverTable::GetSingleUnloadedDriverInfo(UnloadedDriverInfo& info) {
	CString text;
	CString s;

	s = info.DriverName.c_str();
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.StartAddress);
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.EndAddress);
	s += L"\t";
	text += s;

	s = FormatHelper::TimeToString(info.CurrentTime.QuadPart);
	s += L"\t";
	text += s;

	text += L"\r\n";

	return text.GetString();
}

LRESULT CUnloadedDriverTable::OnUnloadedDriverCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	CString text;
	CString s;

	s = info.DriverName.c_str();
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.StartAddress);
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.EndAddress);
	s += L"\t";
	text += s;

	s = FormatHelper::TimeToString(info.CurrentTime.QuadPart);
	s += L"\t";
	text += s;

	text += L"\r\n";

	ClipboardHelper::CopyText(m_hWnd, text);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUnloadedDriverExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleUnloadedDriverInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

LRESULT CUnloadedDriverTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	return TRUE;
}