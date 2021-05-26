#pragma once

#include "TreeNode.h"


class RegKeyTreeNode :public TreeNodeBase {
public:
	RegKeyTreeNode(HKEY hive, const CString& text, HKEY hKey) :TreeNodeBase(text, nullptr), _key(hKey), _root(hive) {
	}

	virtual TreeNodeType GetNodeType() const {
		return TreeNodeType::RegistryKey;
	}

	virtual int GetContextMenuIndex() const override{
		return 1;
	}

	bool Expand(bool expand) override;
	bool IsExpanded() const override;
	bool HasChildren() const override;
	bool CanDelete() const override;

	void SetLink(PCWSTR path) {
		_link = true;
		_linkPath = path;
	}

	const CString& GetLinkPath() const {
		return _linkPath;
	}

	CRegKey* GetKey() {
		return &_key;
	}

	void SetHive(bool hive) {
		_hive = hive;
	}

	HKEY GetRoot() {
		return _root;
	}

private:
	mutable CRegKey _key;
	CString _linkPath;
	HKEY _root;
	bool _expanded : 1{false};
	bool _hive : 1{false};
	bool _link : 1{false};
};