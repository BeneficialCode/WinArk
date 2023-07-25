#pragma once
#include "ProcessThreadTable.h"

class CProcessThreadTable;
class CThreadDlg :public CDialogImpl<CThreadDlg> {
public:
	enum { IDD = IDD_BACKGROUND };

	BEGIN_MSG_MAP_EX(CThreadDlg)
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
	CProcessThreadTable* m_ProcThreadTable;
};