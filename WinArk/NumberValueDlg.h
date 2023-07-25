#pragma once
#include "RegistryKey.h"

class CNumberValueDlg :
	public CDialogImpl<CNumberValueDlg>,
	public CWinDataExchange<CNumberValueDlg> {
public:
	enum { IDD = IDD_NUMBERVALUE };

	CNumberValueDlg(RegistryKey& key, PCWSTR name, DWORD type, bool readOnly);

	DWORD64 GetValue() const;
	DWORD GetType() const;
	bool IsModified() const;

	BEGIN_MSG_MAP(CIntValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()


private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


private:
	enum class ValueFormat {
		Decimal = 10, Hex = 16, Binary = 2, Octal = 8
	};
	void DisplayValue(DWORD64 value, bool checkRedio = true);
	DWORD64 ParseValue(const CString& text, bool& error);
	LRESULT OnClickBase(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClickColor(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	RegistryKey& m_Key;
	CString m_Name;
	DWORD64 m_Value;
	DWORD m_Type;
	bool m_ReadOnly;
	bool m_Modified{ false };
	inline static ValueFormat m_Format{ ValueFormat::Decimal };
};