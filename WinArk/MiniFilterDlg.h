#pragma once
#include "MiniFilterOperationTable.h"


class COperationTable;
class CMiniFilterDlg :public CDialogImpl<CMiniFilterDlg> {
public:
	enum { IDD = IDD_BACKGROUND };

	CMiniFilterDlg(std::wstring filterName) :m_Name(filterName) {}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

private:
	COperationTable* m_OperationTable;
	std::wstring m_Name;

public:

	BEGIN_MSG_MAP_EX(CMiniFilterDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()
};