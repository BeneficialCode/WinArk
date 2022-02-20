#pragma once
#include "resource.h"

class CSystemConfigView :
	public CDialogImpl<CSystemConfigView> {
public:
	enum { IDD = IDD_CONFIG };

	BEGIN_MSG_MAP(CSystemConfigView)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY,OnDestroy)
		COMMAND_ID_HANDLER(IDC_SET_CALLBACK,OnSetCallback)
		COMMAND_ID_HANDLER(IDC_REMOVE_CALLBACK,OnRemoveCallback)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


private:
	CButton m_CheckImageLoad;
};