#pragma once

class CStringValueDlg :
	public CDialogImpl<CStringValueDlg>,
	public CWinDataExchange<CStringValueDlg> {
public:
	enum { IDD = IDD_STRINGVALUE };

	CStringValueDlg(bool canModify) : m_CanModify(canModify) {}

	void SetName(const CString& name, bool isReadOnly);
	const CString& GetName() const {
		return m_Name;
	}

	void SetValue(const CString& value) {
		m_Value = value;
	}
	void SetType(int type) {
		m_RegType = type;
	}

	int GetType() const {
		return m_RegType;
	}

	const CString& GetValue() const {
		return m_Value;
	}

	BEGIN_MSG_MAP(CStringValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		//		COMMAND_ID_HANDLER(IDC_COLOR, OnSelectColor)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChanged)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CStringValueDlg)
		DDX_TEXT_LEN(IDC_NAME, m_Name, 64)
		DDX_TEXT(IDC_NAME, m_Name)
		DDX_TEXT_LEN(IDC_VALUE, m_Value, 1024)
		DDX_TEXT(IDC_VALUE, m_Value)
		DDX_RADIO(IDC_STRING, m_RegType)
	END_DDX_MAP()

	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CString m_Name, m_Value;
	int m_RegType;
	bool m_CanModify, m_ReadOnlyName;
};