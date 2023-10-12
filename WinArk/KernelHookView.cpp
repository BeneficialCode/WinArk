#include "stdafx.h"
#include "resource.h"
#include "KernelHookView.h"

LRESULT CKernelHookView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	CRect r(0, 0, 400, 25);
	CTabCtrl tabCtrl;
	auto hTabCtrl = tabCtrl.Create(m_hWnd, &r, nullptr, WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
		| TCS_HOTTRACK | TCS_SINGLELINE | TCS_RIGHTJUSTIFY | TCS_TABS,
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY,TabId);
	m_TabCtrl.SubclassWindow(hTabCtrl);
	
	//m_TabCtrl.SetFont()
	HFONT hFont = g_hAppFont;
	m_TabCtrl.SetFont(hFont, true);


	struct {
		PCWSTR Name;
	}columns[] = {
		L"SSDT",
		L"Shadow SSDT",
		L"Notify Routine",
		L"MiniFilter",
		L"WFP Filter",
		L"Inline Hook",
		L"Object Callback"
	};

	int i = 0;
	for (auto& col : columns) {
		m_TabCtrl.InsertItem(i++, col.Name);
	}

	InitSSDTHookTable();
	InitShadowSSDTHookTable();
	InitKernelNotifyTable();
	InitMiniFilterTable();
	InitWFPFilterTable();
	InitInlineHookTable();
	InitObCallbackTable();

	return 0;
}

LRESULT CKernelHookView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	GetClientRect(&rc);
	int width = rc.Width();
	int clientHeight = rc.Height();
	m_TabCtrl.GetClientRect(&rc);
	int height = rc.Height();
	::MoveWindow(m_TabCtrl.m_hWnd, rc.left, rc.top, width, height, true);

	int iX = rc.left;
	int iY = rc.top + height;
	clientHeight -= height;
	
	::MoveWindow(m_hwndArray[_index], iX, iY, width, clientHeight, true);

	bHandled = false;
	return 0;
}

void CKernelHookView::InitSSDTHookTable() {
	BarDesc bars[] = {
		{12,"Service Number",0},
		{55,"Service Name",0},
		{20,"Original Address",0},
		{10,"Is Hook",0},
		{20,"Current Address",0},
		{260,"Driver Path",0}
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

	m_SSDTHookTable = new CSSDTHookTable(info, table);
	RECT rect;
	::GetClientRect(m_TabCtrl.m_hWnd, &rect);
	int height = rect.bottom - rect.top;
	GetClientRect(&rect);
	rect.top += height;
	rect.bottom -= height;
	m_SSDTHookTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::SSDT)] = m_SSDTHookTable->m_hWnd;
	m_SSDTHookTable->ShowWindow(SW_SHOW);
}

void CKernelHookView::InitShadowSSDTHookTable() {
	BarDesc bars[] = {
		{15,"Service Number",0},
		{55,"Service Name",0},
		{20,"Original Address",0},
		{10,"Is Hook",0},
		{20,"Current Address",0},
		{260,"Driver Path",0}
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

	m_ShadowSSDTHookTable = new CShadowSSDTHookTable(info, table);
	RECT rect;
	::GetClientRect(m_TabCtrl.m_hWnd, &rect);
	int height = rect.bottom - rect.top;
	GetClientRect(&rect);
	rect.top += height;
	rect.bottom -= height;
	m_ShadowSSDTHookTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::ShadowSSDT)] = m_ShadowSSDTHookTable->m_hWnd;
	m_ShadowSSDTHookTable->ShowWindow(SW_HIDE);
}

void CKernelHookView::InitMiniFilterTable() {
	BarDesc bars[] = {
		{15,"Filter Name",0},
		{15,"Instance count",0},
		{35,"Altitude",0},
		{210,"Frame ID",0},
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

	m_MiniFilterTable = new CMiniFilterTable(info, table);
	RECT rect;
	::GetClientRect(m_TabCtrl.m_hWnd, &rect);
	int height = rect.bottom - rect.top;
	GetClientRect(&rect);
	rect.top += height;
	rect.bottom -= height;
	m_MiniFilterTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::MiniFilter)] = m_MiniFilterTable->m_hWnd;
	m_MiniFilterTable->ShowWindow(SW_HIDE);
}

void CKernelHookView::InitWFPFilterTable() {
	BarDesc bars[] = {
		{10,"Filter Id",0},
		{20,"Mode",0},
		{45,"Flags",0},
		{20,"Action Type",0},
		{30,"Layer Name",0},
		{30,"Name",0},
		{210,"Description",0},
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

	m_WFPFilterTable = new CWFPFilterTable(info, table);
	RECT rect;
	::GetClientRect(m_TabCtrl.m_hWnd, &rect);
	int height = rect.bottom - rect.top;
	GetClientRect(&rect);
	rect.top += height;
	rect.bottom -= height;
	m_WFPFilterTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::WFPFilter)] = m_WFPFilterTable->m_hWnd;
	m_WFPFilterTable->ShowWindow(SW_HIDE);
}

LRESULT CKernelHookView::OnTcnSelChange(int, LPNMHDR hdr, BOOL&) {
	int index = 0;
	
	index = m_TabCtrl.GetCurSel();
	for (auto hwnd : m_hwndArray) {
		if (::IsWindow(hwnd)) {
			::ShowWindow(hwnd, SW_HIDE);
		}
	}
	

	switch (static_cast<TabColumn>(index)) {
		case TabColumn::SSDT:
			m_SSDTHookTable->ShowWindow(SW_SHOW);
			m_SSDTHookTable->SetFocus();
			break;
		case TabColumn::ShadowSSDT:
			m_ShadowSSDTHookTable->ShowWindow(SW_SHOW);
			m_ShadowSSDTHookTable->SetFocus();
			break;
		case TabColumn::NotifyRoutine:
			m_KernelNotifyTable->ShowWindow(SW_SHOW);
			m_KernelNotifyTable->SetFocus();
			break;
		case TabColumn::MiniFilter:
			m_MiniFilterTable->ShowWindow(SW_SHOW);
			m_MiniFilterTable->SetFocus();
			break;
		case TabColumn::WFPFilter:
			m_WFPFilterTable->ShowWindow(SW_SHOW);
			m_WFPFilterTable->SetFocus();
			break;
		case TabColumn::InlineHook:
			m_InlineHookTable->ShowWindow(SW_SHOW);
			m_InlineHookTable->SetFocus();
			break;
		case TabColumn::ObjectCallback:
			m_ObjectCallbackTable->ShowWindow(SW_SHOW);
			m_ObjectCallbackTable->SetFocus();
			break;
	}
	_index = index;
	::PostMessage(m_hWnd, WM_SIZE, 0, 0);
	return 0;
}

void CKernelHookView::InitKernelNotifyTable() {
	BarDesc bars[] = {
		{20,"Callout Address",0},
		{25,"Callout Type",0},
		{25,"Company Name",0},
		{115,"Driver Path",0},
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

	m_KernelNotifyTable = new CKernelNotifyTable(info, table);
	RECT rect;
	::GetClientRect(m_TabCtrl.m_hWnd, &rect);
	int height = rect.bottom - rect.top;
	GetClientRect(&rect);
	rect.top += height;
	rect.bottom -= height;
	m_KernelNotifyTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::NotifyRoutine)] = m_KernelNotifyTable->m_hWnd;
	m_KernelNotifyTable->ShowWindow(SW_HIDE);
}

void CKernelHookView::InitInlineHookTable() {
	BarDesc bars[] = {
		{20,"Hook Object",0},
		{14,"Hook Type",0},
		{62,"Hook Address",0},
		{22,"Target Address",0},
		{260,"Target Module",0},
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


	m_InlineHookTable = new CKernelInlineHookTable(info, table);
	
	RECT rect;
	GetClientRect(&rect);
	m_InlineHookTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	m_hwndArray[static_cast<int>(TabColumn::InlineHook)] = m_InlineHookTable->m_hWnd;
	m_InlineHookTable->ShowWindow(SW_HIDE);
}

void CKernelHookView::InitObCallbackTable() {
	BarDesc bars[] = {
		{22,"Callback Entry",0},
		{22,"Registration Handle",0},
		{18,"Object Type",0},
		{10,"Enabled",0},
		{22,"PreOperation",0},
		{22,"PostOperation",0},
		{36,"Operations",0},
		{25,"Company Name",0},
		{260,"Module",0},
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


	m_ObjectCallbackTable = new CObjectCallbackTable(info, table);

	RECT rect;
	GetClientRect(&rect);
	m_ObjectCallbackTable->Create(m_hWnd, rect, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	int iHorizontalUnit = LOWORD(GetDialogBaseUnits());
	int iVerticalUnit = HIWORD(GetDialogBaseUnits());
	int width, height;
	height = rect.bottom - rect.top - 3 * iHorizontalUnit;
	width = rect.right - rect.left - 2 * iHorizontalUnit;
	m_hwndArray[static_cast<int>(TabColumn::ObjectCallback)] = m_ObjectCallbackTable->m_hWnd;
	m_ObjectCallbackTable->ShowWindow(SW_HIDE);
}