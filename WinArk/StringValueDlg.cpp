#include "stdafx.h"
#include "resource.h"
#include "StringValueDlg.h"

LRESULT CStringValueDlg::OnTextChanged(WORD, WORD, HWND, BOOL&) {
	GetDlgItem(IDOK).EnableWindow(
		GetDlgItem(IDC_VALUE).GetWindowTextLength() > 0 &&
		GetDlgItem(IDC_NAME).GetWindowTextLength() > 0);
	return 0;
}

void CStringValueDlg::SetName(const CString& name, bool readonly) {
	m_Name = name.IsEmpty() ? CString(L"(Default)") : name;
	m_ReadOnlyName = readonly;
}

LRESULT CStringValueDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {

	DoDataExchange(FALSE);
	if (!m_CanModify)
		GetDlgItem(IDC_VALUE).SendMessage(EM_SETREADONLY, TRUE);
	else {
		GetDlgItem(IDC_VALUE).SendMessage(EM_SETSEL, 0, -1);
		GetDlgItem(IDC_VALUE).SetFocus();
	}

	if (m_ReadOnlyName)
		GetDlgItem(IDC_NAME).SendMessage(EM_SETREADONLY, TRUE);


	return TRUE;
}

LRESULT CStringValueDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDCANCEL) {
		EndDialog(IDCANCEL);
	}
	else {
		DoDataExchange(TRUE);
		EndDialog(IDOK);
	}
	return 0;
}
