#pragma once
#include "ProcessInlineHookTable.h"


class CInlineHookDlg :public CDialogImpl<CInlineHookDlg> {
public:
	enum { IDD = IDD_BACKGROUND};

	CInlineHookDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px):m_pm(pm),m_px(px){}

	BEGIN_MSG_MAP_EX(CInlineHookDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE,OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

private:
	CProcessInlineHookTable* m_ProcInlineHookTable{ nullptr };
	ProcessInfoEx& m_px;
	const WinSys::ProcessManager& m_pm;
};