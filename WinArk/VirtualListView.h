#pragma once

#include "ColumnManager.h"
#include <memory>

struct ColumnsState {
	int Count{ 0 };
	int SortColumn{ -1 };
	bool SortAscending{ true };
	std::unique_ptr<int[]> Order;
	std::unique_ptr<LVCOLUMN[]> Columns;
	std::unique_ptr<std::wstring[]> Text;
	std::unique_ptr<int[]> Tags;
};

enum class ListViewRowCheck {
	None,
	Unchecked,
	Checked
};

template<typename T>
struct CVirtualListView {
	BEGIN_MSG_MAP(CVirtualListView)
		NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK,OnColumnClick)
		NOTIFY_CODE_HANDLER(LVN_ODFINDITEM,OnFindItem)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,OnGetDispInfo) // 更新列表控件
		NOTIFY_CODE_HANDLER(NM_RCLICK,OnRightClick)
		NOTIFY_CODE_HANDLER(NM_DBLCLK,OnDoubleClick)
		ALT_MSG_MAP(1)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,OnGetDispInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK,OnColumnClick)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ODFINDITEM, OnFindItem)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnRightClick)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDoubleClick)
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

	LRESULT OnDoubleClick(int, LPNMHDR hdr, BOOL& handled) {
		WCHAR className[16];
		if (::GetClassName(hdr->hwndFrom, className, _countof(className)) && _wcsicmp(className, WC_LISTVIEW))
			return 0;

		CListViewCtrl lv(hdr->hwndFrom);
		POINT pt;
		::GetCursorPos(&pt);
		POINT pt2;
		lv.ScreenToClient(&pt);
		LVHITTESTINFO info{};
		info.pt = pt;
		lv.SubItemHitTest(&info);
		auto pT = static_cast<T*>(this);
		handled = pT->OnDoubleClickList(lv, info.iItem, info.iSubItem, pt2);
		return 0;
	}

	LRESULT OnRightClick(int, LPNMHDR hdr, BOOL& handled) {
		WCHAR className[16];
		if (!::GetClassName(hdr->hwndFrom, className, _countof(className))) {
			handled = FALSE;
			return 0;
		}

		if (::wcscmp(className, WC_LISTVIEW)) {
			handled = FALSE;
			return 0;
		}

		CListViewCtrl lv(hdr->hwndFrom);
		POINT pt;
		::GetCursorPos(&pt);
		POINT pt2(pt);
		auto header = lv.GetHeader();
		ATLASSERT(header);
		header.ScreenToClient(&pt);
		HDHITTESTINFO hti;
		hti.pt = pt;
		auto pT = static_cast<T*>(this);
		int index = header.HitTest(&hti);
		if (index >= 0) {
			handled = pT->OnRightClickHeader(index, pt2);
		}
		else {
			LVHITTESTINFO info{};
			info.pt = pt;
			lv.SubItemHitTest(&info);
			handled = pT->OnRightClickList(hdr->hwndFrom,info.iItem, info.iSubItem, pt2);
		}
		return 0;
	}

	bool OnRightClickHeader(int index, POINT& pt) {
		return false;
	}

	bool OnRightClickList(HWND, int row, int col, POINT& pt) {
		return false;
	}

	bool OnDoubleClickList(HWND, int row, int col, POINT& pt) {
		return false;
	}

protected:
	void UpdateList(CListViewCtrl& lv, int count, bool full = false) {
		lv.SetItemCountEx(count, full ? 0 : (LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL));
		auto top = lv.GetTopIndex(), page = lv.GetCountPerPage();
		if (!full && (lv.SendMessage(LVM_ISITEMVISIBLE, top) || lv.SendMessage(LVM_ISITEMVISIBLE, top + page - 1)))
			lv.RedrawItems(top, top + page);
	}

	ColumnManager* GetExistingColumnManager(HWND hListView) const {
		auto it = std::find_if(m_Columns.begin(), m_Columns.end(), [=](auto& cm) {
			return cm->GetListView() == hListView;
			});
		if (it != m_Columns.end())
			return (*it).get();
		return nullptr;
	}

	ColumnManager* GetColumnManager(HWND hListView) const {
		auto mgr = GetExistingColumnManager(hListView);
		if (mgr)
			return mgr;
		auto cm = std::make_unique<ColumnManager>(hListView);
		auto pcm = cm.get();
		m_Columns.push_back(std::move(cm));
		return pcm;
	}

	int GetRealColumn(HWND hListView, int column) const {
		auto cm = GetExistingColumnManager(hListView);
		return cm ? cm->GetRealColumn(column) : column;
	}
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
		auto col = GetRealColumn(hdr->hwndFrom, item.iSubItem);
		auto p = static_cast<T*>(this);
		if (item.mask & LVIF_TEXT) {
			auto name = p->GetColumnTextPointer(hdr->hwndFrom, item.iItem, col);
			if (name)
				item.pszText = (PWSTR)name;
			else
				::StringCchCopy(item.pszText, item.cchTextMax, p->GetColumnText(hdr->hwndFrom, item.iItem, item.iSubItem));
		}
		if (item.mask & LVIF_IMAGE) {
			item.iImage = p->GetRowImage(hdr->hwndFrom, item.iItem, col);
		}
		if (item.mask & LVIF_INDENT)
			item.iIndent = p->GetRowIndent(item.iItem);
		if ((ListView_GetExtendedListViewStyle(hdr->hwndFrom) & LVS_EX_CHECKBOXES)
			&& item.iSubItem == 0 && (item.mask & LVIF_STATE)) {
			item.state = LVIS_STATEIMAGEMASK;

			if (item.iItem == m_Selected) {
				item.state |= LVIS_SELECTED;
				item.stateMask |= LVIS_SELECTED;
			}
		}
		return 0;
	}

	int m_Selected = -1;

	LRESULT OnItemChanaged(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled) {
		auto lv = (NMLISTVIEW*)hdr;
		if (lv->uChanged & LVIF_STATE) {
			if (lv->uNewState & LVIS_SELECTED)
				m_Selected = lv->iItem;
		}
		return 0;
	}

	bool IsSortable(HWND, int) const {
		return true;
	}

	LRESULT OnColumnClick(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
		auto lv = (NMLISTVIEW*)hdr;
		// auto col = lv->iSubItem;
		auto col = GetRealColumn(hdr->hwndFrom, lv->iSubItem);

		auto p = static_cast<T*>(this);
		if (!p->IsSortable(hdr->hwndFrom,col))
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
	
	PCWSTR GetColumnTextPointer(HWND, int row, int col) const {
		return nullptr;
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

	int GetRowImage(HWND hWnd, int row,int col) const {
		return -1;
	}

	int GetRowIndent(int row) const {
		return 0;
	}

	// 行
	ListViewRowCheck IsRowChecked(int row) const {
		return ListViewRowCheck::None;
	}

	bool LoadState(HWND h, ColumnsState const& state) {
		if (state.Count == 0) {
			return false;
		}

		CListViewCtrl lv(h);
		while (lv.DeleteColumn(0))
			;
		auto header = lv.GetHeader();
		auto empty = header.GetItemCount() == 0;
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		for (int i = 0; i < state.Count; i++) {

		}
		if (state.SortColumn < 0)
			ClearSort(h);
		else {
			auto si = GetSortInfo(h);
			si->SortAscending = state.SortAscending;
			si->SortColumn = state.SortColumn;
		}
		GetColumnManager(h)->AddFromControl();
		lv.SetColumnOrderArray(state.Count, state.Order.get());
		return true;
	}

	ColumnsState SaveState(HWND h, bool names = true) {
		CListViewCtrl lv(h);
		ColumnsState state;
		auto si = GetSortInfo(h);
		auto header = lv.GetHeader();
		auto count = header.GetItemCount();
		state.Order = std::make_unique<int[]>(count);
		state.Columns = std::make_unique<LVCOLUMN[]>(count);
		state.Tags = std::make_unique<int[]>(count);
		if (names)
			state.Text = std::make_unique<std::wstring[]>(count);
		if (si) {
			state.SortColumn = si->SortColumn;
			state.SortAscending = si->SortAscending;
		}
		lv.GetColumnOrderArray(count, state.Order.get());
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_MINWIDTH | (names ? LVCF_TEXT : 0);
		state.Count = count;
		WCHAR text[128];
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		for (int i = 0; i < count; i++) {
			state.Columns[i].mask = lvc.mask;
			if (names) {
				state.Columns[i].cchTextMax = _countof(text);
				state.Columns[i].pszText = text;
			}
			lv.GetColumn(i, &state.Columns[i]);
			if (names)
				state.Text[i] = state.Columns[i].pszText;
			header.GetItem(i, &hdi);
			state.Tags[i] = (int)hdi.lParam;
		}
		return state;
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
	mutable std::vector<std::unique_ptr<ColumnManager>> m_Columns;
};