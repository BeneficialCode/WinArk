#pragma once

class CIntValueDlg :
	public CDialogImpl<CIntValueDlg>,
	public CWinDataExchange<CIntValueDlg> {
public:
	CIntValueDlg(bool canModify) : m_CanModify(canModify) {}

	enum { IDD = IDD_INTVALUE };

	void SetValue(ULONGLONG value);
	void SetName(const CString& name, bool readonly);
	const CString& GetName() const {
		return m_Name;
	}

	ULONGLONG GetRealValue() const;
	int GetRadix(int n) const;

	BEGIN_MSG_MAP(CIntValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_COLOR, OnSelectColor)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChanged)
		COMMAND_RANGE_CODE_HANDLER(IDC_HEX, IDC_BIN, BN_CLICKED, OnRadioButtonClicked)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CIntValueDlg)
		DDX_RADIO(IDC_HEX, m_HexOrDec)
		DDX_TEXT_LEN(IDC_NAME, m_Name, 64)
		DDX_TEXT(IDC_NAME, m_Name)
		DDX_TEXT_LEN(IDC_VALUE, m_Value, 32)
		DDX_TEXT(IDC_VALUE, m_Value)
	END_DDX_MAP()

private:
	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRadioButtonClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectColor(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	CString m_Value, m_Name;
	static int m_HexOrDec;
	bool m_ReadOnlyName{ false }, m_Initialized{ false };
	bool m_CanModify;
};