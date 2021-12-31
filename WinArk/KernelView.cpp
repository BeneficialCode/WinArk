#include "stdafx.h"
#include "resource.h"
#include "KernelView.h"

LRESULT CKernelView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	CRect r(0, 0, 400, 25);
	CTabCtrl tabCtrl;
	auto hTabCtrl = tabCtrl.Create(m_hWnd, &r, nullptr, WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
		| TCS_HOTTRACK | TCS_SINGLELINE | TCS_RIGHTJUSTIFY | TCS_TABS,
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY, TabId);
	m_TabCtrl.SubclassWindow(hTabCtrl);

	//m_TabCtrl.SetFont()
	HFONT hFont = (HFONT)::GetStockObject(SYSTEM_FIXED_FONT);
	m_TabCtrl.SetFont(hFont, true);
	::DeleteObject(hFont);

	struct {
		PCWSTR Name;
	}columns[] = {
		L"驱动加载记录表",
	};

	int i = 0;
	for (auto& col : columns) {
		m_TabCtrl.InsertItem(i++, col.Name);
	}

	return 0;
}

LRESULT CKernelView::OnSize(UINT, WPARAM, LPARAM, BOOL& bHandled) {
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

LRESULT CKernelView::OnTcnSelChange(int, LPNMHDR hdr, BOOL&) {
	int index = 0;

	index = m_TabCtrl.GetCurSel();


	switch (static_cast<TabColumn>(index)) {
		case TabColumn::PiDDBCacheTable:
		{

			break;
		}
	}
	_index = index;
	::PostMessage(m_hWnd, WM_SIZE, 0, 0);
	return 0;
}