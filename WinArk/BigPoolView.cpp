#include "stdafx.h"
#include "BigPoolView.h"
#include <unordered_set>

#pragma comment(lib,"ntdll")

void CBigPoolView::LoadPoolTagText() {
	auto hModule = _Module.GetResourceInstance();
	auto hResource = ::FindResource(hModule, MAKEINTRESOURCE(IDR_POOLTAG), L"TXT");
	ATLASSERT(hResource);

	auto hGlobal = ::LoadResource(hModule, hResource);
	ATLASSERT(hGlobal);

	auto data = static_cast<const char*>(::LockResource(hGlobal));
	auto next = strchr(data, '\n');
	for (; next; data = next + 1) {
		if (strncmp(data, "//", 2) == 0 || _strnicmp(data, "rem", 3) == 0
			|| strncmp(data, "\r\n", 2) == 0 || strncmp(data, "\n", 1) == 0) {
			next = strchr(data, '\n');
			continue;
		}

		next = strchr(data, '\n');

		// read the tag
		std::string tag(data, data + 4);

		// locate the first dash
		auto dash1 = strchr(data, '-');
		if (dash1 == nullptr)
			continue;

		// locate second dash
		auto dash2 = strchr(dash1 + 1, '-');
		if (dash2 == nullptr)
			continue;

		if (dash2 > next) {
			dash2 = dash1;
			dash1 = nullptr;
		}

		CStringA trimmedTag(tag.c_str());
		trimmedTag.TrimLeft();
		trimmedTag += L"  ";
		if (trimmedTag.GetLength() > 4)
			trimmedTag = trimmedTag.Mid(0, 4);
		CStringW trimmedDriverName(L"");
		if (dash1) {
			std::string driverName(dash1 + 1, dash2);
			trimmedDriverName = driverName.c_str();
			trimmedDriverName.Trim();
		}

		std::string driverDesc(dash2 + 1, next - 1);
		CStringW trimmedDriverDesc(driverDesc.c_str());
		trimmedDriverDesc.Trim();

		m_TagSource.insert({ trimmedTag, std::make_pair(CString(trimmedDriverName), CString(trimmedDriverDesc)) });
	}
}

void CBigPoolView::UpdateBigPools() {
	ULONG size = 1 << 22;
	if (m_BigPools == nullptr) {
		LoadPoolTagText();
		m_BigPools = static_cast<SYSTEM_BIGPOOL_INFORMATION*>(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_BigPools == nullptr) {
			AtlMessageBox(m_hWnd, L"Not enough memory", IDR_MAINFRAME, MB_ICONERROR);
			return;
		}
	}
	ULONG len;
	auto status = NtQuerySystemInformation(SystemBigPoolInformation, m_BigPools, size, &len);
	if (status) {
		CString msg;
		msg.Format(L"Failed in getting pool information. status: 0x%x", status);
		AtlMessageBox(m_hWnd, msg.GetString(), IDR_MAINFRAME, MB_ICONERROR);
		return;
	}

	auto count = m_BigPools->Count;
	if (m_Pools.empty()) {
		m_Pools.reserve(count + 16);
		m_PoolsMap.reserve(count + 16);

		for (decltype(count) i = 0; i < count; i++) {
			const auto& info = m_BigPools->AllocateInfo[i];
			if (info.NonPaged) {
				m_TotalNonPaged += info.SizeInBytes;
			}
			else {
				m_TotalPaged += info.SizeInBytes;
			}
			AddTag(info, i);
		}
		SetItemCount(count);
	}
	else {
		int size = static_cast<int>(m_Pools.size());
		std::unordered_set<int> set;
		for (int i = 0; i < size; i++)
			set.insert(i);

		m_TotalPaged = m_TotalNonPaged = 0;
		for (decltype(count) i = 0; i < count; i++) {
			const auto& info = m_BigPools->AllocateInfo[i];
			if (info.NonPaged) {
				m_TotalNonPaged += info.SizeInBytes;
			}
			else {
				m_TotalPaged += info.SizeInBytes;
			}
			auto it = m_PoolsMap.find(info.TagUlong);
			if (it == m_PoolsMap.end()) {
				// new tag
				AddTag(info, i);
				count++;
			}
			else {
				auto& newinfo = it->second->BigPoolInfo;

				for (int col = 0; col < (int)ColumnType::NumColumns; col++) {
					auto change = GetChange(newinfo, info, (ColumnType)col);
					if (change) {
						CellColor cell(info.TagUlong, col);
						cell.BackColor = change > 0 ? RGB(0, 240, 0) : RGB(255, 64, 0);
						AddCellColor(cell, ::GetTickCount() + 2000);
					}
				}
				it->second->BigPoolInfo = info;
				it->second->Index = i;
				set.erase(i);
			}
		}

		int bias = 0;
		for (auto index : set) {
			int i = index - bias;
			count--;
			m_PoolsMap.erase(m_Pools[i]->BigPoolInfo.TagUlong);
			m_Pools.erase(m_Pools.begin() + i);
			bias++;
		}

		SetItemCountEx(count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

		UpdateVisible();
	}
	UpdatePaneText();
}

void CBigPoolView::AddTag(const SYSTEM_BIGPOOL_ENTRY& info, int index) {
	char tag[5] = { 0 };
	::CopyMemory(tag, &info.Tag, 4);
	auto item = std::make_shared<BigPoolItem>();
	item->Tag = CStringA(tag);
	item->Index = index;
	item->BigPoolInfo = info;

	if(auto it = m_TagSource.find(item->Tag); it != m_TagSource.end()) {
		item->SourceName = it->second.first;
		item->SourceDesc = it->second.second;
	}

	m_Pools.push_back(item);
	m_PoolsMap.insert({ info.TagUlong,item });
}

void CBigPoolView::UpdateVisible() {
	int page = GetCountPerPage();
	int start = GetTopIndex();
	RedrawItems(start, start + page);
}

BOOL CBigPoolView::PreTranslateMessage(MSG* pMsg) {
	return FALSE;
}

DWORD CBigPoolView::OnPrePaint(int id, LPNMCUSTOMDRAW cd) {
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CBigPoolView::OnItemPrePaint(int id, LPNMCUSTOMDRAW cd) {
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CBigPoolView::OnSubItemPrePaint(int id, LPNMCUSTOMDRAW cd) {
	auto lcd = (LPNMLVCUSTOMDRAW)cd;
	int row = static_cast<int>(cd->dwItemSpec);
	int col = lcd->iSubItem;
	lcd->clrTextBk = CLR_INVALID;

	const auto& item = m_Pools[row];
	CellColorKey key(item->BigPoolInfo.TagUlong, col);
	if (auto it = m_CellColors.find(key); it != m_CellColors.end()) {
		auto& cc = it->second;
		if (cc.TargetTime > 0 && cc.TargetTime < ::GetTickCount64())
			m_CellColors.erase(key);
		else
			lcd->clrTextBk = it->second.BackColor;
	}

	return CDRF_DODEFAULT;
}

BOOL CBigPoolView::OnIdle() {
	return FALSE;
}

LRESULT CBigPoolView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERINALLVIEWS | LVS_EX_HEADERDRAGDROP |
		LVS_EX_DOUBLEBUFFER);

	InsertColumn(ColumnType::TagName, L"Tag", LVCFMT_CENTER, 80);
	InsertColumn(ColumnType::VirtualAddress, L"Virtual Address", LVCFMT_RIGHT, 150);
	InsertColumn(ColumnType::NonPaged, L"NonPaged", LVCFMT_RIGHT,80);
	InsertColumn(ColumnType::Size, L"Size", LVCFMT_RIGHT, 80);
	InsertColumn(ColumnType::SourceName, L"Source", LVCFMT_LEFT, 150);
	InsertColumn(ColumnType::SourceDescription, L"Source Description", LVCFMT_LEFT, 350);

	UpdateBigPools();

	//SetTimer(1, m_UpdateInterval, nullptr);

	auto pLoop = _Module.GetMessageLoop();
	pLoop->AddIdleHandler(this);

	return 0;
}

LRESULT CBigPoolView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (wParam == 1)
		UpdateBigPools();

	return 0;
}

LRESULT CBigPoolView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (m_BigPools)
		::VirtualFree(m_BigPools, 0, MEM_RELEASE);

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveIdleHandler(this);
	bHandled = FALSE;

	return 1;
}

LRESULT CBigPoolView::OnGetDisplayInfo(int, LPNMHDR nmhdr, BOOL&) {
	auto disp = (NMLVDISPINFO*)nmhdr;
	auto& item = disp->item;
	auto index = item.iItem;
	auto& info = *m_Pools[index];

	if (disp->item.mask & LVIF_TEXT) {
		switch (disp->item.iSubItem)
		{
			case ColumnType::TagName:
				StringCchCopy(item.pszText, item.cchTextMax, CString(info.Tag));
				break;

			case ColumnType::VirtualAddress:
				StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", info.BigPoolInfo.VirtualAddress);
				break;
			
			case ColumnType::Size:
			{
				auto value = info.BigPoolInfo.SizeInBytes;
				if (value < 1 << 12)
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld B", value);
				else
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld KB", value >> 10);
				break;
			}

			case ColumnType::NonPaged:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.BigPoolInfo.NonPaged);
				break;

			case ColumnType::SourceName:
				item.pszText = (PWSTR)info.SourceName;
				break;

			case ColumnType::SourceDescription:
				item.pszText = (PWSTR)info.SourceDesc;
				break;
		}
	}

	return 0;
}

LRESULT CBigPoolView::OnColumnClick(int, LPNMHDR nmhdr, BOOL&) {
	auto lv = (NMLISTVIEW*)nmhdr;
	int column = lv->iSubItem;
	int oldSortColumn = m_SortColumn;
	if (column == m_SortColumn)
		m_Ascending = !m_Ascending;
	else {
		m_SortColumn = column;
		m_Ascending = true;
	}

	HDITEM h;
	h.mask = HDI_FORMAT;
	auto header = GetHeader();
	header.GetItem(m_SortColumn, &h);
	h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING | (m_Ascending ? HDF_SORTUP : HDF_SORTDOWN);
	header.SetItem(m_SortColumn, &h);

	if (oldSortColumn >= 0 && oldSortColumn != m_SortColumn) {
		h.mask = HDI_FORMAT;
		header.GetItem(oldSortColumn, &h);
		h.fmt = (h.fmt & HDF_JUSTIFYMASK) | HDF_STRING;
		header.SetItem(oldSortColumn, &h);
	}

	DoSort();
	UpdateVisible();

	return 0;
}

LRESULT CBigPoolView::OnItemChanged(int, LPNMHDR, BOOL&) {

	return 0;
}

LRESULT CBigPoolView::OnFindItem(int, LPNMHDR nmhdr, BOOL&) {
	auto fi = (NMLVFINDITEM*)nmhdr;
	auto start = GetSelectedIndex() + 1;
	auto count = GetItemCount();
	CString text(fi->lvfi.psz);

	if (fi->lvfi.flags & LVFI_STRING) {
		for (int i = start; i < start + count; i++) {
			if (CString(m_Pools[i % count]->Tag).Mid(0, text.GetLength()).CompareNoCase(text) == 0) {
				return i % count;
			}
		}
	}

	return -1;
}

bool CBigPoolView::CompareItems(const BigPoolItem& item1, const BigPoolItem& item2) {
	int result;
	switch (m_SortColumn)
	{
		case ColumnType::TagName:
			result = item2.Tag.CompareNoCase(item1.Tag);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::SourceName:
			result = ::_wcsicmp(item2.SourceName, item1.SourceName);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::SourceDescription:
			result = ::_wcsicmp(item2.SourceDesc, item1.SourceDesc);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::NonPaged:
			if (m_Ascending)
				return item1.BigPoolInfo.NonPaged < item2.BigPoolInfo.NonPaged;
			else
				return item1.BigPoolInfo.NonPaged > item2.BigPoolInfo.NonPaged;
		case ColumnType::VirtualAddress:
			if (m_Ascending)
				return item1.BigPoolInfo.VirtualAddress < item2.BigPoolInfo.VirtualAddress;
			else
				return item1.BigPoolInfo.VirtualAddress > item2.BigPoolInfo.VirtualAddress;
		case ColumnType::Size:
			if (m_Ascending)
				return item1.BigPoolInfo.SizeInBytes < item2.BigPoolInfo.SizeInBytes;
			else
				return item1.BigPoolInfo.SizeInBytes > item2.BigPoolInfo.SizeInBytes;
	}
	ATLASSERT(false);
	return false;
}

void CBigPoolView::DoSort() {
	std::sort(m_Pools.begin(), m_Pools.end(), [this](const auto& i1, const auto& i2) {
		return CompareItems(*i1, *i2);
		});
}

void CBigPoolView::SetToolBar(HWND hWnd) {
}

void CBigPoolView::AddCellColor(CellColor& cell, DWORD64 targetTime) {
	cell.TargetTime = targetTime;
	m_CellColors.insert({ cell, cell });
}

void CBigPoolView::RemoveCellColor(const CellColorKey& key) {
	m_CellColors.erase(key);
}

int CBigPoolView::GetChange(const SYSTEM_BIGPOOL_ENTRY& info, const SYSTEM_BIGPOOL_ENTRY& newinfo, ColumnType type) const {
	static const auto compare = [](const auto v1, const auto v2) -> int {
		if (v1 > v2)
			return -1;
		if (v2 > v1)
			return 1;
		return 0;
	};

	switch (type)
	{
		case ColumnType::VirtualAddress:
			return compare(info.VirtualAddress, info.VirtualAddress);
		case ColumnType::Size:
			return compare(info.SizeInBytes, info.SizeInBytes);
		case ColumnType::NonPaged:
			return compare(info.NonPaged, info.NonPaged);
	}
	return 0;
}

LRESULT CBigPoolView::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateBigPools();
	return 0;
}

void CBigPoolView::UpdatePaneText() {
	if (IsWindowVisible()) {
		CString text;
		text.Format(L"%d BigPools", GetItemCount());
		m_pFrame->SetPaneText(2, text);
		text.Format(L"Paged: %u MB", (unsigned)(m_TotalPaged >> 20));
		m_pFrame->SetPaneText(3, text);
		text.Format(L"Non Paged: %u MB", (unsigned)(m_TotalNonPaged >> 20));
		m_pFrame->SetPaneText(4, text);
	}
}

LRESULT CBigPoolView::OnRBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(3);
	POINT pt;
	::GetCursorPos(&pt);

	auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
	if (id) {
		PostMessage(WM_COMMAND, id);
	}


	return 0;
}

void CBigPoolView::DoFind(const CString& text, DWORD flags) {
	auto searchDown = flags & FR_DOWN;

	int start = GetSelectedIndex();
	CString find(text);
	auto ignoreCase = !(flags & FR_MATCHCASE);
	if (ignoreCase)
		find.MakeLower();

	int from = searchDown ? start + 1 : start - 1 + GetItemCount();
	int to = searchDown ? GetItemCount() + start : start + 1;
	int step = searchDown ? 1 : -1;

	int findIndex = -1;
	for (int i = from; i != to; i += step) {
		int index = i % GetItemCount();
		const auto& item = m_Pools[index];
		CString text(item->Tag);
		if (ignoreCase)
			text.MakeLower();
		if (text.Find(find) >= 0) {
			findIndex = index;
			break;
		}
		text = item->SourceName;
		if (ignoreCase)
			text.MakeLower();
		if (text.Find(find) >= 0) {
			findIndex = index;
			break;
		}
	}

	if (findIndex >= 0) {
		SelectItem(findIndex);
	}
	else
		AtlMessageBox(m_hWnd, L"Not found");
}