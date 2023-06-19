#include "stdafx.h"
#include "WinExtHostsTable.h"
#include "SymbolHelper.h"
#include "Helpers.h"
#include "ExtensionTableDlg.h"


LRESULT CWinExtHostsTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CWinExtHostsTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CWinExtHostsTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CWinExtHostsTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWinExtHostsTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWinExtHostsTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWinExtHostsTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWinExtHostsTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWinExtHostsTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CWinExtHostsTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWinExtHostsTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWinExtHostsTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWinExtHostsTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(6);
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
LRESULT CWinExtHostsTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CWinExtHostsTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CWinExtHostsTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CWinExtHostsTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CWinExtHostsTable::CWinExtHostsTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CWinExtHostsTable::ParseTableEntry(CString& s, char& mask, int& select, WinExtHostInfo& info, int column) {
	switch (static_cast<TableColumn>(column)) 
	{
		case TableColumn::Flags:
		{
			s.Format(L"0x%X", info.Flags);
			break;
		}
		case TableColumn::ExHost:
		{
			s.Format(L"0x%p", info.ExHost);
			break;
		}
		
		case TableColumn::Id:
		{
			s.Format(L"%d", info.Id);
			break;
		}

		case TableColumn::Version:
		{
			s.Format(L"%d", info.Version);
			break;
		}

		case TableColumn::BindNotification:
		{
			s.Format(L"0x%p", info.BindNotification);
			break;
		}

		case TableColumn::BindNotificationContext:
		{
			s.Format(L"0x%p", info.BindNotificationContext);
			break;
		}

		case TableColumn::ExtensionTable:
		{
			s.Format(L"0x%p", info.ExtensionTable);
			break;
		}

		case TableColumn::FunctionCount:
		{
			s.Format(L"%d", info.FunctionCount);
			break;
		}

		case TableColumn::HostTable:
		{
			if (info.HostTable == nullptr) {
				s = L"None";
				break;
			}
			s.Format(L"0x%p ", info.HostTable);
			DWORD64 offset = 0;
			auto symbol = SymbolHelper::GetSymbolFromAddress((DWORD64)info.HostTable);
			if (symbol) {
				std::string name = symbol->GetSymbolInfo()->Name;
				s += Helpers::StringToWstring(name).c_str();
			}
			break;
		}
	}
	return s.GetLength();
}

bool CWinExtHostsTable::CompareItems(const WinExtHostInfo& s1, const WinExtHostInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:

			break;
		default:
			break;
	}
	return false;
}

LRESULT CWinExtHostsTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

void CWinExtHostsTable::Refresh() {
	m_Table.data.n = 0;
	m_Table.data.info.clear();

	PVOID pListHead = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ExpHostList");
	if (pListHead != nullptr) {
		ULONG count = 0;
		count = DriverHelper::GetWinExtHostsCount(pListHead);
		if (count > 0) {
			ULONG size = (count + 5) * sizeof(WinExtHostInfo);
			wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
			WinExtHostInfo* pInfo = (WinExtHostInfo*)buffer.get();
			bool success = DriverHelper::EnumWinExtHosts(pListHead, pInfo, size);
			if (pInfo != nullptr && success) {
				for (ULONG i = 0; i < count; i++) {
					m_Table.data.info.push_back(pInfo[i]);
				}
			}
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

LRESULT CWinExtHostsTable::OnExtTable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	CExtensionTableDlg dlg(info);
	dlg.DoModal(m_hWnd);

	return TRUE;
}