#pragma once
#include "resource.h"
#include "SystemInformation.h"

class CSystemConfigDlg :
	public CDialogImpl<CSystemConfigDlg> {
public:
	enum { IDD = IDD_CONFIG };

	BEGIN_MSG_MAP(CSystemConfigDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY,OnDestroy)
		COMMAND_ID_HANDLER(IDC_SET_CALLBACK,OnSetCallback)
		COMMAND_ID_HANDLER(IDC_REMOVE_CALLBACK,OnRemoveCallback)
		COMMAND_ID_HANDLER(IDC_ENABLE_DBGSYS,OnEnableDbgSys)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEnableDbgSys(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


private:
	CButton m_CheckImageLoad;
	WinSys::BasicSystemInfo m_BasicSysInfo;
	bool m_enableDbgSys = false;
};