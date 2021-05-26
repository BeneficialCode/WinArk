#include "stdafx.h"
#include "resource.h"
#include "ProcessModuleTable.h"
#include "ProcessModuleDlg.h"

LRESULT CModuleDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}

LRESULT CModuleDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	BarDesc bars[] = {
		{25,"模块名",0},
		{8,"类型",0},
		{15,"映像大小",0},
		{20,"基地址",0},
		{20,"镜像基址",0},
		{70,"属性",0},
		{260,"所在路径",0}
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
	m_ProcModuleTable = new CProcessModuleTable(info, table,pid,m_hWnd);
	WCHAR proc[25];
	_itow(pid, proc, 10);
	std::wstring title = L"Process: pid = ";
	title += proc;
	SetWindowText(title.c_str());
	RECT rect;
	GetClientRect(&rect);
	m_ProcModuleTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcModuleTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	m_ProcModuleTable->ShowWindow(SW_SHOW);
	return TRUE;
}

LRESULT CModuleDlg::CModuleDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rect;
	GetClientRect(&rect);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ProcModuleTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

void CModuleDlg::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 150;
}


