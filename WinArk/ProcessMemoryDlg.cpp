#include "stdafx.h"
#include "resource.h"
#include "ProcessMemoryTable.h"
#include "ProcessMemoryDlg.h"

LRESULT CMemoryDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}

LRESULT CMemoryDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	BarDesc bars[] = {
		{15,"State",0},
		{20,"Address",0},
		{15,"Size",0},
		{20,"Type",0},
		{20,"Protection",0},
		{20,"Alloc Protection",0},
		{15,"Usage",0},
		{260,"Details",0}
	};

	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	DWORD pid = static_cast<DWORD>(lParam);
	m_ProcMemoryTable = new CProcessMemoryTable(info, table, pid);
	WCHAR proc[25];
	_itow(pid, proc, 10);
	std::wstring title = L"Process: pid = ";
	title += proc;
	SetWindowText(title.c_str());
	RECT rect;
	GetClientRect(&rect);
	m_ProcMemoryTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcMemoryTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	m_ProcMemoryTable->ShowWindow(SW_SHOW);
	return TRUE;
}

LRESULT CMemoryDlg::CMemoryDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rect;
	GetClientRect(&rect);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcMemoryTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

void CMemoryDlg::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 150;
}