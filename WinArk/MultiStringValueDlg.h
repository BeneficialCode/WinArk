#pragma once

class CMultiStringValueDlg :
	public CDialogImpl<CMultiStringValueDlg>,
	public CWinDataExchange<CMultiStringValueDlg> {
public:
	enum { IDD = IDD_MULTISTRING };

	CMultiStringValueDlg(bool canModify) : m_CanModify(canModify) {}

	void SetName(const CString& name, bool isReadOnly);
	const CString& GetName() const {
		return m_Name;
	}

	void SetValue(const CString& value) {
		m_Value = value;
	}

	const CString& GetValue() const {
		return m_Value;
	}

	BEGIN_MSG_MAP(CMultiStringValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChanged)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CMultiStringValueDlg)
		DDX_TEXT_LEN(IDC_NAME, m_Name, 64)
		DDX_TEXT(IDC_NAME, m_Name)
		DDX_TEXT(IDC_VALUE, m_Value)
	END_DDX_MAP()

private:
	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CString m_Value, m_Name;
	bool m_CanModify, m_ReadOnlyName;
};