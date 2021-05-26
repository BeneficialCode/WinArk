#pragma once

template<typename T>
struct CVirtualListView {
	BEGIN_MSG_MAP(CVirtualListView)
		NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK,OnColumnClick)
		NOTIFY_CODE_HANDLER(LVN_ODFINDITEM,OnFindItem)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,OnGetDispInfo) // 更新列表控件
		ALT_MSG_MAP(1)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,OnGetDispInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK,OnColumnClick)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ODFINDITEM, OnFindItem)
	END_MSG_MAP()

	struct SortInfo {
		int SortColumn = -1;
		UINT_PTR Id;
		HWND hWnd;
		bool SortAscending;
	private:
		friend struct CVirtualListView;
		int RealSortColumn = -1;
	};

	bool ClearSort(UINT_PTR id = 0) {
		auto si = FindById(id);
		if (si == nullptr)
			return false;

		auto header = CListViewCtrl(si->hWnd).GetHeader();
		HDITEM h;
		h.mask = HDI_FORMAT;
		header.GetItem(si->SortColumn, &h);
		h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING;
		header.SetItem(si->SortColumn, &h);
		si->SortColumn = -1;
		return true;
	}

	bool ClearSort(HWND hWnd) {
		auto si = FindByHwnd(hWnd);
		if (si == nullptr)
			return false;

		auto header = CListViewCtrl(si->hWnd).GetHeader();
		HDITEM h;
		h.mask = HDI_FORMAT;
		header.GetItem(si->RealSortColumn, &h);
		si->SortColumn = -1;
		return true;
	}

protected:
	/*
	* 当Virtual List 需要显示一行数据时,
	它发送这个消息给父窗口询问数据内容.
	父窗口收到消息后从传过来参数中知道Virtual List 现在显示的是第几项,第几列,
	内容是什么类型.然后父窗口在外部数据中找到数据内容,
	将它返回给Virtual List.
	*/
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR hdr, BOOL&/*bHandled*/) {
		auto lv = (NMLVDISPINFO*)hdr;
		auto& item = lv->item;
		auto p = static_cast<T*>(this);
		if (item.mask & LVIF_TEXT)
			::StringCchCopy(item.pszText, item.cchTextMax, p->GetColumnText(hdr->hwndFrom, item.iItem, item.iSubItem));

		return 0;
	}

	bool IsSortable(int) const {
		return true;
	}

	LRESULT OnColumnClick(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
		auto lv = (NMLISTVIEW*)hdr;
		auto col = lv->iSubItem;

		auto p = static_cast<T*>(this);
		if (!p->IsSortable(col))
			return 0;

		auto si = FindById(hdr->idFrom);
		if (si == nullptr) {
			SortInfo s;
			s.hWnd = hdr->hwndFrom;
			s.Id = hdr->idFrom;
			m_Controls.push_back(s);
			si = m_Controls.data() + m_Controls.size() - 1;
		}

		auto oldSortColumn = si->SortColumn;
		if (col == si->SortColumn)
			si->SortAscending = !si->SortAscending;
		else {
			si->SortColumn = col;
			si->SortAscending = true;
		}

		HDITEM h;
		h.mask = HDI_FORMAT;
		CListViewCtrl list(hdr->hwndFrom);
		auto header = list.GetHeader();
		header.GetItem(si->SortColumn, &h);
		h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING | (si->SortAscending ? HDF_SORTUP : HDF_SORTDOWN);
		header.SetItem(si->SortColumn, &h);

		if (oldSortColumn >= 0 && oldSortColumn != si->SortColumn) {
			h.mask = HDI_FORMAT;
			header.GetItem(oldSortColumn, &h);
			h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING;
			header.SetItem(oldSortColumn, &h);
		}

		static_cast<T*>(this)->DoSort(si);
		list.RedrawItems(list.GetTopIndex(), list.GetTopIndex() + list.GetCountPerPage());

		return 0;
	}

	LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
		auto fi = (NMLVFINDITEM*)hdr;
		auto text = fi->lvfi.psz;
		auto len = ::wcslen(text);
		auto list = fi->hdr.hwndFrom;

		if (ListView_GetSelectedCount(list) == 0)
			return -1;

		int selected = ListView_GetNextItem(list, -1, LVIS_SELECTED);
		int start = selected + 1;
		int count = ListView_GetItemCount(list);
		WCHAR name[256];
		for (int i = start; i < count + start; i++) {
			ListView_GetItemText(list, i % count, 0, name, _countof(name));
			if (::_wcsnicmp(name, text, len) == 0)
				return i % count;
		}
		return -1;
	}

	CString GetColumnText(HWND hWnd, int row, int column) const {
		return L"";
	}

	int GetSortColumn(UINT_PTR id = 0) const {
		auto si = FindById(id);
		return si ? si->SortColumn : -1;
	}

	int IsSortAscending(UINT_PTR id) const {
		auto si = FindById(id);
		return si ? si->SortAscending : false;
	}

	SortInfo* GetSortInfo(HWND h = nullptr) {
		if (h == nullptr && m_Controls.empty())
			return nullptr;
		return h == nullptr ? &m_Controls[0] : FindByHwnd(h);
	}

private:
	SortInfo* FindById(UINT_PTR id) const {
		if (id == 0)
			return m_Controls.empty() ? nullptr : &m_Controls[0];
		for (auto& info : m_Controls)
			if (info.Id == id)
				return &info;
		return nullptr;
	}

	SortInfo* FindByHwnd(HWND h) const {
		if (h == nullptr)
			return m_Controls.empty() ? nullptr : &m_Controls[0];
		for (auto& info : m_Controls)
			if (info.hWnd == h)
				return &info;
		return nullptr;
	}

	mutable std::vector<SortInfo> m_Controls;
};