#include "stdafx.h"
#include "RegistryManager.h"
#include "Internals.h"


RegistryManager* RegistryManager::_instance;

RegistryManager::RegistryManager(CTreeViewCtrl& tree,CRegistryManagerView& view):_tree(tree),_view(view){
	ATLASSERT(_instance == nullptr);
	_instance = this;
	OBJECT_ATTRIBUTES keyAttr;
	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, L"\\REGISTRY");

	InitializeObjectAttributes(&keyAttr, &keyName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	HANDLE h;
	auto status = ::NtOpenKey(&h, KEY_READ | KEY_ENUMERATE_SUB_KEYS, &keyAttr);
	ATLASSERT(NT_SUCCESS(status));

	_registryRoot = new RegKeyTreeNode(nullptr, L"REGISTRY", (HKEY)h);

	_stdRegistryRoot = new TreeNodeBase(L"Standard Registry");
	_stdRegistryRoot->AddChild(_HKCR = new RegKeyTreeNode(HKEY_CLASSES_ROOT, L"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT));
	_stdRegistryRoot->AddChild(_HKCU = new RegKeyTreeNode(HKEY_CURRENT_USER, L"HKEY_CURRENT_USER", HKEY_CURRENT_USER));
	_stdRegistryRoot->AddChild(_HKLM = new RegKeyTreeNode(HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE));
	_stdRegistryRoot->AddChild(_HKUsers = new RegKeyTreeNode(HKEY_USERS, L"HKEY_USERS", HKEY_USERS));
	_stdRegistryRoot->AddChild(_HKCC = new RegKeyTreeNode(HKEY_CURRENT_CONFIG, L"HKEY_CURENT_CONFIG", HKEY_CURRENT_CONFIG));
	_stdRegistryRoot->AddChild(_HKPD = new RegKeyTreeNode(HKEY_PERFORMANCE_DATA, L"HKEY_PERFORMANCE_DATA", HKEY_PERFORMANCE_DATA));
	_stdRegistryRoot->AddChild(_HKPT = new RegKeyTreeNode(HKEY_PERFORMANCE_TEXT, L"HKEY_PERFORMANCE_TEXT", HKEY_PERFORMANCE_TEXT));
	_stdRegistryRoot->AddChild(_HKPNT = new RegKeyTreeNode(HKEY_PERFORMANCE_NLSTEXT, L"HKEY_PERFORMANCE_NLSTEXT", HKEY_PERFORMANCE_NLSTEXT));
	_stdRegistryRoot->AddChild(_HKCULS = new RegKeyTreeNode(HKEY_CURRENT_USER_LOCAL_SETTINGS, L"HKEY_CURRENT_USER_LOCAL_SETTINGS", HKEY_CURRENT_USER_LOCAL_SETTINGS));

	BuildHiveList();

	_deletedKey.Create(HKEY_CURRENT_USER, DeletedKey, nullptr, 0, KEY_CREATE_SUB_KEY | KEY_READ, nullptr, nullptr);
	ATLASSERT(_deletedKey);
}

bool RegistryManager::IsHive(TreeNodeBase* node) const {
	auto text = L"\\" + node->GetFullPath();
	if (text.Left(9) == L"\\REGISTRY") {
	}
	else {
		text.Replace(L"\\Standard Registry\\HKEY_LOCAL_MACHINE\\", L"\\REGISTRY\\MACHINE\\");
		text.Replace(L"\\Standard Registry\\HKEY_USERS\\", L"\\REGISTRY\\USER\\");
	}
	if (_hiveList.Lookup(text))
		return true;

	return false;
}

HTREEITEM RegistryManager::AddItem(TreeNodeBase* item, HTREEITEM hParent, HTREEITEM hAfter) {
	if (IsHive(item)) {
		static_cast<RegKeyTreeNode*>(item)->SetHive(true);
	}
	// 插入一个树项
	auto hItem = _tree.InsertItem(item->GetText(), item->GetImage(), item->GetSelectedImage(), hParent, hAfter);
	_tree.SetItemData(hItem, reinterpret_cast<LPARAM>(item));// 方便后面取数据
	item->_hItem = hItem;
	if (item->HasChildren()) {
		_tree.InsertItem(L"***", hItem, TVI_LAST);
	}
	return hItem;
}

void RegistryManager::BuildTreeView() {
	WCHAR computerName[64] = L"Local Computer";
	DWORD size = _countof(computerName);
	::GetComputerName(computerName, &size);
	auto rootNode = new TreeNodeBase(computerName);
	rootNode->SetImage(6); // ?? 为什么是6？
	auto hRootNode = AddItem(rootNode, TVI_ROOT);// 增加根项

	TreeNodeBase* roots[] = { _registryRoot,_stdRegistryRoot };
	rootNode->AddChild(roots[1]);
	rootNode->AddChild(roots[0]);

	for (auto& root : roots) {
		auto hRoot = AddItem(root, hRootNode);// 增加子根项
		for (const auto& node : root->GetChildNodes()) {
			AddItem(node, hRoot);// 增加子项
		}
	}

	// 展开
	_tree.Expand(hRootNode, TVE_EXPAND);
	_tree.Expand(roots[0]->GetHItem(), TVE_EXPAND);
	_tree.Expand(roots[1]->GetHItem(), TVE_EXPAND);
	// 选中
	_tree.SelectItem(_stdRegistryRoot->GetHItem());
}

void RegistryManager::BuildHiveList() {
	_hiveList.RemoveAll();	// 清除旧的
	CRegKey key;
	if (ERROR_SUCCESS != key.Open(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\hivelist", KEY_READ))
		return;

	WCHAR name[256];
	WCHAR data[512];
	DWORD type;
	for (DWORD i = 0;; i++) {
		DWORD len = _countof(name), datalen = _countof(data);
		if (ERROR_SUCCESS != ::RegEnumValue(key, i, name, &len, nullptr, &type, (BYTE*)data, &datalen))
			break;

		_hiveList.SetAt(name, data);
	}
}

void RegistryManager::ExpandItem(TreeNodeBase* node) {
	auto hItem = node->GetHItem();
	ATLASSERT(hItem);

	auto hChild = _tree.GetChildItem(hItem);
	if (hChild == nullptr)
		return;

	if (_tree.GetItemData(hChild) == 0) {
		_tree.DeleteItem(hChild);

		_tree.LockWindowUpdate();
		CWaitCursor wait;
		node->Expand(true);
		for (auto& n : node->GetChildNodes()) {
			AddItem(n, hItem);
		}
		_tree.SortChildren(hItem);
		_tree.LockWindowUpdate(FALSE);
	}
}

LSTATUS RegistryManager::CreateKey(const CString& parent, const CString& name) {
	CString path;
	auto root = GetRoot(parent, path);
	ATLASSERT(root);

	CRegKey key;
	auto status = key.Create(*root->GetKey(), path + L"\\" + name);
	if (status != ERROR_SUCCESS)
		return status;

	auto parentNode = FindNode(root, path);
	ATLASSERT(parentNode);
	auto node = new RegKeyTreeNode(*root->GetKey(), name, key.Detach());
	parentNode->AddChild(node);
	if (IsExpanded(parentNode))
		AddItem(node, parentNode->GetHItem());
	_view.Update(parentNode, true);

	return ERROR_SUCCESS;
}

bool RegistryManager::SelectNode(TreeNodeBase* parent, PCWSTR name) {
	if (name == nullptr) {
		_tree.SelectItem(parent->GetHItem());
		return true;
	}
	else {
		const auto& nodes = parent->GetChildNodes();
		_tree.Expand(parent->GetHItem(), TVE_EXPAND);
		for (const auto& node : nodes) {
			if (node->GetText().Compare(name) == 0) {
				ATLASSERT(node->GetHItem());
				if (_tree.SelectItem(node->GetHItem())) {
					_tree.Expand(node->GetHItem(), TVE_EXPAND);
					ExpandItem(node);
				}
				return true;
			}
		}
	}
	return false;
}

void RegistryManager::AddNewKeys(RegKeyTreeNode* node) {
	WCHAR name[128];
	auto key = *node->GetKey();
	FILETIME lastWrite;
	for (DWORD index = 0;; index++) {
		DWORD len = 128;
		if (!key || key.EnumKey(index, name, &len, &lastWrite)!=ERROR_SUCCESS)
			break;

		if (!node->FindChild(name)) {
			CRegKey subkey;
			subkey.Open(key, name, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
			auto newnode = new RegKeyTreeNode(node->GetRoot(), name, subkey.Detach());
			node->AddChild(newnode);
			AddItem(newnode, node->GetHItem());
		}
	}
}

bool RegistryManager::IsExpanded(TreeNodeBase* node) const{
	auto hChild = _tree.GetChildItem(node->GetHItem());
	if (hChild == nullptr && node->GetChildNodes().size() == 0)
		return false;

	if (hChild == nullptr)
		return true;

	return _tree.GetItemData(hChild) != 0;
}

void RegistryManager::Refresh(TreeNodeBase* node) {
	if (!node->IsExpanded())
		return;

	ATLASSERT(L"RegistryManager::Refresh %s\n", node->GetFullPath());

	for (auto& child : node->GetChildNodes())
		Refresh(child);

	if (node->GetNodeType() == TreeNodeType::RegistryKey) {
		auto regNode = static_cast<RegKeyTreeNode*>(node);
		int deleted = 0;
		ATLASSERT(*regNode->GetKey());
		for (size_t i = 0; i < regNode->GetChildNodes().size(); i++) {
			auto child = regNode->GetChildNodes()[i];
			CRegKey subkey;
			if (ERROR_FILE_NOT_FOUND == subkey.Open(*regNode->GetKey(), child->GetText(), KEY_READ)) {
				regNode->RemoveChild(i);
				_tree.DeleteItem(child->GetHItem());
				i--;
				deleted++;
			}
		}
		BYTE buffer[2048];
		auto info = reinterpret_cast<KEY_FULL_INFORMATION*>(buffer);
		ULONG len;
		if (NT_SUCCESS(::NtQueryKey(*regNode->GetKey(), KeyFullInformation, info, 2048, &len))) {
			if (info->SubKeys != regNode->GetChildNodes().size()) {
				// new keys
				AddNewKeys(regNode);
			}
		}
	}
}

void RegistryManager::Refresh() {
	_tree.LockWindowUpdate();

	Refresh(_stdRegistryRoot);
	Refresh(_registryRoot);
	
	_tree.LockWindowUpdate(FALSE);
}

LRESULT RegistryManager::HandleNotification(NMHDR* nmhdr) {
	auto code = nmhdr->code;
	auto tv = reinterpret_cast<NMTREEVIEW*>(nmhdr);
	auto hItem = tv->itemNew.hItem;

	switch (code) {
		case TVN_ITEMEXPANDING:
			if ((tv->action & TVE_EXPAND) == 0)
				break;

			auto node = reinterpret_cast<TreeNodeBase*>(_tree.GetItemData(hItem));
			ATLASSERT(node);
			if (node->HasChildren())
				ExpandItem(node);
			break;
	}
	return 0;
}

void RegistryManager::Destroy() {
	CRegKey key(HKEY_CURRENT_USER);
	key.DeleteSubKey(DeletedKey);
}

void RegistryManager::GetHiveAndPath(const CString& parent, CString& hive, CString& path) {
	auto firstSlash = parent.Find('\\') + 1;
	auto secondSlash = parent.Find(L'\\', firstSlash);
	hive = parent.Mid(firstSlash, parent.Find(L'\\', firstSlash + 1) - firstSlash);
	path = parent.Mid(secondSlash + 1);
}

RegKeyTreeNode* RegistryManager::GetHiveNode(const CString& name) const {
	static const struct {
		PCWSTR Name;
		RegKeyTreeNode* Node;
	} hives[] = {
		{ L"HKEY_LOCAL_MACHINE", _HKLM },
		{ L"HKEY_CURRENT_USER", _HKCU },
		{ L"HKEY_CLASSES_ROOT", _HKCR },
		{ L"HKEY_USERS", _HKUsers },
		{ L"HKEY_CURRENT_CONFIG", _HKCC },
	};

	for (auto& hive : hives)
		if (hive.Name == name)
			return hive.Node;
	return nullptr;
}

RegKeyTreeNode* RegistryManager::GetRoot(const CString& parent, CString& path) {
	CString hive;
	RegKeyTreeNode* root;
	if (parent.Left(8) == L"REGISTRY") {
		// real registry
		path = parent.Mid(9);
		root = _registryRoot;
	}
	else {
		GetHiveAndPath(parent, hive, path);
		root = GetHiveNode(hive);
	}
	return root;
}

TreeNodeBase* RegistryManager::FindNode(TreeNodeBase* root, const CString& path) const {
	int index = 0;
	//auto name = path.Tokenize(L"\\", index);
	for (;;) {
		auto name = path.Tokenize(L"\\", index);
		if (index < 0)
			break;

		auto old = root;
		for (auto& node : root->GetChildNodes()) {
			if (node->GetText().CompareNoCase(name) == 0) {
				root = node;
				break;
			}
		}
		if (old == root)	// not found
			return nullptr;
	}
	return root;
}

LRESULT RegistryManager::SetValue(CRegKey& key, const CString& name, const ULONGLONG& value, DWORD type) {
	if (type == REG_DWORD)
		return key.SetDWORDValue(name, (DWORD)value);
	return key.SetQWORDValue(name, value);
}

LRESULT RegistryManager::SetValue(CRegKey& key, const CString& name, const CString& value, DWORD type) {
	if (type == REG_MULTI_SZ) {
		CString temp(value);
		temp.Replace(L"\r\n", L"\n");
		auto count = temp.GetLength();
		auto p = temp.GetBuffer();
		for (; *p; p++) {
			auto q = ::wcschr(p, L'\n');
			if (q == nullptr)
				break;
			*q = L'\0';
			p = q + 1;
		}
		return key.SetMultiStringValue(name, temp.GetBuffer());
	}

	ATLASSERT(type == REG_SZ || type == REG_EXPAND_SZ);
	return key.SetStringValue(name, value, type);
}

LRESULT RegistryManager::SetValue(CRegKey& key, const CString& name, const BinaryValue& value, DWORD type) {
	ATLASSERT(type == REG_BINARY);
	return key.SetBinaryValue(name, value.Buffer.get(), value.Size);
}

LSTATUS RegistryManager::DeleteKey(const CString& parent, const CString& name) {
	CString path;
	auto root = GetRoot(parent, path);
	ATLASSERT(root);

	CRegKey key;
	auto status = key.Open(*root->GetKey(), path, DELETE);
	if (status != ERROR_SUCCESS)
		return status;

	status = key.DeleteSubKey(name);
	if (status != ERROR_SUCCESS)
		return status;

	auto parentNode = FindNode(root, path);
	ATLASSERT(parentNode);
	auto node = parentNode->RemoveChild(name);
	ATLASSERT(node);
	_tree.DeleteItem(node->GetHItem());
	_view.Update(parentNode, true);

	return status;
}

LSTATUS RegistryManager::RenameKey(const CString& parent, const CString& name) {
	CString path;
	auto root = GetRoot(parent, path);
	ATLASSERT(root);

	CRegKey key;
	auto status = key.Open(*root->GetKey(), path, KEY_WRITE);
	if (status != ERROR_SUCCESS)
		return status;

	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, name);
	status = ::NtRenameKey((HANDLE)key, &uname);
	if (NT_SUCCESS(status)) {
		auto node = FindNode(root, path);
		if (node) {
			_tree.SetItemText(node->GetHItem(), name);
			node->SetText(name);
		}
	}
	return status;
}

LSTATUS RegistryManager::RenameValue(const CString& path, const CString& oldName, const CString& newName) {
	CString realPath;
	auto root = GetRoot(path, realPath);
	if (root == nullptr)
		return ERROR_NOT_FOUND;


	CRegKey key;
	auto status = key.Open(*root->GetKey(), realPath, KEY_READ | KEY_WRITE);
	if (status != ERROR_SUCCESS)
		return status;

	DWORD type, size = 0;
	status = key.QueryValue(oldName, &type, nullptr, &size);
	if (status != ERROR_SUCCESS)
		return status;

	auto buffer = std::make_unique<BYTE[]>(size);
	status = key.QueryValue(oldName, &type, buffer.get(), &size);
	if (status != ERROR_SUCCESS)
		return status;

	status = key.SetValue(newName, type, buffer.get(), size);
	if (status != ERROR_SUCCESS)
		return status;

	status = key.DeleteValue(oldName);
	return status;
}

LSTATUS RegistryManager::DeleteValue(const CString& path, const CString& name) {
	CString realpath;
	auto root = GetRoot(path, realpath);
	ATLASSERT(root);

	CRegKey key;
	auto status = key.Open(*root->GetKey(), realpath, KEY_WRITE);
	if (status != ERROR_SUCCESS)
		return status;

	status = key.DeleteValue(name);
	if (status == ERROR_SUCCESS)
		_view.Update(nullptr, true);
	return status;
}