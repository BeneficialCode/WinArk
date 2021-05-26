#pragma once
#include "ProcessModuleTable.h"

class CProcessMemoryTable;
class CMemoryDlg :public CDialogImpl<CMemoryDlg> {
public:
	enum {
		IDD = IDD_MEMORY
	};

	BEGIN_MSG_MAP_EX(CMemoryDlg)
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
	CProcessMemoryTable* m_ProcMemoryTable;
};