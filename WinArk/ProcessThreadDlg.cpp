#include "stdafx.h"
#include "resource.h"
#include "ProcessThreadDlg.h"
#include "ProcessThreadTable.h"

LRESULT CThreadDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}

LRESULT CThreadDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	BarDesc bars[] = {
		{12,"Thread Status",0},
		{15,"Thread Id",0},
		{15,"Process Id",0},
		{20,"Process Name",0},
		{20,"CPUTime",0},
		{30,"CreateTime",0},
		{10,"Priority",0},
		{10,"BasePriority",0},
		{20,"Teb",0},
		{20,"WaitReason",0},
		{20,"StartAddress",0},
		{30,"Win32StartAddress",0},
		{20,"StackBase",0},
		{20,"StackLimit",0},
		{30,"ContextSwitch",0},
		{25,"KernelTime",0},
		{25,"UserTime",0},
		{10,"IoPriority",0},
		{5,"MemoryPriority",0},
		{30,"ComFlags",0},
		{30,"ComApartment",0},
		{25,"WaitTime",0},
		{260,"Module",0}
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
	m_ProcThreadTable = new CProcessThreadTable(info, table, pid);
	WCHAR proc[25];
	_itow_s(pid, proc, 10);
	std::wstring title = L"Process: pid = ";
	title += proc;
	SetWindowText(title.c_str());
	RECT rect;
	GetClientRect(&rect);
	m_ProcThreadTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcThreadTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	m_ProcThreadTable->ShowWindow(SW_SHOW);
	return TRUE;
}

LRESULT CThreadDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rect;
	GetClientRect(&rect);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcThreadTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

void CThreadDlg::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 150;
}