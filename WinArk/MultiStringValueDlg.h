#pragma once
#include "RegistryKey.h"

class CMultiStringValueDlg :
	public CDialogImpl<CMultiStringValueDlg>,
	public CWinDataExchange<CMultiStringValueDlg> {
public:
	enum { IDD = IDD_MULTISTRING };

	CMultiStringValueDlg(RegistryKey& key, PCWSTR name, bool readOnly);

	const CString& GetValue() const;
	bool IsModified() const;

	BEGIN_MSG_MAP(CMultiStringValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	RegistryKey& m_Key;
	CString m_Value, m_Name;
	bool m_ReadOnly;
	bool m_Modified{ false };
};