#include "stdafx.h"
#include "TreeNodes.h"
#include "RegistryManager.h"

bool RegKeyTreeNode::Expand(bool expand) {
	if (expand && GetChildNodes().empty()) {
		if (_key) {
			WCHAR linkPath[257];
			WCHAR name[256];
			DWORD len;
			FILETIME lastWrite;
			for (DWORD index = 0;; index++) {
				len = 128;
				if (_key.m_hKey == nullptr || _key.EnumKey(index, name, &len, &lastWrite) != ERROR_SUCCESS)
					break;

				if (::wcsicmp(name, RegistryManager::DeletedKey) == 0) // skip deleted key root
					continue;

				CRegKey key;
				auto error = key.Open(_key.m_hKey, name, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
				if (error == ERROR_SUCCESS) {
					bool link = false;
					CRegKey hLinkKey;
					error = ::RegOpenKeyEx(_key.m_hKey, name, REG_OPTION_OPEN_LINK, KEY_READ, &hLinkKey.m_hKey);
					if (error == ERROR_SUCCESS) {
						DWORD type = 0;
						DWORD size = sizeof(linkPath);
						auto error = ::RegQueryValueEx(hLinkKey, L"SymbolicLinkValue", nullptr, &type, (BYTE*)linkPath, &size);
						link = type == REG_LINK;
						if (link)
							linkPath[size / sizeof(WCHAR)] = L'\0';
					}

					auto node = new RegKeyTreeNode(_root, name, key.Detach());
					if (link)
						node->SetLink(linkPath);
					AddChild(node);
				}
			}
			_expanded = true;
		}
	}
	return expand;
}

bool RegKeyTreeNode::IsExpanded() const {
	return _expanded;
}

bool RegKeyTreeNode::HasChildren() const {
	if (!_key)
		return false;

	WCHAR name[4];
	DWORD len = 4;
	
	auto status = _key.EnumKey(0, name, &len);
	if (status == ERROR_SUCCESS || status == ERROR_MORE_DATA)
		return true;
	return false;
}

bool RegKeyTreeNode::CanDelete() const {
	return (ULONG_PTR)_key.m_hKey < (ULONG_PTR)(1 << 24) && !_hive;
}