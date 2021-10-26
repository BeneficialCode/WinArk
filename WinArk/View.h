#pragma once

#include "resource.h"
#include "VirtualListView.h"
#include "IMainFrame.h"
#include "Registry.h"
#include "CommandManager.h"
#include "EnumStrings.h"
#include "DeleteKeyCommand.h"
#include "DeleteValueCommand.h"
#include "LocationManager.h"
#include "AppSettings.h"
#include "Theme.h"


enum class NodeType {
	None=0,
	Key=1,
	Predefined=2,
	RegistryRoot=0x10,
	StandardRoot=0x20,
	Hive=0x40,
	Link=0x80,
	Loaded=0x100,
	Machine=0x200,
	AccessDenied=0x400,
	RemoteRegistry=0x800,
};
DEFINE_ENUM_FLAG_OPERATORS(NodeType);

// 界面上
//		颜色修改技术 WM_CTLCOLORXXX
//		用户自绘技术 CustomDraw WM_CUSTOMDRAW
//		拥有者自绘技术 OwnerDraw WM_DRAWITEM
// 行为上
//		窗口子类化技术，控件子类化技术
class CRegistryManagerView :
	public CWindowImpl<CRegistryManagerView>,
	public CVirtualListView<CRegistryManagerView>,
	public CAutoUpdateUI<CRegistryManagerView>,
	public COwnerDraw<CRegistryManagerView>,
	public IMainFrame {
public:
	DECLARE_FRAME_WND_CLASS(L"RegExpWndClass", IDR_MAINFRAME)
	
	CRegistryManagerView():m_AddressBar(this,2){}

	const UINT WM_BUILD_TREE = WM_APP + 11;
	const UINT WM_FIND_UPDATE = WM_APP + 12;
	const UINT WM_RUN = WM_APP + 13;
	const UINT TreeId = 123;

	void SetStartKey(const CString& key);

	void RunOnUiThread(std::function<void()> f);
	
	// IMainFrame
	HWND GetHwnd() const override;
	AppSettings& GetSettings() override;
	void OnFindNext(PCWSTR path, PCWSTR name, PCWSTR data) override;
	void OnFindStart();
	void OnFindEnd(bool cancelled);
	bool GoToItem(PCWSTR path, PCWSTR name, PCWSTR data) override;
	// BOOL TrackPopupMenu(HMENU hMenu, DWORD flags, int x, int y, HWND hWnd = nullptr) override;
	CString GetCurrentKeyPath() override;
	void DisplayError(PCWSTR msg, HWND hWnd = nullptr, DWORD error = ::GetLastError()) const override;
	//bool AddMenu(HMENU hMenu) override;

	CString GetColumnText(HWND, int row, int col) const;

	// int GetRowImage(HWND, int row) const;
	void DoSort(const SortInfo* si);
	bool IsSortable(HWND, int col) const;
	BOOL OnRightClickList(HWND, int row, int col, const POINT&);
	BOOL OnDoubleClickList(HWND, int row, int col, const POINT& pt);

	/*void DrawItem(LPDRAWITEMSTRUCT dis);*/

	enum {
		ID_LOCATION_FIRST = 1500
	};

	void UpdateList(bool force = false);

	BEGIN_MSG_MAP(CRegistryManagerView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COPYDATA, OnGoToKeyExternal)
		MESSAGE_HANDLER(WM_BUILD_TREE, OnBuildTree)
		MESSAGE_HANDLER(WM_FIND_UPDATE, OnFindUpdate)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_RUN, OnRunOnUIThread)
		NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING,OnTreeItemExpanding)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED,OnTreeSelChanged)
		NOTIFY_CODE_HANDLER(TVN_ENDLABELEDIT,OnTreeEndEdit)
		NOTIFY_CODE_HANDLER(TVN_BEGINLABELEDIT,OnTreeBeginEdit)
		NOTIFY_CODE_HANDLER(TVN_KEYDOWN,OnTreeKeyDown)
		NOTIFY_HANDLER(TreeId, NM_RCLICK, OnTreeContextMenu)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED,OnListItemChanged)
		NOTIFY_CODE_HANDLER(LVN_ENDLABELEDIT,OnListEndEdit)
		NOTIFY_CODE_HANDLER(LVN_BEGINLABELEDIT,OnListBeginEdit)
		NOTIFY_CODE_HANDLER(LVN_KEYDOWN,OnListKeyDown)
		//MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
		CHAIN_MSG_MAP(CAutoUpdateUI<CRegistryManagerView>)
		CHAIN_MSG_MAP(CVirtualListView<CRegistryManagerView>)
		REFLECT_NOTIFICATIONS_EX()
	ALT_MSG_MAP(2)
		MESSAGE_HANDLER(WM_KEYDOWN,OnEditKeyDown)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	// LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	// LRESULT OnMenuSelect(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnFindUpdate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBuildTree(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGoToKeyExternal(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRunOnUIThread(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	// LRESULT OnShowKeysInList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);



	//LRESULT OnFocusChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeItemExpanding(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeSelChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeEndEdit(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeBeginEdit(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeKeyDown(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeContextMenu(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void InvokeTreeContextMenu(const CPoint& pt);


	LRESULT OnListItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnListEndEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnListBeginEdit(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnListKeyDown(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);


	void ExpandItem(HTREEITEM hItem);
	HTREEITEM FindItemByPath(PCWSTR path) const;


	void InitTree();
	HTREEITEM BuildTree(HTREEITEM hRoot, HKEY hKey, PCWSTR name = nullptr);
	NodeType GetNodeData(HTREEITEM hItem) const;
	void SetNodeData(HTREEITEM hItem, NodeType type);
	HTREEITEM GotoKey(const CString& path, bool knownToExist = false);
	CString GetFullNodePath(HTREEITEM hItem) const;
	CString GetFullParentNodePath(HTREEITEM hItem) const;
	CString GetKeyDetails(const RegistryItem& item) const;
	CString GetValueDetails(const RegistryItem& item) const;
	HTREEITEM BuildKeyPath(const CString& path, bool accessible);
	CTreeItem InsertKeyItem(HTREEITEM hParent, PCWSTR name, NodeType type = NodeType::Key);

private:
	enum class StatusPane {
		Icon,
		Key
	};

	struct FindData {
		PCWSTR Path;
		PCWSTR Name;
		PCWSTR Data;
	};

	enum class ColumnType {
		Name, Type, Value, Details, Size, TimeStamp
	};

	enum class Operation {
		None,
		RenameKey,
		CreateKey,
		CreateValue,
		RenameValue,
	};
	enum class ClipboardOperation {
		Copy,Cut
	};

	struct ClipboardItem {
		CString Path;
		CString Name;
		bool Key;
	};

	struct LocalClipboard {
		ClipboardOperation Operation;
		std::vector<ClipboardItem> Items;
	};

	CommandManager m_CmdMgr;
	LocationManager m_Locations;
	LocalClipboard m_Clipboard;
	mutable RegistryKey m_CurrentKey;
	CString m_CurrentPath;
	CListViewCtrl m_List;
	mutable CTreeViewCtrlEx m_Tree;
	CListViewCtrl m_Details;
	std::vector<RegistryItem> m_Items;
	CTreeItem m_hLocalRoot, m_hStdReg, m_hRealReg;
	NodeType m_CurrentNodeType{ NodeType::None };
	int m_CurrentSelectedItem{ -1 };
	AppSettings m_Settings;
	Operation m_CurrentOperation{ Operation::None };
	HANDLE m_hSingleInstMutex{ nullptr };
	CString m_StartKey;
	CComObject<CEnumStrings>* m_AutoCompleteStrings{ nullptr };
	Theme m_DarkTheme, m_DefaultTheme{ true };
	CFont m_Font;
	bool m_ReadOnly{ true };
	bool m_UpdateNoDelay{ false };
	

public:
	CSplitterWindow m_MainSplitter;
	CContainedWindowT<CEdit> m_AddressBar;
};