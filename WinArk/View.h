#pragma once

#include "resource.h"
#include "VirtualListView.h"


struct TreeNodeBase;
struct ITreeOperations;
struct IMainApp;

struct ListItem {
	const std::wstring& GetName() const;
	const std::wstring& GetType() const;

	TreeNodeBase* TreeNode{ nullptr };
	CString ValueName;
	DWORD ValueType, ValueSize = 0;
	LARGE_INTEGER LastWriteTime = { 0 };
	mutable std::wstring Name, Type;
	bool UpDir{ false };
};

class CRegistryManagerView :
	public CWindowImpl<CRegistryManagerView,CListViewCtrl>,
	public CVirtualListView<CRegistryManagerView> {
public:
	DECLARE_WND_SUPERCLASS(nullptr,CListViewCtrl::GetWndClassName())

	const ListItem& GetItem(int index) const;
	ListItem& GetItem(int index);

	CString GetDataAsString(const ListItem& item) const;
	static CString GetKeyDetails(TreeNodeBase*);
	static PCWSTR GetRegTypeAsString(DWORD type);

	bool CanEditValue() const;
	bool IsViewKeys() const;
	bool CanDeleteSelected() const;

	void Update(TreeNodeBase* node, bool ifTheSame = false);
	void Init(ITreeOperations*, IMainApp*);

	CString GetColumnText(HWND hWnd, int row, int column) const;
	void DoSort(const SortInfo* si);

	BEGIN_MSG_MAP(CRegistryManagerView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU,OnContextMenu)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK,OnDoubleClick)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_RETURN, OnReturnKey)	// 回车键
		REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRightClick)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_BEGINLABELEDIT, OnBeginRename)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ENDLABELEDIT, OnEndRename)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		CHAIN_MSG_MAP_ALT(CVirtualListView<CRegistryManagerView>, 1); // 消息节1
		DEFAULT_REFLECTION_HANDLER()
		ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_DELETE, OnDelete)
		COMMAND_ID_HANDLER(ID_EDIT_RENAME, OnEditRename)
		COMMAND_ID_HANDLER(ID_EDIT_MODIFYVALUE, OnModifyValue)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		ALT_MSG_MAP(2)
		COMMAND_ID_HANDLER(ID_NEW_DWORDVALUE, OnNewDwordValue)
		COMMAND_ID_HANDLER(ID_NEW_QWORDVALUE, OnNewQwordValue)
		COMMAND_ID_HANDLER(ID_NEW_STRINGVALUE, OnNewStringValue)
		COMMAND_ID_HANDLER(ID_NEW_MULTIPLESTRINGSVALUE, OnNewMultiStringValue)
		COMMAND_ID_HANDLER(ID_NEW_EXPANDSTRINGVALUE, OnNewExpandStringValue)
		COMMAND_ID_HANDLER(ID_NEW_BINARYVALUE,OnNewBinaryValue)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnModifyValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnReturnKey(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnDoubleClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnBeginRename(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnEndRename(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	LRESULT OnNewDwordValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewQwordValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewStringValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewExpandStringValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewMultiStringValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewBinaryValue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT HandleNewIntValue(int size);
	LRESULT HandleNewStringValue(DWORD type);

	static bool CompareItems(const ListItem& i1, const ListItem& i2, int col, bool asc);

private:

	CEdit m_Edit;
	TreeNodeBase* m_CurrentNode{ nullptr };
	ITreeOperations* m_TreeOperations;
	IMainApp* m_App;
	std::vector<ListItem> m_Items;
	bool m_ViewKeys{ true };

public:
	CSplitterWindow m_Splitter;
};