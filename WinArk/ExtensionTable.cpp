#include "stdafx.h"
#include "ExtensionTable.h"
#include "SymbolHelper.h"
#include "FileVersionInfoHelper.h"

LRESULT CExtensionTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CExtensionTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CExtensionTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CExtensionTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CExtensionTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CExtensionTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CExtensionTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CExtensionTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CExtensionTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CExtensionTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	/*CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(0);
	POINT pt;
	::GetCursorPos(&pt);
	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}*/

	return 0;
}

LRESULT CExtensionTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CExtensionTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CExtensionTable::CExtensionTable(BarInfo& bars, TableInfo& table, WinExtHostInfo& info)
	:CTable(bars, table),_info(info){
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CExtensionTable::ParseTableEntry(CString& s, char& mask, int& select, ExtTableInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
		case Column::Address:
			s.Format(L"0x%p", info.Routine);
			break;

		case Column::Company:
			s = info.Company.c_str();
			break;

		case Column::Module:
			s = info.Module.c_str();
			break;
	}
	return s.GetLength();
}

bool CExtensionTable::CompareItems(const ExtTableInfo& s1, const ExtTableInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:

			break;
		default:
			break;
	}
	return false;
}



void CExtensionTable::Refresh() {
	m_Table.data.n = 0;
	m_Table.data.info.clear();
	
	ExtHostData data;
	data.ExpFindHost = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ExpFindHost");
	data.Id = _info.Id;
	data.Version = _info.Version;
	data.Count = _info.FunctionCount;
	ULONG count = _info.FunctionCount;
	ULONG size = _info.FunctionCount * sizeof(void*);
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
	PVOID* pInfo = (PVOID*)buffer.get();
	bool success = DriverHelper::EnumExtTable(&data, pInfo, size);
	if (success) {
		ExtTableInfo info;
		for (ULONG i = 0; i < count; i++) {
			info.Routine = pInfo[i];
			std::string path = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
			info.Module = Helpers::StringToWstring(path);
			info.Company = FileVersionInfoHelpers::GetCompanyName(info.Module);
			m_Table.data.info.push_back(info);
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

