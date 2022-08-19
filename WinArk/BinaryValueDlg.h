#pragma once

#include "HexControl.h"
#include "MemoryBuffer.h"
#include "RegistryKey.h"
#include "Interfaces.h"


class CBinaryValueDlg :
	public CDialogImpl<CBinaryValueDlg>,
	public CWinDataExchange<CBinaryValueDlg>,
	public CAutoUpdateUI<CBinaryValueDlg> {
public:
	enum { IDD = IDD_BINVALUE };

	enum { ID_DATA_BYTE = 500, ID_LINE = 520 };

	CBinaryValueDlg(RegistryKey& key, PCWSTR name, bool readOnly, IRegView* frame);

	const std::vector<BYTE>& GetValue() const;
	bool IsModified() const;

	BEGIN_MSG_MAP(CBinaryValueDlg)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		NOTIFY_CODE_HANDLER(HCN_SIZECHANGED, OnHexBufferSizeChanged)
		CHAIN_MSG_MAP(CAutoUpdateUI<CBinaryValueDlg>)
		REFLECT_NOTIFICATIONS_EX()
	END_MSG_MAP()

private:

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnHexBufferSizeChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void UpdateBufferSize();

private:

	RegistryKey& m_Key;
	CString m_Name;
	CHexControl m_Hex;
	MemoryBuffer m_Buffer;
	std::vector<BYTE> m_Value;
	DWORD m_Type{ 0 };
	IRegView* m_pFrame;
	bool m_ReadOnly;
	bool m_Modified{ false };
};