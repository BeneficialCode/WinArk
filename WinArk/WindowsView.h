#pragma once

#include "VirtualListView.h"
#include "TreeViewManager.h"

class CWindowsView :
	public CWindowImpl<CWindowsView, CListViewCtrl>,
	public CCustomDraw<CWindowsView>,
	public CVirtualListView<CWindowsView>,
	public CTreeViewManager<CWindowsView> {
public:
	DECLARE_WND_CLASS(nullptr);

	BOOL PreTranslateMessage(MSG* pMsg);

	CString GetColumnText(HWND, int row, int col) const;
	int GetRowImage(HWND, int row) const;

	void OnActivate(bool activate);
	void DoSort(const SortInfo* si);
	bool IsSortable(int col) const;

	DWORD OnPrePaint(int, LPNMCUSTOMDRAW cd);
	DWORD OnItemPrePaint(int, LPNMCUSTOMDRAW cd);

	virtual void OnFinalMessage(HWND /*hWnd*/);

	enum { IDC_TREE = 123 };


private:
	enum class WindowListType {
		Parent = 1,
		AllChildren = 2,
		DirectChidren = 4,
	};

	enum class DataItemType {
		Handle, ClassName, Text, Style, ExtenedStyle, ProcessName,
		ProcessId, ThreadId, ParentWindow, FirstChildWindow, NextWindow, PrevWindow, OwnerWindow,
		WindowProc, UserData, ID, Rectangle,
		ClassAtom, ClassStyle, ClassExtra, WindowExtra,
	};
	struct DataItem {
		CString Property;
		DataItemType Type;
	};

	void UpdateUI();
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
	LRESULT OnToggleHiddenWindows (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggleEmptyTitleWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggleChildWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowFlash(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWindowBringToFront(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

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