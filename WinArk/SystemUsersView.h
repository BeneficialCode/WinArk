#pragma once
#include "ViewBase.h"
#include "VirtualListView.h"
#include "resource.h"

class CSystemUsersView :
	public CViewBase<CSystemUsersView>,
	public CVirtualListView<CSystemUsersView> {
public:
	CSystemUsersView(IMainFrame* frame):CViewBase(frame){}

	BEGIN_MSG_MAP(CSystemUsersView)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		CHAIN_MSG_MAP(CVirtualListView<CSystemUsersView>)
		CHAIN_MSG_MAP(CViewBase<CSystemUsersView>)
	END_MSG_MAP()

	CString GetColumnText(HWND, int row, int col) const;
	void DoSort(const SortInfo* si);
	bool IsSortable(HWND, int col) const;
	bool IsUpdating() const;

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void Refresh();
	
	std::string GetReadableTime(DWORD time) const;

private:
	CListViewCtrl m_List;
	struct UserInfo {
		std::wstring UserName;
		std::wstring Comment;
		DWORD NumLogons;
		DWORD LastLogon;
		DWORD LastLogoff;
		DWORD UserId;
		std::wstring Sid;
	};
	std::vector<UserInfo> m_Users;
	std::vector<UserInfo> EnumSystemUsers();
};