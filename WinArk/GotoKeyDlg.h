#pragma once
#include "VirtualListView.h"

class CGotoKeyDlg :
	public CDialogImpl<CGotoKeyDlg>,
	public CDynamicDialogLayout<CGotoKeyDlg>,
	public CVirtualListView<CGotoKeyDlg>{
public:
	enum {IDD= IDD_GOTOKEY};

	const CString& GetKey() const;
	void SetKey(const CString& key);

	CString GetColumnText(HWND, int row, int col) const;
	void DoSort(const SortInfo* si);

	BOOL OnDoubleClickList(HWND, int row, int, const POINT&);

	BEGIN_MSG_MAP(CGotoKeyDlg)
		NOTIFY_HANDLER(IDC_KEY_LIST,LVN_ITEMCHANGED,OnKeyItemChanged)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		CHAIN_MSG_MAP(CVirtualListView<CGotoKeyDlg>)
		CHAIN_MSG_MAP(CDynamicDialogLayout<CGotoKeyDlg>)
		REFLECT_NOTIFICATIONS_EX()
	END_MSG_MAP()

private:
	struct ListItem {
		CString Name;
		CString Path;
	};

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnKeyItemChanged(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/);

	CListViewCtrl m_List;
	CString m_Key;
	std::vector<ListItem> m_Items;
};