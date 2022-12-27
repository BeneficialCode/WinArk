#include "stdafx.h"
#include "ScyllaDlg.h"

BOOL CScyllaDlg::PreTranslateMessage(MSG* pMsg) {
	return FALSE;
}

LRESULT CScyllaDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CScyllaDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}