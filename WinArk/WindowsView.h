#pragma once

#include "VirtualListView.h"
#include "TreeViewManager.h"


class CWindowsView :
	public CWindowImpl<CWindowsView, CListViewCtrl>,
	public CVirtualListView<CWindowsView>,
	public CTreeViewManager<CWindowsView> {
public:
	DECLARE_WND_CLASS(nullptr);

	CString GetColumnText(HWND, int row, int col) const;
	int GetRowImage(HWND, int row,int col) const;

	void OnActivate(bool activate);
	void DoSort(const SortInfo* si);
	bool IsSortable(HWND,int col) const;

	//DWORD OnPrePaint(int, LPNMCUSTOMDRAW cd);
	//DWORD OnItemPrePaint(int, LPNMCUSTOMDRAW cd);

	enum { IDC_WINDOW_TREE = 123 };

	// ATL::CMessageMap::ProcessWindowMessage
	BEGIN_MSG_MAP(CWindowsView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE,OnSize)
		NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnNodeExpanding)
		NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnNodeDeleted)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnNodeSelected)
		NOTIFY_HANDLER(IDC_WINDOW_TREE, NM_RCLICK, OnTreeNodeRightClick)
		COMMAND_ID_HANDLER(ID_WINDOW_REFRESH, OnRefresh)
		COMMAND_ID_HANDLER(ID_WINDOW_SHOW, OnWindowShow)
		COMMAND_ID_HANDLER(ID_WINDOW_HIDE, OnWindowHide)
		COMMAND_ID_HANDLER(ID_WINDOW_BRINGTOFRONT, OnWindowBringToFront)
		COMMAND_ID_HANDLER(ID_WINDOW_MINIMIZE, OnWindowMinimize)
		COMMAND_ID_HANDLER(ID_WINDOW_MAXIMIZE, OnWindowMaximize)
		COMMAND_ID_HANDLER(ID_STATE_FLASH, OnWindowFlash)
		COMMAND_ID_HANDLER(ID_WINDOW_RESTORE, OnWindowRestore)
		/*COMMAND_ID_HANDLER(ID_VIEW_HIDDENWINDOWS, OnToggleHiddenWindows)
		COMMAND_ID_HANDLER(ID_VIEW_EMPTYTITLEWINDOWS, OnToggleEmptyTitleWindows)
		COMMAND_ID_HANDLER(ID_VIEW_CHILDWINDOWS, OnToggleChildWindows)*/
		CHAIN_MSG_MAP(CVirtualListView<CWindowsView>)
		CHAIN_MSG_MAP(CTreeViewManager<CWindowsView>)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

private:
	enum class WindowListType {
		Parent = 1,
		AllChildren = 2,
		DirectChidren = 4,
	};

	enum class DataItemType {
		Handle, ClassName, Text, Style, ExtendedStyle, ProcessName,
		ProcessId, ThreadId, ParentWindow, FirstChildWindow, NextWindow, PrevWindow, OwnerWindow,
		WindowProc, UserData, ID, Rectangle,
		ClassAtom, ClassStyle, ClassExtra, WindowExtra,
	};

	struct DataItem {
		CString Property;
		DataItemType Type;
	};

	void Refresh();
	void InitTree();
	void AddChildWindows(HTREEITEM hParent);
	CTreeItem AddNode(HWND hWnd, HTREEITEM hParent);
	BOOL AddChildNode(HWND hWnd);
	void AddChildWindows(std::vector<HWND>& v, HWND hParent, bool directOnly = false);
	
	void UpdateList();
	CString GetDetails(const DataItem& item) const;
	static CString GetWindowClassAndTitle(HWND);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnNodeExpanding(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnNodeDeleted(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnNodeSelected(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeNodeRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnWindowShow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowHide(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowMinimize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowMaximize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowRestore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	//LRESULT OnToggleHiddenWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	//LRESULT OnToggleEmptyTitleWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	//LRESULT OnToggleChildWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowFlash(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowBringToFront(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	friend class CMainFrame;
	CSplitterWindow m_Splitter;
	CListViewCtrl m_List;
	CTreeViewCtrlEx m_Tree;
	inline static CImageList s_Images;
	CTreeItem m_hCurrentNode;
	CTreeItem m_DesktopNode;
	CTreeItem m_Selected;
	CWindow m_SelectedHwnd;
	std::unordered_map<HWND, HTREEITEM> m_WindowsMap;
	inline static std::unordered_map<HWND, int> s_IconMap;
	std::vector<HWND> m_Items;
	DWORD m_TotalWindows, m_TotalVisibleWindows, m_TopLevelWindows;

	bool m_ShowHiddenWindows : 1{false};
	bool m_ShowNoTitleWindows : 1{true};
	bool m_ShowChildWindows : 1{true};
	bool m_Deleting{ false };
};