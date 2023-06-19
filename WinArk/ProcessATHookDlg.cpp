#include "stdafx.h"
#include "ProcessATHookDlg.h"

LRESULT CEATHookDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}

LRESULT CEATHookDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	BarDesc bars[] = {
		{20,"Hook Object",0},
		{10,"Hook Type",0},
		{52,"Hook Address",0},
		{52,"Target Address",0},
		{260,"Target Path",0},
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
	bool x64 = m_px.GetBitness() == 64 ? true : false;
	m_ProcEATHookTable = new CProcessATHookTable(info, table, pid, x64);
	WCHAR proc[25];
	_itow_s(pid, proc, 10);
	std::wstring title = L"Address Table Hook Process: pid = ";
	title += proc;
	SetWindowText(title.c_str());
	RECT rect;
	GetClientRect(&rect);
	m_ProcEATHookTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcEATHookTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	m_ProcEATHookTable->ShowWindow(SW_SHOW);
	return TRUE;
}

LRESULT CEATHookDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rect;
	GetClientRect(&rect);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcEATHookTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

void CEATHookDlg::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 150;
}