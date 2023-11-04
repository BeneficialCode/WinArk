#pragma once

#include "resource.h"
#include "VirtualListView.h"
#include "Interfaces.h"
#include "Registry.h"
#include "CommandManager.h"
#include "EnumStrings.h"
#include "DeleteKeyCommand.h"
#include "DeleteValueCommand.h"
#include "LocationManager.h"
#include "AppSettings.h"
#include "Theme.h"
#include "SecurityInformation.h"


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
	public IRegView {
public:
	DECLARE_WND_CLASS(L"RegExpWndClass")
	
	CRegistryManagerView():m_AddressBar(this,2){}

	const UINT WM_BUILD_TREE = WM_APP + 11;
	const UINT WM_FIND_UPDATE = WM_APP + 12;
	const UINT WM_RUN = WM_APP + 13;
	const UINT TreeId = 123;

	const UINT ID_EXPORT_BIN = 32000;

	void SetStartKey(const CString& key);

	void RunOnUiThread(std::function<void()> f);
	
	// IMainFrame
	HWND GetHwnd() const override;
	void OnFindNext(PCWSTR path, PCWSTR name, PCWSTR data) override;
	void OnFindStart();
	void OnFindEnd(bool cancelled);
	bool GoToItem(PCWSTR path, PCWSTR name, PCWSTR data) override;
	// BOOL TrackPopupMenu(HMENU hMenu, DWORD flags, int x, int y, HWND hWnd = nullptr) override;
	CString GetCurrentKeyPath() override;
	void DisplayError(PCWSTR msg, HWND hWnd = nullptr, DWORD error = ::GetLastError()) const override;
	//bool AddMenu(HMENU hMenu) override;

	CString GetColumnText(HWND, int row, int col) const;

	int GetRowImage(HWND hWnd, int row, int col) const;
	void DoSort(const SortInfo* si);
	bool IsSortable(HWND, int col) const;
	BOOL OnRightClickList(HWND, int row, int col, const POINT&);
	BOOL OnDoubleClickList(HWND, int row, int col, const POINT& pt);
	int GetKeyImage(const RegistryItem& item) const;
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
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH,OnViewRefresh)
		COMMAND_ID_HANDLER(ID_NEW_KEY,OnNewKey)
		COMMAND_ID_HANDLER(ID_TREE_REFRESH,OnTreeRefresh)
		COMMAND_ID_HANDLER(ID_NEW_DWORDVALUE, OnNewValue)
		COMMAND_ID_HANDLER(ID_NEW_QWORDVALUE, OnNewValue)
		COMMAND_ID_HANDLER(ID_NEW_MULTIPLESTRINGVALUE, OnNewValue)
		COMMAND_ID_HANDLER(ID_NEW_BINARYVALUE, OnNewValue)
		COMMAND_ID_HANDLER(ID_NEW_STRINGVALUE,OnNewValue)
		COMMAND_ID_HANDLER(ID_NEW_EXPANDSTRINGVALUE,OnNewValue)
		COMMAND_ID_HANDLER(ID_EDIT_COPY,OnEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_CUT,OnEditCut)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE,OnEditPaste)
		COMMAND_ID_HANDLER(ID_EDIT_RENAME,OnEditRename)
		COMMAND_ID_HANDLER(ID_EDIT_DELETE,OnEditDelete)
		COMMAND_ID_HANDLER(ID_COPY_FULLNAME,OnCopyFullKeyName)
		COMMAND_ID_HANDLER(ID_COPY_NAME,OnCopyKeyName)
		COMMAND_ID_HANDLER(ID_KEY_PERMISSIONS,OnKeyPermissions)
		COMMAND_ID_HANDLER(ID_KEY_PROPERTIES, OnProperties)
		COMMAND_ID_HANDLER(ID_FILE_IMPORT,OnImport)
		COMMAND_ID_HANDLER(ID_FILE_EXPORT,OnExport)
		COMMAND_ID_HANDLER(ID_KEY_GOTO,OnGotoKey)
		COMMAND_ID_HANDLER(ID_EXPORT_BIN,OnExportBin)
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
	LRESULT OnGotoKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);




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

	void RefreshFull(HTREEITEM hItem);
	bool RefreshItem(HTREEITEM hItem);

	LRESULT OnViewRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTreeRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnNewKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	AppCommandCallback<DeleteKeyCommand> GetDeleteKeyCommandCallback();

	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyFullKeyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyKeyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnKeyPermissions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExportBin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);



	INT_PTR ShowValueProperties(RegistryItem& item, int index);



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
	Operation m_CurrentOperation{ Operation::None };
	HANDLE m_hSingleInstMutex{ nullptr };
	CString m_StartKey;
	CComObject<CEnumStrings>* m_AutoCompleteStrings{ nullptr };
	Theme m_DarkTheme, m_DefaultTheme{ true };
	CFont m_Font;
	bool m_ReadOnly{ false };
	bool m_UpdateNoDelay{ false };
	
public:
	CSplitterWindow m_MainSplitter;
	CContainedWindowT<CEdit> m_AddressBar;
};