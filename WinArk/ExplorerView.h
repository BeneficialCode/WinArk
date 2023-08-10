#pragma once
#include "resource.h"
#include "ShellMgr.h"

class CExplorerView : 
	public CWindowImpl<CExplorerView> {
public:

	DECLARE_WND_CLASS(L"WtlExplorerWndClass")
	const int ID_DELETE_FILE = 32444;
	BEGIN_MSG_MAP(CExplorerView)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		MESSAGE_HANDLER(WM_SIZE,OnSize)

		NOTIFY_CODE_HANDLER(NM_RCLICK,OnNMRClick)

		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTVSelChanged)
		NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnTVItemExpanding)
		NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnTVDeleteItem)

		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnLVGetDispInfo)
		NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnLVDeleteItem)
		COMMAND_ID_HANDLER(ID_DELETE_FILE,OnForceDeleteFile)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT OnNMRClick(int, LPNMHDR pnmh, BOOL&);


	void InitViews();
	HRESULT FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST lpifq, HTREEITEM hParent);
	BOOL FillListView(LPTVITEMDATA lptvid, LPSHELLFOLDER pShellFolder);

	LRESULT OnTVSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnTVItemExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnTVDeleteItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);



	LRESULT OnLVGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnLVDeleteItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT OnForceDeleteFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	void RefreshTreeView();

	void ForceDeleteDir(std::wstring dir);
	

public:
	CSplitterWindow m_WndSplitter;

private:

	WCHAR m_szListViewBuffer[MAX_PATH];
	CTreeViewCtrlEx m_WndTreeView;
	CListViewCtrl m_WndListView;

	CShellMgr m_ShellMgr;
	WCHAR m_Path[MAX_PATH] = { 0 };
};