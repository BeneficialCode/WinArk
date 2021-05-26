#pragma once
#include "ProcessModuleTable.h"

class CProcessModuleTable;
class CModuleDlg :public CDialogImpl<CModuleDlg> {
public:
	enum {
		IDD = IDD_MODULES
	};

	BEGIN_MSG_MAP_EX(CModuleDlg)
		MESSAGE_HANDLER(WM_INITDIALOG,OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE,OnSize)
		MESSAGE_HANDLER(WM_CLOSE,OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

private:
	CProcessModuleTable* m_ProcModuleTable;
};