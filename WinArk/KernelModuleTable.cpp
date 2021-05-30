#include "stdafx.h"
#include "KernelModuleTracker.h"
#include "KernelModuleTable.h"


LRESULT CKernelModuleTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CKernelModuleTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CKernelModuleTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelModuleTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CKernelModuleTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}

void CKernelModuleTable::DoRefresh() {
	auto count = m_Tracker.EnumModules();
	m_Table.data.info = m_Tracker.GetModules();
	m_Table.data.n = m_Table.data.info.size();
}

CKernelModuleTable::CKernelModuleTable(BarInfo& bars, TableInfo& table)
	: CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	DoRefresh();
}

int CKernelModuleTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::KernelModuleInfo>& info, int column) {
	switch (column) {
		case 0: 
			s = info->Name.c_str();
			break;
		case 1:
			s.Format(L"0x%p", info->ImageBase);
			break;
		case 2:
			s.Format(L"0x%X", info->ImageSize);
			break;
		case 3:
			s.Format(L"%u", info->LoadOrderIndex);
			break;
		case 4:
			s = info->FullPath.c_str();
			break;
		default:
			break;
	}
	return s.GetLength();
}

bool CKernelModuleTable::CompareItems(const std::shared_ptr<WinSys::KernelModuleInfo>& p1, const std::shared_ptr<WinSys::KernelModuleInfo>& p2, int col, bool asc) {
	switch (static_cast<KernemModuleColumn>(col)) {
	case KernemModuleColumn::Name: return SortHelper::SortStrings(p1->Name, p2->Name, asc);
	case KernemModuleColumn::ImageBase:return SortHelper::SortNumbers(p1->ImageBase, p2->ImageBase, asc);
	case KernemModuleColumn::ImageSize:return SortHelper::SortNumbers(p1->ImageSize, p2->ImageSize, asc);
	case KernemModuleColumn::LoadOrderIndex: return SortHelper::SortNumbers(p1->LoadOrderIndex, p2->LoadOrderIndex, asc);
	case KernemModuleColumn::FullPath: return SortHelper::SortStrings(p1->FullPath, p2->FullPath, asc);
	}
	return false;
}