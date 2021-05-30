#include "stdafx.h"
#include "WindowsView.h"
#include "resource.h"
#include "ProcessHelper.h"
#include "WindowHelper.h"
#include "SortHelper.h"

BOOL CWindowsView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

CString CWindowsView::GetColumnText(HWND, int row, int col) const {
	const auto h = m_Items[row];
	if (!::IsWindow(h)) {
		return L"";
	}

	CWindow win(h);

	CString text;


}
