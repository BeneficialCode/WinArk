#pragma once

class CExportDlg :
	public CDialogImpl<CExportDlg> {
public:
	enum {IDD = IDD_EXPORT};

	void SetKeyPath(PCWSTR path);

	const CString& GetSelectedKey() const;
	const CString& GetFileName() const;

	BEGIN_MSG_MAP(CExportDlg)
		COMMAND_HANDLER(IDC_PATH,EN_CHANGE,OnPathTextChanged)
		MESSAGE_HANDLER(WM_INITDIALOG,OnInitDialog)
		COMMAND_ID_HANDLER(IDOK,OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL,OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BROWSE,OnBrowse)
		REFLECT_NOTIFICATIONS_EX()
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnPathTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	CString m_Key;
	CString m_FileName;
};