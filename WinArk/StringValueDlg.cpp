#include "stdafx.h"
#include "resource.h"
#include "StringValueDlg.h"
#include "RegHelpers.h"

CStringValueDlg::CStringValueDlg(RegistryKey& key, PCWSTR name, bool readOnly) :
	m_Key(key), m_Name(name), m_ReadOnly(readOnly) {
}

const CString& CStringValueDlg::GetValue() const {
	return m_Value;
}

DWORD CStringValueDlg::GetType() const {
	return m_Type;
}

bool CStringValueDlg::IsModified() const {
	return m_Modified;
}

LRESULT CStringValueDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	ULONG chars = 0;
	m_Key.QueryStringValue(m_Name, nullptr, &chars);
	if (chars) {
		auto buffer = std::make_unique<WCHAR[]>(chars);
		if (ERROR_SUCCESS != m_Key.QueryStringValue(m_Name, buffer.get(), &chars)) {
			EndDialog(IDRETRY);
			return 0;
		}
		SetDlgItemText(IDC_VALUE, m_Value = buffer.get());
	}

	m_Key.QueryValue(m_Name, &m_Type, nullptr, nullptr);
	ATLASSERT(m_Type == REG_SZ || m_Type == REG_EXPAND_SZ);
	CheckDlgButton(m_Type == REG_SZ ? IDC_STRING : IDC_EXPANDSZ, BST_CHECKED);

	if (m_ReadOnly) {
		((CEdit)GetDlgItem(IDC_VALUE)).SetReadOnly(TRUE);
		GetDlgItem(IDC_STRING).EnableWindow(FALSE);
		GetDlgItem(IDC_EXPANDSZ).EnableWindow(FALSE);
	}
	else {
		::SHAutoComplete(GetDlgItem(IDC_VALUE), SHACF_FILESYS_DIRS);
	}
	SetDlgItemText(IDC_NAME, m_Name.IsEmpty() ? RegHelpers::DefaultValueName : m_Name);

	return TRUE;
}

LRESULT CStringValueDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDOK) {
		CString value;
		GetDlgItemText(IDC_VALUE, value);
		auto type = IsDlgButtonChecked(IDC_STRING) == BST_CHECKED ? REG_SZ : REG_EXPAND_SZ;
		if (type != m_Type || value != m_Value) {
			m_Value = value;
			m_Type = type;
			// something changed
			m_Modified = true;
		}
	}
	EndDialog(wID);
	return 0;
}
