#pragma once
#include "resource.h"
#include "ProcessATHookTable.h"

class CEATHookDlg :public CDialogImpl<CEATHookDlg> {
public:
	enum { IDD = IDD_BACKGROUND };

	CEATHookDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px) :m_pm(pm), m_px(px) {}

	BEGIN_MSG_MAP_EX(CEATHookDlg)
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
	CProcessATHookTable* m_ProcEATHookTable;
	ProcessInfoEx& m_px;
	const WinSys::ProcessManager& m_pm;
};