#pragma once
#include "resource.h"
#include "ShellMgr.h"

class CExplorerView : 
	public CWindowImpl<CExplorerView> {
public:

	DECLARE_WND_CLASS(L"WtlExplorerWndClass")

	BEGIN_MSG_MAP(CExplorerView)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		MESSAGE_HANDLER(WM_SIZE,OnSize)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	

	void InitViews();
	HRESULT FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST lpifq, HTREEITEM hParent);


public:
	CSplitterWindow m_WndSplitter;

private:

	
	CTreeViewCtrlEx m_WndTreeView;
	CListViewCtrl m_WndListView;

	CShellMgr m_ShellMgr;
};