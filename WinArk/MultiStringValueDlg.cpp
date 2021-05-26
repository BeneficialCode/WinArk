#include "stdafx.h"
#include "resource.h"
#include "MultiStringValueDlg.h"

void CMultiStringValueDlg::SetName(const CString& name, bool isReadOnly) {
	m_Name = name;
	m_ReadOnlyName = isReadOnly;
}

LRESULT CMultiStringValueDlg::OnTextChanged(WORD, WORD, HWND, BOOL&) {
	GetDlgItem(IDOK).EnableWindow(
		GetDlgItem(IDC_VALUE).GetWindowTextLength() > 0 &&
		GetDlgItem(IDC_NAME).GetWindowTextLength() > 0);
	return 0;
}

LRESULT CMultiStringValueDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	

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

LRESULT CMultiStringValueDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDCANCEL) {
		EndDialog(IDCANCEL);
	}
	else {
		DoDataExchange(TRUE);
		EndDialog(IDOK);
	}
	return 0;
}
