#pragma once
#include "ExtensionTable.h"
#include "DriverHelper.h"

class CExtensionTableDlg :public CDialogImpl<CExtensionTableDlg> {
public:
	enum {IDD = IDD_THREADS};

	CExtensionTableDlg(WinExtHostInfo& info):_info(info) {}

	BEGIN_MSG_MAP_EX(CExtensionTableDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

private:
	WinExtHostInfo& _info;
	CExtensionTable* m_ExtTable{ nullptr };
};