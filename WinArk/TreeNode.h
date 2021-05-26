#pragma once

enum class TreeNodeType {
	Generic,
	RegistryKey,
};

struct TreeNodeBase {
	friend class RegistryManager;

	TreeNodeBase(const CString& text,TreeNodeBase* parent=nullptr):_parentNode(parent),_text(text),_full(text){ }

	virtual ~TreeNodeBase(){ }

	virtual TreeNodeType GetNodeType() const {
		return TreeNodeType::Generic;
	}

	void SetText(const CString& text) {
		_text = text;
	}

	HTREEITEM GetHItem() const {
		return _hItem;
	}

	const CString& GetText() const {
		return _text;
	}

	virtual int GetImage() const {
		return _image;
	}

	virtual int GetContextMenuIndex() const {
		return -1;
	}

	virtual int GetSelectedImage() const {
		return _selectedImage;
	}

	void SetImage(int image, int selected = -1) {
		_image = image;
		_selectedImage = selected < 0 ? image : selected;
	}
	void SetSelectedImage(int image) {
		_selectedImage = image;
	}
	
	TreeNodeBase* GetParent() {
		return _parentNode;
	}

	TreeNodeBase* AddChild(TreeNodeBase* child);

	TreeNodeBase* RemoveChild(const CString& name);
	TreeNodeBase* RemoveChild(size_t index);

	const std::vector<TreeNodeBase*>& GetChildNodes() {
		return _childNodes;
	}

	TreeNodeBase* FindChild(const CString& text) const;

	virtual void Delete() {
		delete this;
	}

	virtual bool Expand(bool expand) {
		return false;
	}

	virtual bool IsExpanded() const {
		return true;
	}

	virtual bool CanDelete() const {
		return false;
	}

	virtual bool HasChildren() const {
		return false;
	}

	const CString& GetFullPath() const {
		return _full;
	}

private:
	void SetParent(TreeNodeBase* parent);
protected:
	std::vector<TreeNodeBase*> _childNodes;
private:
	CString _text, _full;
	TreeNodeBase* _parentNode;
	HTREEITEM _hItem{ nullptr };
	int _image{ 0 }, _selectedImage{ 0 };
};