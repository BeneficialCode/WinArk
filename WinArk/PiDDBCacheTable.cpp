#include "stdafx.h"
#include "PiDDBCacheTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>
#include "RegHelpers.h"
#include "FormatHelper.h"
#include "ClipboardHelper.h"
#include "SymbolHelper.h"

LRESULT CPiDDBCacheTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CPiDDBCacheTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CPiDDBCacheTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CPiDDBCacheTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CPiDDBCacheTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(0);
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
LRESULT CPiDDBCacheTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CPiDDBCacheTable::CPiDDBCacheTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CPiDDBCacheTable::ParseTableEntry(CString& s, char& mask, int& select, PiDDBCacheInfo& info, int column) {
	// Name,LoadStatus,TimeDateStamp
	switch (column)
	{
		case 0:
			s = info.DriverName.c_str();
			break;

		case 1:
			s = RegHelpers::GetErrorText(info.LoadStatus);
			break;

		case 2:
		{
			// https://devblogs.microsoft.com/oldnewthing/20180103-00/?p=97705
			time_t t = (time_t)info.TimeDateStamp;
			CString time = CTime(t).Format(L"%A, %B %d, %Y");
			CString stamp;
			stamp.Format(L"0x%X ", info.TimeDateStamp);
			s = stamp + time;
			break;
		}
		default:
			break;
	}
	return s.GetLength();
}

bool CPiDDBCacheTable::CompareItems(const PiDDBCacheInfo& s1, const PiDDBCacheInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:
			return SortHelper::SortStrings(s1.DriverName, s2.DriverName, asc);
			break;
		default:
			break;
	}
	return false;
}

void CPiDDBCacheTable::Refresh() {
	static ULONG_PTR address = SymbolHelper::GetKernelSymbolAddressFromName("PiDDBCacheTable");
	if (address != 0) {
		ULONG len = DriverHelper::GetPiDDBCacheDataSize(address);

		if (len > sizeof PiDDBCacheData) {
			wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
			DriverHelper::EnumPiDDBCacheTable(address, buffer.get(), len);

			PiDDBCacheData* p = (PiDDBCacheData*)buffer.get();

			PiDDBCacheInfo info;
			m_Table.data.info.clear();
			m_Table.data.n = 0;
			for (;;) {
				info.LoadStatus = p->LoadStatus;
				info.DriverName = (WCHAR*)((PUCHAR)p + p->StringOffset);
				info.TimeDateStamp = p->TimeDateStamp;
				m_Table.data.info.push_back(info);
				if (p->NextEntryOffset == 0)
					break;
				p = (PiDDBCacheData*)((PUCHAR)p + p->NextEntryOffset);
				if (p == nullptr)
					break;
			}
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

std::wstring CPiDDBCacheTable::GetSinglePiDDBCacheInfo(PiDDBCacheInfo& info) {
	CString text;
	CString s;

	s = info.DriverName.c_str();
	s += L"\t";
	text += s;

	s = RegHelpers::GetErrorText(info.LoadStatus);
	s += L"\t";
	text += s;

	time_t t = (time_t)info.TimeDateStamp;
	CString time = CTime(t).Format(L"%A, %B %d, %Y");
	CString stamp;
	stamp.Format(L"0x%X ", info.TimeDateStamp);
	s = stamp + time;
	text += s;

	text += L"\r\n";

	return text.GetString();
}

LRESULT CPiDDBCacheTable::OnPiDDBCacheCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	CString text;
	CString s;

	s = info.DriverName.c_str();
	s += L"\t";
	text += s;

	s = RegHelpers::GetErrorText(info.LoadStatus);
	s += L"\t";
	text += s;

	time_t t = (time_t)info.TimeDateStamp;
	CString time = CTime(t).Format(L"%A, %B %d, %Y");
	CString stamp;
	stamp.Format(L"0x%X ", info.TimeDateStamp);
	s = stamp + time;
	text += s;

	ClipboardHelper::CopyText(m_hWnd, text);
	return 0;
}

LRESULT CPiDDBCacheTable::OnPiDDBCacheExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSinglePiDDBCacheInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

LRESULT CPiDDBCacheTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

void CPiDDBCacheTable::DoFind(const CString& text, DWORD flags) {
	auto searchDown = flags & FR_DOWN;

	int start = 0;
	CString find(text);
	auto ignoreCase = !(flags & FR_MATCHCASE);
	if (ignoreCase)
		find.MakeLower();

	int from = searchDown ? start + 1 : static_cast<int>(start - 1 + m_Table.data.n);
	int to = searchDown ? static_cast<int>(m_Table.data.n + start) : start + 1;
	int step = searchDown ? 1 : -1;

	int findIndex = -1;
	for (int i = from; i != to; i += step) {
		int index = i % m_Table.data.n;
		const auto& item = m_Table.data.info[i];
		CString text(item.DriverName.c_str());
		if (ignoreCase)
			text.MakeLower();
		if (text.Find(find) >= 0) {
			findIndex = index;
			break;
		}
		
		text.Format(L"0x%X ", item.TimeDateStamp);
		if (ignoreCase)
			text.MakeLower();
		if (text.Find(find) >= 0) {
			findIndex = index;
			break;
		}
	}
	RECT client, bar;
	GetClientRect(&client);
	memcpy(&bar, &client, sizeof(bar));
	if (m_Table.showbar == 1) {
		client.top = g_AvHighFont + 4;
		bar.bottom = g_AvHighFont + 4;
	}
	else {
		bar.bottom = bar.top;
	}
	int rows = (client.bottom - client.top) / g_AvHighFont;
	if (findIndex >= 0) {
		m_Table.offset = findIndex - findIndex%rows;
		m_Table.data.selected = findIndex;
		InvalidateRect(nullptr);
	}
	else
		AtlMessageBox(m_hWnd, L"Not found");
}