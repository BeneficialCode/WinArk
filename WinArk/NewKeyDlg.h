#pragma once
// CNewKeyDlg dialog

class CNewKeyDlg :
	public CDialogImpl<CNewKeyDlg>,
	public CWinDataExchange<CNewKeyDlg> {
public:
	enum { IDD = IDD_NEWKEY };

	BEGIN_MSG_MAP(CNewKeyDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChanged)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CNewKeyDlg)
		DDX_TEXT(IDC_NAME, m_Name)
	END_DDX_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	const CString& GetKeyName() const {
		return m_Name;
	}

private:
	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CString m_Name;
};