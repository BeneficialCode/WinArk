#include "stdafx.h"
#include "resource.h"
#include "NewKeyDlg.h"


LRESULT CNewKeyDlg::OnTextChanged(WORD, WORD, HWND, BOOL&) {
	DoDataExchange(TRUE);
	::EnableWindow(GetDlgItem(IDOK), !m_Name.IsEmpty());
	return 0;
}

LRESULT CNewKeyDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	CenterWindow(GetParent());

	return TRUE;
}

LRESULT CNewKeyDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	EndDialog(wID);
	return 0;
}