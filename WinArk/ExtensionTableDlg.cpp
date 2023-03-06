#include "stdafx.h"
#include "resource.h"
#include "ExtensionTableDlg.h"

LRESULT CExtensionTableDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}

LRESULT CExtensionTableDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	BarDesc bars[] = {
		{22,"函数地址",0},
		{25,"文件厂商",0},
		{260,"目标地址所属模块",0},
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

	m_ExtTable = new CExtensionTable(info, table, _info);
	SetWindowText(L"Extension Table Info");
	RECT rect;
	GetClientRect(&rect);
	m_ExtTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ExtTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	m_ExtTable->ShowWindow(SW_SHOW);
	return TRUE;
}

LRESULT CExtensionTableDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	RECT rect;
	GetClientRect(&rect);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	::MoveWindow(m_ExtTable->m_hWnd, iHorizontalUnit, iVerticalUnit, width, height, false);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

void CExtensionTableDlg::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 150;
}