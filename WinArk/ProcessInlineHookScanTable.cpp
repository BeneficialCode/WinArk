#include "stdafx.h"
#include "ProcessInlineHookScanTable.h"

CProcessInlineHookScanTable::CProcessInlineHookScanTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);

}

int CProcessInlineHookScanTable::ParseTableEntry(CString& s, char& mask, int& select, InlineHookInfo& info, int column){
	return 0;
}

bool CProcessInlineHookScanTable::CompareItems(const InlineHookInfo& s1, const InlineHookInfo& s2, int col, bool asc){
	return false;
}

LRESULT CProcessInlineHookScanTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&){
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookScanTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CProcessInlineHookScanTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookScanTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&){
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

void CProcessInlineHookScanTable::Refresh() {

}
