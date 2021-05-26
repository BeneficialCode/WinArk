#include "stdafx.h"
#include "resource.h"
#include "IntValueDlg.h"
#include "FormatHelper.h"


int CIntValueDlg::m_HexOrDec = 0;

void CIntValueDlg::SetValue(ULONGLONG value) {
	switch (m_HexOrDec) {
	case 0:
		m_Value.Format(L"%llX", value);
		break;

	case 1:
		m_Value.Format(L"%llu", value);
		break;

	case 2:
		m_Value = FormatHelper::ToBinary(value);
		break;

	default:
		ATLASSERT(false);
	}
}

void CIntValueDlg::SetName(const CString& name, bool readonly) {
	m_Name = name;
	m_ReadOnlyName = readonly;
}

ULONGLONG CIntValueDlg::GetRealValue() const {
	return _wcstoui64(m_Value, nullptr, GetRadix(m_HexOrDec));
}

int CIntValueDlg::GetRadix(int n) const {
	static const int radix[] = { 16, 10, 2 };
	ATLASSERT(n >= 0 && n <= 2);
	return radix[n];
}

LRESULT CIntValueDlg::OnTextChanged(WORD, WORD, HWND, BOOL&) {
	GetDlgItem(IDOK).EnableWindow(
		GetDlgItem(IDC_VALUE).GetWindowTextLength() > 0 &&
		GetDlgItem(IDC_NAME).GetWindowTextLength() > 0);
	return 0;
}

LRESULT CIntValueDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	DoDataExchange(FALSE);
	if (m_ReadOnlyName) {
		GetDlgItem(IDC_NAME).SendMessage(EM_SETREADONLY, TRUE);
		GetDlgItem(IDC_VALUE).SetFocus();
	}

	if (!m_CanModify)
		GetDlgItem(IDC_VALUE).SendMessage(EM_SETREADONLY, TRUE);

	GetDlgItem(IDC_VALUE).SendMessage(EM_SETSEL, 0, -1);

	CenterWindow(GetParent());
	m_Initialized = true;
	return FALSE;

}

LRESULT CIntValueDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDCANCEL) {
		EndDialog(IDCANCEL);
		return 0;
	}
	if (DoDataExchange(TRUE)) {
		EndDialog(IDOK);
	}
	return 0;
}

LRESULT CIntValueDlg::OnRadioButtonClicked(WORD, WORD wID, HWND, BOOL&) {
	auto value = GetRealValue();
	DoDataExchange(TRUE);
	SetValue(value);
	DoDataExchange(FALSE);
	return 0;
}

LRESULT CIntValueDlg::OnSelectColor(WORD, WORD wID, HWND, BOOL&) {
	CColorDialog dlg((COLORREF)GetRealValue(), CC_ANYCOLOR | CC_FULLOPEN, m_hWnd);
	if (dlg.DoModal() == IDOK && m_CanModify) {
		SetValue((DWORD)dlg.GetColor());
		DoDataExchange(FALSE);
	}
	return 0;
}