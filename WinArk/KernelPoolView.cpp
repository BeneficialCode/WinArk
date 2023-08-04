#include "stdafx.h"
#include "KernelPoolView.h"
#include <unordered_set>

#pragma comment(lib,"ntdll")

void CKernelPoolView::LoadPoolTagText() {
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

ULONG CKernelPoolView::UpdateSystemPoolTags() {
	ULONG size = 1 << 22;
	if (m_PoolTags == nullptr) {
		LoadPoolTagText();
		m_PoolTags = static_cast<SYSTEM_POOLTAG_INFORMATION*>(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_PoolTags == nullptr) {
			AtlMessageBox(m_hWnd, L"Not enough memory", IDR_MAINFRAME, MB_ICONERROR);
			return 0;
		}
	}
	ULONG len;

	auto status = NtQuerySystemInformation(SystemPoolTagInformation,
		m_PoolTags,
		size, &len
	);

	if (!NT_SUCCESS(status)) {
		CString msg;
		msg.Format(L"Failed in getting pool information. status: 0x%x", status);
		AtlMessageBox(m_hWnd, msg.GetString(), IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}

	auto count = m_PoolTags->Count;
	m_Tags.reserve(count + 16);
	m_TagsMap.reserve(count + 16);

	for (decltype(count) i = 0; i < count; i++) {
		const auto& info = m_PoolTags->TagInfo[i];
		m_TotalPaged += info.PagedUsed;
		m_TotalNonPaged += info.NonPagedUsed;
		AddTag(info, i);
	}
	return count;
}

ULONG CKernelPoolView::UpdateSessionPoolTags(ULONG sessionId) {
	ULONG count = 0;
	ULONG size = 1 << 22;
	if (m_SessionPoolTags == nullptr) {
		LoadPoolTagText();
		m_SessionPoolTags = static_cast<SYSTEM_SESSION_POOLTAG_INFORMATION*>(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_SessionPoolTags == nullptr) {
			AtlMessageBox(m_hWnd, L"Not enough memory", IDR_MAINFRAME, MB_ICONERROR);
			return 0;
		}
	}
	ULONG len;
	SYSTEM_SESSION_PROCESS_INFORMATION procInfo;
	procInfo.SessionId = sessionId;
	procInfo.Buffer = m_SessionPoolTags;
	procInfo.SizeOfBuf = size;
	auto status = NtQuerySystemInformation(SystemSessionPoolTagInformation,
		&procInfo,
		sizeof(SYSTEM_SESSION_PROCESS_INFORMATION), &len
	);

	if (!NT_SUCCESS(status)) {
		if (status == 0xC0000003) {
			return 0;
		}
		CString msg;
		msg.Format(L"Failed in getting pool information. status: 0x%x", status);
		AtlMessageBox(m_hWnd, msg.GetString(), IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}

	count = m_SessionPoolTags->Count;
	for (decltype(count) i = 0; i < count; i++) {
		const auto& info = m_SessionPoolTags->TagInfo[i];
		m_TotalPaged += info.PagedUsed;
		m_TotalNonPaged += info.NonPagedUsed;
		AddTag(info, i);
	}
	return count;
}

void CKernelPoolView::UpdatePoolTags() {
	ULONG count = 0;
	m_Tags.clear();
	m_TagsMap.clear();
	m_TotalPaged = m_TotalNonPaged = 0;
	count = UpdateSystemPoolTags();
	count += UpdateSessionPoolTags(0);
	count += UpdateSessionPoolTags(1);
	if (m_Tags.empty()) {
		SetItemCount(count);
	}
	else {
		SetItemCountEx(count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
		UpdateVisible();
	}
	UpdatePaneText();
}

void CKernelPoolView::AddTag(const SYSTEM_POOLTAG& info, int index) {
	char tag[5] = { 0 };
	::CopyMemory(tag, &info.Tag, 4);
	auto item = std::make_shared<TagItem>();
	item->Tag = CStringA(tag);

	item->TagInfo = info;
	item->Index = index;

	if (auto it = m_TagSource.find(item->Tag); it != m_TagSource.end()) {
		item->SourceName = it->second.first;
		item->SourceDesc = it->second.second;
	}

	m_Tags.push_back(item);
	m_TagsMap.insert({ info.TagUlong, item });
}

void CKernelPoolView::UpdateVisible() {
	int page = GetCountPerPage();
	int start = GetTopIndex();
	RedrawItems(start, start + page);
}

BOOL CKernelPoolView::PreTranslateMessage(MSG* pMsg) {

	return FALSE;
}

DWORD CKernelPoolView::OnPrePaint(int id, LPNMCUSTOMDRAW cd) {
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CKernelPoolView::OnItemPrePaint(int id, LPNMCUSTOMDRAW cd) {
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CKernelPoolView::OnSubItemPrePaint(int id, LPNMCUSTOMDRAW cd) {
	auto lcd = (LPNMLVCUSTOMDRAW)cd;
	int row = static_cast<int>(cd->dwItemSpec);
	int col = lcd->iSubItem;
	lcd->clrTextBk = CLR_INVALID;

	const auto& item = m_Tags[row];
	CellColorKey key(item->TagInfo.TagUlong, col);
	if (auto it = m_CellColors.find(key); it != m_CellColors.end()) {
		auto& cc = it->second;
		if (cc.TargetTime > 0 && cc.TargetTime < ::GetTickCount64())
			m_CellColors.erase(key);
		else
			lcd->clrTextBk = it->second.BackColor;
	}

	return CDRF_DODEFAULT;
}

BOOL CKernelPoolView::OnIdle() {

	return FALSE;
}

LRESULT CKernelPoolView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERINALLVIEWS | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER);

	InsertColumn(ColumnType::TagName, L"Tag", LVCFMT_CENTER, 80);
	InsertColumn(ColumnType::PagedAllocs, L"Paged Allocs", LVCFMT_RIGHT, 100);
	InsertColumn(ColumnType::PagedFrees, L"Paged Frees", LVCFMT_RIGHT, 100);
	InsertColumn(ColumnType::PagedDiff, L"Paged Diff", LVCFMT_RIGHT, 80);
	InsertColumn(ColumnType::PagedUsage, L"Paged Usage", LVCFMT_RIGHT, 100);

	InsertColumn(ColumnType::NonPagedAllocs, L"NPaged Allocs", LVCFMT_RIGHT, 120);
	InsertColumn(ColumnType::NonPagedFrees, L"NPaged Frees", LVCFMT_RIGHT, 120);
	InsertColumn(ColumnType::NonPagedDiff, L"NPaged Diff", LVCFMT_RIGHT, 100);
	InsertColumn(ColumnType::NonPagedUsage, L"NPaged Usage", LVCFMT_RIGHT, 100);

	InsertColumn(ColumnType::SourceName, L"Source", LVCFMT_LEFT, 150);
	InsertColumn(ColumnType::SourceDescription, L"Source Description", LVCFMT_LEFT, 350);

	UpdatePoolTags();

	SetTimer(1, m_UpdateInterval, nullptr);

	auto pLoop = _Module.GetMessageLoop();
	pLoop->AddIdleHandler(this);

	return 0;
}

void CKernelPoolView::UpdatePaneText() {
	if (IsWindowVisible()) {
		CString text;
		text.Format(L"%d Tags", GetItemCount());
		m_pFrame->SetPaneText(2, text);
		text.Format(L"Paged: %u MB", (unsigned)(m_TotalPaged >> 20));
		m_pFrame->SetPaneText(3, text);
		text.Format(L"Non Paged: %u MB", (unsigned)(m_TotalNonPaged >> 20));
		m_pFrame->SetPaneText(4, text);
	}
}

LRESULT CKernelPoolView::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&) {
	if (wParam == 1) {
		// UpdatePoolTags();
		UpdatePaneText();
	}

	return 0;
}

LRESULT CKernelPoolView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (m_PoolTags)
		::VirtualFree(m_PoolTags, 0, MEM_RELEASE);

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveIdleHandler(this);
	bHandled = FALSE;

	return 1;
}

LRESULT CKernelPoolView::OnGetDisplayInfo(int, LPNMHDR nmhdr, BOOL&) {
	auto disp = (NMLVDISPINFO*)nmhdr;
	auto& item = disp->item;
	auto index = item.iItem;
	auto& info = *m_Tags[index];

	if (disp->item.mask & LVIF_TEXT) {
		switch (disp->item.iSubItem) {
			case ColumnType::TagName:
				StringCchCopyW(item.pszText, item.cchTextMax, CString(info.Tag));
				break;

			case ColumnType::PagedAllocs:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.PagedAllocs);
				break;

			case ColumnType::PagedFrees:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.PagedFrees);
				break;

			case ColumnType::PagedDiff:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.PagedAllocs - info.TagInfo.PagedFrees);
				break;

			case ColumnType::PagedUsage:
			{
				auto value = info.TagInfo.PagedUsed;
				if (value < 1 << 12)
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld B", value);
				else
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld KB", value >> 10);

				break;
			}

			case ColumnType::NonPagedAllocs:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.NonPagedAllocs);
				break;

			case ColumnType::NonPagedFrees:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.NonPagedFrees);
				break;

			case ColumnType::NonPagedDiff:
				StringCchPrintf(item.pszText, item.cchTextMax, L"%u", info.TagInfo.NonPagedAllocs - info.TagInfo.NonPagedFrees);
				break;

			case ColumnType::NonPagedUsage:
			{
				auto value = info.TagInfo.NonPagedUsed;
				if (value < 1 << 12)
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld B", value);
				else
					StringCchPrintf(item.pszText, item.cchTextMax, L"%ld KB", value >> 10);
				break;
			}

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

LRESULT CKernelPoolView::OnColumnClick(int, LPNMHDR nmhdr, BOOL&) {
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

LRESULT CKernelPoolView::OnItemChanged(int, LPNMHDR, BOOL&) {

	return 0;
}

LRESULT CKernelPoolView::OnFindItem(int, LPNMHDR nmhdr, BOOL&) {
	auto fi = (NMLVFINDITEM*)nmhdr;
	auto start = GetSelectedIndex() + 1;
	auto count = GetItemCount();
	CString text(fi->lvfi.psz);

	if (fi->lvfi.flags & LVFI_STRING) {
		for (int i = start; i < start + count; i++) {
			if (CString(m_Tags[i % count]->Tag).Mid(0, text.GetLength()).CompareNoCase(text) == 0) {
				return i % count;
			}
		}
	}

	return -1;
}

bool CKernelPoolView::CompareItems(const TagItem& item1, const TagItem& item2) {
	int result;
	switch (m_SortColumn) {
		case ColumnType::TagName:
			result = item2.Tag.CompareNoCase(item1.Tag);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::SourceName:
			result = ::_wcsicmp(item2.SourceName, item1.SourceName);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::SourceDescription:
			result = ::_wcsicmp(item2.SourceDesc, item1.SourceDesc);
			return m_Ascending ? (result > 0) : (result < 0);

		case ColumnType::PagedAllocs:
			if (m_Ascending)
				return item1.TagInfo.PagedAllocs < item2.TagInfo.PagedAllocs;
			else
				return item1.TagInfo.PagedAllocs > item2.TagInfo.PagedAllocs;

		case ColumnType::PagedFrees:
			if (m_Ascending)
				return item1.TagInfo.PagedFrees < item2.TagInfo.PagedFrees;
			else
				return item1.TagInfo.PagedFrees > item2.TagInfo.PagedFrees;

		case ColumnType::PagedUsage:
			if (m_Ascending)
				return item1.TagInfo.PagedUsed < item2.TagInfo.PagedUsed;
			else
				return item1.TagInfo.PagedUsed > item2.TagInfo.PagedUsed;

		case ColumnType::PagedDiff:
			if (m_Ascending)
				return item1.TagInfo.PagedAllocs - item1.TagInfo.PagedFrees < item2.TagInfo.PagedAllocs - item2.TagInfo.PagedFrees;
			else
				return item1.TagInfo.PagedAllocs - item1.TagInfo.PagedFrees > item2.TagInfo.PagedAllocs - item2.TagInfo.PagedFrees;

		case ColumnType::NonPagedAllocs:
			if (m_Ascending)
				return item1.TagInfo.NonPagedAllocs < item2.TagInfo.NonPagedAllocs;
			else
				return item1.TagInfo.NonPagedAllocs > item2.TagInfo.NonPagedAllocs;

		case ColumnType::NonPagedFrees:
			if (m_Ascending)
				return item1.TagInfo.NonPagedFrees < item2.TagInfo.NonPagedFrees;
			else
				return item1.TagInfo.NonPagedFrees > item2.TagInfo.NonPagedFrees;

		case ColumnType::NonPagedDiff:
			if (m_Ascending)
				return item1.TagInfo.NonPagedAllocs - item1.TagInfo.NonPagedFrees < item2.TagInfo.NonPagedAllocs - item2.TagInfo.NonPagedFrees;
			else
				return item1.TagInfo.NonPagedAllocs - item1.TagInfo.NonPagedFrees > item2.TagInfo.NonPagedAllocs - item2.TagInfo.NonPagedFrees;

		case ColumnType::NonPagedUsage:
			if (m_Ascending)
				return item1.TagInfo.NonPagedUsed < item2.TagInfo.NonPagedUsed;
			else
				return item1.TagInfo.NonPagedUsed > item2.TagInfo.NonPagedUsed;

	}

	ATLASSERT(false);
	return false;
}

void CKernelPoolView::DoSort() {
	std::sort(m_Tags.begin(), m_Tags.end(), [this](const auto& i1, const auto& i2) {
		return CompareItems(*i1, *i2);
		});
}

void CKernelPoolView::SetToolBar(HWND hWnd) {

}

void CKernelPoolView::AddCellColor(CellColor& cell, DWORD64 targetTime) {
	cell.TargetTime = targetTime;
	m_CellColors.insert({ cell, cell });
}

void CKernelPoolView::RemoveCellColor(const CellColorKey& key) {
	m_CellColors.erase(key);
}

int CKernelPoolView::GetChange(const SYSTEM_POOLTAG& info, const SYSTEM_POOLTAG& newinfo, ColumnType type) const {
	static const auto compare = [](const auto v1, const auto v2) -> int {
		if (v1 > v2)
			return -1;
		if (v2 > v1)
			return 1;
		return 0;
	};

	switch (type) {
		case ColumnType::NonPagedAllocs: return compare(info.NonPagedAllocs, newinfo.NonPagedAllocs);
		case ColumnType::NonPagedFrees: return compare(info.NonPagedFrees, newinfo.NonPagedFrees);
		case ColumnType::NonPagedDiff: return compare(info.NonPagedAllocs - info.NonPagedFrees, newinfo.NonPagedAllocs - newinfo.NonPagedFrees);
		case ColumnType::NonPagedUsage: return compare(info.NonPagedUsed, newinfo.NonPagedUsed);

		case ColumnType::PagedAllocs: return compare(info.PagedAllocs, newinfo.PagedAllocs);
		case ColumnType::PagedFrees: return compare(info.PagedFrees, newinfo.PagedFrees);
		case ColumnType::PagedDiff: return compare(info.PagedAllocs - info.PagedFrees, newinfo.PagedAllocs - newinfo.PagedFrees);
		case ColumnType::PagedUsage: return compare(info.PagedUsed, newinfo.PagedUsed);
	}
	return 0;
}

LRESULT CKernelPoolView::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdatePoolTags();
	return 0;
}

LRESULT CKernelPoolView::OnRBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(2);
	POINT pt;
	::GetCursorPos(&pt);

	auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
	if (id) {
		PostMessage(WM_COMMAND, id);
	}


	return 0;
}

void CKernelPoolView::DoFind(const CString& text, DWORD flags) {
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
		const auto& item = m_Tags[index];
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


