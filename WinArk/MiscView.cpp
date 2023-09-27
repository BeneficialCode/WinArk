#include "stdafx.h"
#include "resource.h"
#include "MiscView.h"
#include "ThemeSystem.h"

LRESULT CMiscView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CRect r(0, 0, 400, 25);
	CTabCtrl tabCtrl;
	auto hTabCtrl = tabCtrl.Create(m_hWnd, &r, nullptr, WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
		| TCS_HOTTRACK | TCS_SINGLELINE | TCS_RIGHTJUSTIFY | TCS_TABS,
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY, TabId);
	m_TabCtrl.SubclassWindow(hTabCtrl);

	HFONT hFont = g_hAppFont;
	m_TabCtrl.SetFont(hFont, true);

	struct {
		PCWSTR Name;
	}columns[] = {
		L"Logon Sessions",
		L"Bypass Detect",
		L"System Information",
		L"Task Scheduler",
	};

	int i = 0;
	for (auto& col : columns) {
		m_TabCtrl.InsertItem(i++, col.Name);
	}
	
	InitLogonSessionsView();
	InitBypassDectectView();
	InitSysInfoView();
	InitTaskSchedView();

	return 0;
}

LRESULT CMiscView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
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

LRESULT CMiscView::OnTcnSelChange(int, LPNMHDR hdr, BOOL&) {
	int index = 0;

	index = m_TabCtrl.GetCurSel();
	for (auto hwnd : m_hwndArray) {
		if (::IsWindow(hwnd)) {
			::ShowWindow(hwnd, SW_HIDE);
		}
	}

	switch (static_cast<TabColumn>(index)) {
		case TabColumn::LogonSessions:
			m_pLogonSessionView->ShowWindow(SW_SHOW);
			break;

		case TabColumn::BypassDetect:
			m_BypassView.ShowWindow(SW_SHOW);
			break;

		case TabColumn::SystemInformation:
			m_SysInfoView->ShowWindow(SW_SHOW);
			break;

		case TabColumn::TaskScheduler:
			m_TaskSchedView.ShowWindow(SW_SHOW);
			break;
	}
	_index = index;
	::PostMessage(m_hWnd, WM_SIZE, 0, 0);
	return 0;
}

void CMiscView::InitBypassDectectView() {
	HWND hWnd = m_BypassView.Create(m_hWnd, rcDefault);
	m_hwndArray[static_cast<int>(TabColumn::BypassDetect)] = m_BypassView.m_hWnd;
}

void CMiscView::InitLogonSessionsView() {
	m_pLogonSessionView = new CLogonSessionsView(m_pFrame);
	m_pLogonSessionView->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_hwndArray[static_cast<int>(TabColumn::LogonSessions)] = m_pLogonSessionView->m_hWnd;
}

void CMiscView::InitSysInfoView() {
	m_SysInfoView = new CSysInfoView(m_pFrame);
	m_SysInfoView->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::SystemInformation)] = m_SysInfoView->m_hWnd;
}

void CMiscView::InitTaskSchedView() {
	m_TaskSchedView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
		| WS_CLIPCHILDREN);
	m_TaskSchedView.m_MainSplitter.ShowWindow(SW_SHOW);
	m_hwndArray[static_cast<int>(TabColumn::TaskScheduler)] = m_TaskSchedView.m_hWnd;
}