#pragma once
#include "RegistryKey.h"

class CStringValueDlg :
	public CDialogImpl<CStringValueDlg>,
	public CWinDataExchange<CStringValueDlg> {
public:
	enum { IDD = IDD_STRINGVALUE };

	CStringValueDlg(RegistryKey& key, PCWSTR name, bool readOnly);

	const CString& GetValue() const;

	DWORD GetType() const;
	bool IsModified() const;

	BEGIN_MSG_MAP(CStringValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		REFLECT_NOTIFICATIONS_EX()
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	RegistryKey& m_Key;
	CString m_Name;
	CString m_Value;
	DWORD m_Type{ 0 };
	bool m_ReadOnly;
	bool m_Modified{ false };
};