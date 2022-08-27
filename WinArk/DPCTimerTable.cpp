#include "stdafx.h"
#include "DPCTimerTable.h"
#include "SymbolHelper.h"
#include "DriverHelper.h"

LRESULT CDPCTimerTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Refresh();
	return 0;
}

LRESULT CDPCTimerTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CDPCTimerTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CDPCTimerTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDPCTimerTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDPCTimerTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDPCTimerTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDPCTimerTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDPCTimerTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CDPCTimerTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDPCTimerTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDPCTimerTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}


CDPCTimerTable::CDPCTimerTable(BarInfo& bars, TableInfo& table)
	: CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	
}

int CDPCTimerTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<DPCTimerInfo>& info, int column) {
	switch (column) {
		
	}
	return s.GetLength();
}




void CDPCTimerTable::Refresh() {
	ULONG memberSize = SymbolHelper::GetKernelSturctMemberSize("_KTIMER_TABLE", "TimerEntries");
	ULONG size = SymbolHelper::GetKernelStructSize("_KTIMER_TABLE_ENTRY");
	KernelTimerData data;
	if (size != 0) {
		ULONG entryCount = memberSize / size;
		data.entriesOffset = SymbolHelper::GetKernelStructMemberOffset("_KTIMER_TABLE", "TimerEntries");
		data.maxEntryCount = entryCount;
		data.tableOffset = SymbolHelper::GetKernelStructMemberOffset("_KPRCB", "TimerTable");
		data.pKiProcessorBlock = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiProcessorBlock");
#ifdef _WIN64
		data.pKiWaitAlways = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiWaitAlways");
		data.pKiWaitNever = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiWaitNever");
#endif // _WIN64

		DriverHelper::EnumKernelTimer(&data);
	}
	
	auto count = static_cast<int>(m_Table.data.info.size());
	m_Table.data.n = count;

	return;
}