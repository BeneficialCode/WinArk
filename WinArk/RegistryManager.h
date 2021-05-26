#pragma once
#include "TreeNodes.h"
#include "UICommon.h"
#include "View.h"

struct BinaryValue {
	unsigned Size;
	std::unique_ptr<BYTE[]> Buffer;
};

class RegistryManager:public ITreeOperations {
public:
	inline static const WCHAR* DeletedKey = L"__deleted__";

	static RegistryManager& Get() {
		return *_instance;
	}

	void BuildTreeView();
	LRESULT HandleNotification(NMHDR* nmhdr);
	void ExpandItem(TreeNodeBase* node);
	CRegKey& GetDeletedKey() {
		return _deletedKey;
	}

	LSTATUS CreateKey(const CString& parent, const CString& name);
	LSTATUS DeleteKey(const CString& parent, const CString& name);
	LSTATUS RenameKey(const CString& parent, const CString& name);
	LSTATUS RenameValue(const CString& path, const CString& oldName, const CString& newName);
	LSTATUS DeleteValue(const CString& path, const CString& name);

	RegKeyTreeNode* GetRoot(const CString& parent, CString& path);


public:
	template<typename T>
	LSTATUS CreateValue(const CString& path, const CString& name, const T& value, DWORD type);

	TreeNodeBase* FindNode(TreeNodeBase* root, const CString& path) const;
	RegKeyTreeNode* GetHiveNode(const CString& name) const;
	bool IsExpanded(TreeNodeBase* node) const;
	bool IsHive(TreeNodeBase* node) const;
	void GetHiveAndPath(const CString& parent, CString& hive, CString& path);
	void Refresh();

	void Destroy();
private:
	friend class CMainDlg;
	RegistryManager(CTreeViewCtrl& tree,CRegistryManagerView& view);
	void BuildHiveList();

	HTREEITEM AddItem(TreeNodeBase* item, HTREEITEM hParent, HTREEITEM hAfter = TVI_LAST);// 默认在最后项之后
	void Refresh(TreeNodeBase* node);
	void AddNewKeys(RegKeyTreeNode* node);

	LRESULT SetValue(CRegKey& key, const CString& name, const ULONGLONG& value, DWORD type);
	LRESULT SetValue(CRegKey& key, const CString& name, const CString& value, DWORD type);
	LRESULT SetValue(CRegKey& key, const CString& name, const BinaryValue& value, DWORD type);

	RegKeyTreeNode* _registryRoot;
	TreeNodeBase* _stdRegistryRoot;

	RegKeyTreeNode* _HKLM;		// HKEY_LOCAL_MACHINE
	RegKeyTreeNode* _HKCR;		// HKEY_CLASSES_ROOT 
	RegKeyTreeNode* _HKCU;		// HKEY_CURRENT_USER
	RegKeyTreeNode* _HKUsers;	// HKEY_USERS
	RegKeyTreeNode* _HKCC;		// HKEY_CLASSES_CONFIG
	RegKeyTreeNode* _HKPT;		// HKEY_PERFORMANCE_TEXT
	RegKeyTreeNode* _HKPD;		// HKEY_PERFORMANCE_DATA
	RegKeyTreeNode* _HKCULS;	// HKEY_CURRENT_USER_LOCAL_SETTINGS
	RegKeyTreeNode* _HKPNT;		// HKEY_PERFORMANCE_NLSTEXT

	CAtlMap<CString, CString> _hiveList;

	CTreeViewCtrl& _tree;
	CRegKey _deletedKey;

	// Inherited via ITreeOperations
	bool SelectNode(TreeNodeBase* parent, PCWSTR name) override;
	CRegistryManagerView& _view;
	static RegistryManager* _instance;
};

template<typename T>
inline LSTATUS RegistryManager::CreateValue(const CString& path, const CString& name, const T& value, DWORD type) {
	CString realpath;
	auto root = GetRoot(path, realpath);
	ATLASSERT(root);

	CRegKey key;
	auto status = key.Open(*root->GetKey(), realpath, KEY_WRITE);
	if (status != ERROR_SUCCESS)
		return status;

	status = (LSTATUS)SetValue(key, name, value, type);
	if (status == ERROR_SUCCESS)
		_view.Update(nullptr, true);
	return status;
}