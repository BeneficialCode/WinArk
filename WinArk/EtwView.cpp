#include "stdafx.h"
#include "EtwView.h"
#include "resource.h"
#include "TraceManager.h"
#include "FormatHelper.h"
#include <execution>
#include "ClipboardHelper.h"
#include "SortHelper.h"

CEtwView::CEtwView(IEtwFrame* frame) :CViewBase(frame) {
}

void CEtwView::Activate(bool active) {
	m_IsActive = active;
	if (active) {
		UpdateEventStatus();
		UpdateUI();
	}
}

void CEtwView::AddEvent(std::shared_ptr<EventData> data) {
	{
		std::lock_guard lock(m_EventsLock);
		m_TempEvents.push_back(data);
	}
}

void CEtwView::StartMonitoring(TraceManager& tm, bool start) {
	if (start) {
		std::vector<KernelEventTypes> types;
		std::vector<std::wstring> categories;
		for (auto& cat : m_EventsConfig.GetCategories()) {
			auto c = KernelEventCategory::GetCategory(cat.Name.c_str());
			ATLASSERT(c);
			types.push_back(c->EnableFlag);
			categories.push_back(c->Name);
		}
		std::initializer_list<KernelEventTypes> events(types.data(), types.data() + types.size());
		tm.SetKernelEventTypes(events);
		tm.SetKernelEventStacks(std::initializer_list<std::wstring>(categories.data(), categories.data() + categories.size()));
		ApplyFilters(m_FilterConfig);
	}
	else {
		m_IsDraining = true;
	}
	m_IsMonitoring = start;
}

CString CEtwView::GetColumnText(HWND, int row, int col) const {
	auto item = m_Events[row].get();

	CString text;
	switch (col)
	{
		case 0:// index
			text.Format(L"%7u", item->GetIndex());
			if (item->GetStackEventData())
				text += L" *";
			break;

		case 1:// Time
		{
			auto ts = item->GetTimeStamp();
			return FormatHelper::FormatTime(ts);
		}

		case 3:// PID
		{
			auto pid = item->GetProcessId();
			if (pid != (DWORD)-1)
				text.Format(L"%u (0x%X)", pid, pid);
			break;
		}

		case 5:// TID
		{
			auto tid = item->GetThreadId();
			if (tid != (DWORD)-1 && tid != 0)
				text.Format(L"%u (0x%X)", tid, tid);
			break;
		}

		case 6:// Details
			return GetEventDetails(item).c_str();
	}

	return text;
}

int CEtwView::GetRowImage(HWND, int row) const {
	auto& evt = m_Events[row];
	if (auto it = s_IconsMap.find(evt->GetEventName()); it != s_IconsMap.end())
		return it->second;

	auto pos = evt->GetEventName().find(L'/');
	if (pos != std::wstring::npos) {
		if (auto it = s_IconsMap.find(evt->GetEventName().substr(0, pos)); it != s_IconsMap.end())
			return it->second;
	}
	return 0;
}

PCWSTR CEtwView::GetColumnTextPointer(HWND, int row, int col) const {
	auto item = m_Events[row].get();
	switch (col)
	{
		case 2: return item->GetEventName().c_str();
		case 4: return item->GetProcessName().c_str();
	}
	return nullptr;
}

bool CEtwView::OnRightClickList(HWND, int index, int col, POINT& pt) {
	if (index >= 0) {
		CMenu menu;
		menu.LoadMenu(IDR_ETW_CONTEXT);
		GetFrame()->TrackPopupMenu(menu.GetSubMenu(0), *this, &pt);
		return true;
	}
	return false;
}

bool CEtwView::OnDoubleClickList(HWND, int row, int col, POINT& pt) {
	SendMessage(WM_COMMAND, ID_EVENT_PROPERTIES);
	return true;
}

std::wstring CEtwView::ProcessSpecialEvent(EventData* data) const {
	std::wstring details;
	CString text;
	auto& name = data->GetEventName();
	if (name == L"Process/Start") {
		text.Format(L"PID: %u; Image: %s; Command Line: %s",
			data->GetProperty(L"ProcessId")->GetValue<DWORD>(),
			CString(data->GetProperty(L"ImageFileName")->GetAnsiString()),
			data->GetProperty(L"CommandLine")->GetUnicodeString());
		details = std::move(text);
	}
	return details;
}

std::wstring CEtwView::GetEventDetails(EventData* data) const {
	auto details = ProcessSpecialEvent(data);
	if (details.empty()) {
		for (auto& prop : data->GetProperties()) {
			if (prop.Name.substr(0, 8) != L"Reserved" && prop.Name.substr(0, 4) != L"TTID") {
				auto value = FormatHelper::FormatProperty(data, prop);
				if (value.empty())
					value = data->FormatProperty(prop);
				if (!value.empty()) {
					if (value.size() > 102)
						value = value.substr(0, 100) + L"...";
					details += prop.Name + L": " + value + L";";
				}
			}
		}
	}
	return details;
}

void CEtwView::UpdateEventStatus() {
	CString text;
	text.Format(L"Events: %u", (uint32_t)m_Events.size());
	GetFrame()->SetPaneText(2, text);
}

bool CEtwView::IsSortable(HWND, int col) const {
	return col != 8 && (!m_IsMonitoring || GetFrame()->GetTraceManager().IsPaused());
}

void CEtwView::DoSort(const SortInfo* si) {
	auto compare = [&](auto& i1, auto& i2) {
		switch (si->SortColumn)
		{
			case 0: return SortHelper::SortNumbers(i1->GetIndex(), i2->GetIndex(), si->SortAscending);
			case 1: return SortHelper::SortNumbers(i1->GetTimeStamp(), i2->GetTimeStamp(), si->SortAscending);
			case 2: return SortHelper::SortStrings(i1->GetEventName(), i2->GetEventName(), si->SortAscending);
			case 3: return SortHelper::SortNumbers(i1->GetProcessId(), i2->GetProcessId(), si->SortAscending);
			case 4: return SortHelper::SortStrings(i1->GetProcessName(), i2->GetProcessName(), si->SortAscending);
			case 5: return SortHelper::SortNumbers(i1->GetThreadId(), i2->GetThreadId(), si->SortAscending);
			case 6: return SortHelper::SortNumbers(i1->GetEventDescriptor().Opcode, i2->GetEventDescriptor().Opcode, si->SortAscending);
		}
		return false;
	};

	if (m_Events.size() < 20000)
		std::sort(m_Events.begin(), m_Events.end(), compare);
	else {
		// C++ 17引入了执行策略
		// std::execution::par使算法在多个线程中执行，并且线程各自具有自己的顺序任务。即并行但不并发。
		// 并行在操作系统中是指，一组程序按独立异步的速度执行，不等于时间上的重叠（同一个时刻发生）。
		// 并发是指：在同一个时间段内，两个或多个程序执行，有时间上的重叠（宏观上是同时，微观上仍是顺序执行）。
		std::sort(std::execution::par, m_Events.begin(), m_Events.end(), compare);
	}
}

BOOL CEtwView::PreTransalteMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

DWORD CEtwView::OnPrePaint(int, LPNMCUSTOMDRAW) {
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CEtwView::OnSubItemPrePaint(int, LPNMCUSTOMDRAW cd) {
	auto lcd = (LPNMLVCUSTOMDRAW)cd;
	auto cm = GetColumnManager(m_List);
	auto sub = cm->GetRealColumn(lcd->iSubItem);
	lcd->clrTextBk = CLR_INVALID;

	if ((cm->GetColumn(sub).Flags & ColumnFlags::Numeric) == ColumnFlags::Numeric)
		::SelectObject(cd->hdc, (HFONT)GetFrame()->GetMonoFont());
	else
		::SelectObject(cd->hdc, m_hFont);

	int index = (int)cd->dwItemSpec;
	if (sub == 8 && (m_List.GetItemState(index, LVIS_SELECTED) & LVIS_SELECTED) == 0) {
		auto& item = m_Events[index];
		auto start = (size_t)0;
		auto details = GetEventDetails(item.get());
		bool bold = false;
		CDCHandle dc(cd->hdc);
		std::wstring str;
		int x = cd->rc.left + 6, y = cd->rc.top, right = cd->rc.right;
		SIZE size;
		for (;;) {
			auto pos = details.find(L";", start);
			if (pos == std::wstring::npos)
				break;
			str = details.substr(start, pos - start);

			auto colon = str.find(L":");
			dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
			dc.GetTextExtent(str.c_str(), (int)colon + 1, &size);
			if (x + size.cx > right)
				break;

			dc.TextOutW(x, y, str.c_str(), (int)colon + 1);
			x += size.cx;

			dc.SetTextColor(RGB(0, 0, 255));
			dc.GetTextExtent(str.data() + colon + 1, (int)str.size() - (int)colon - 1, &size);
			if (x + size.cx > right)
				break;
			dc.TextOutW(x, y, str.data() + colon + 1, (int)str.size() - (int)colon - 1);
			x += size.cx + 4;

			start = pos + 1;
		}
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CEtwView::OnItemPrePaint(int, LPNMCUSTOMDRAW cd) {
	m_hFont = (HFONT)::GetCurrentObject(cd->hdc, OBJ_FONT);
	return CDRF_NOTIFYSUBITEMDRAW;
}

CImageList CEtwView::GetEventImageList() {
	return s_Images;
}

int CEtwView::GetImageFromEventName(PCWSTR name) {
	if (auto it = s_IconsMap.find(name); it != s_IconsMap.end())
		return it->second;

	auto pos = ::wcschr(name, L'/');
	if (pos) {
		if (auto it = s_IconsMap.find(std::wstring(name, pos)); it != s_IconsMap.end())
			return it->second;
	}
	return 0;
}

void CEtwView::OnFinalMessage(HWND) {
	GetFrame()->ViewDestroyed(this);
	delete this;
}

LRESULT CEtwView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_hWndClient = m_List.Create(*this, rcDefault, nullptr,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHAREIMAGELISTS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP |
		LVS_EX_DOUBLEBUFFER);

	auto processes = KernelEventCategory::GetCategory(L"Process");
	ATLASSERT(processes);

	// init event
	EventConfigCategory cat;
	cat.Name = processes->Name;
	m_EventsConfig.AddCategory(cat);

	// init filters
	FilterDescription desc;
	desc.Name = L"Process Id";
	desc.Action = FilterAction::Exclude;
	desc.Parameters = std::to_wstring(::GetCurrentProcessId());
	m_FilterConfig.AddFilter(desc);

	if (s_Images == nullptr) {
		s_Images.Create(16, 16, ILC_COLOR32, 8, 8);
		struct {
			int icon;
			PCWSTR type;
		}icons[] = {
			{IDI_GENERIC,nullptr},
		};

		int index = 0;
		for (auto entry : icons) {
			s_Images.AddIcon(AtlLoadIconImage(entry.icon, 0, 16, 16));
			if (entry.type)
				s_IconsMap.insert({ entry.type,index });
			index++;
		}
	}

	m_List.SetImageList(s_Images, LVSIL_SMALL);

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"#", 0, 100, ColumnFlags::Numeric | ColumnFlags::Visible);
	cm->AddColumn(L"Time", LVCFMT_RIGHT, 180, ColumnFlags::Numeric | ColumnFlags::Visible);
	cm->AddColumn(L"Event", LVCFMT_LEFT, 160);
	cm->AddColumn(L"PID", LVCFMT_RIGHT, 120, ColumnFlags::Numeric | ColumnFlags::Visible);
	cm->AddColumn(L"Process Name", LVCFMT_LEFT, 150);
	cm->AddColumn(L"TID", LVCFMT_RIGHT, 120, ColumnFlags::Numeric | ColumnFlags::Visible);
	cm->AddColumn(L"Details", LVCFMT_LEFT, 700);

	cm->UpdateColumns();

	m_TempEvents.reserve(1 << 12);
	m_Events.reserve(1 << 16);
	SetTimer(1, 1000, nullptr);

	return 0;
}

LRESULT CEtwView::OnDestroy(UINT, WPARAM, LPARAM, BOOL& handled) {
	KillTimer(1);
	handled = FALSE;
	return 0;
}

LRESULT CEtwView::OnForwardMsg(UINT, WPARAM, LPARAM lp, BOOL& handled) {
	auto msg = (MSG*)lp;
	LRESULT result;
	handled = ProcessWindowMessage(msg->hwnd, msg->message, msg->wParam, msg->lParam, result, 1);
	return result;
}

LRESULT CEtwView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (m_IsMonitoring && id == 1) {
		if (!m_TempEvents.empty()) {
			std::lock_guard lock(m_EventsLock);
			m_Events.insert(m_Events.end(), m_TempEvents.begin(), m_TempEvents.end());
			m_OrgEvents.insert(m_OrgEvents.end(), m_TempEvents.begin(), m_TempEvents.end());
			m_TempEvents.clear();
		}
		else if (!m_IsMonitoring) {
			m_IsDraining = false;
		}
		UpdateList(m_List, static_cast<int>(m_Events.size()));
		if (m_AutoScroll)
			m_List.EnsureVisible(m_List.GetItemCount() - 1, FALSE);
		if (m_IsActive)
			UpdateEventStatus();
	}
	return 0;
}

LRESULT CEtwView::OnClear(WORD, WORD, HWND, BOOL&) {
	CWaitCursor wait;
	m_List.SetItemCount(0);
	std::lock_guard lock(m_EventsLock);
	m_Events.clear();
	m_TempEvents.clear();
	m_OrgEvents.clear();
	return 0;
}

LRESULT CEtwView::OnCallStack(WORD, WORD, HWND, BOOL&) {
	auto selected = m_List.GetSelectedIndex();
	if (selected < 0)
		return 0;

	auto data = m_Events[selected].get();
	if (data->GetStackEventData() == nullptr) {
		AtlMessageBox(*this, L"Call stack not available for this event", IDS_TITLE, MB_ICONEXCLAMATION);
		return 0;
	}

	return 0;
}

LRESULT CEtwView::OnEventProperties(WORD, WORD, HWND, BOOL&) {
	auto selected = m_List.GetSelectedIndex();
	if (selected < 0)
		return 0;

	return 0;
}

LRESULT CEtwView::OnClose(UINT, WPARAM, LPARAM, BOOL& handled) {
	if (m_IsMonitoring) {
		AtlMessageBox(nullptr, L"Cannot close tab while monitoring is active", IDS_TITLE, MB_ICONWARNING);
		handled = TRUE;
	}
	handled = FALSE;
	return 0;
}

LRESULT CEtwView::OnCopy(WORD, WORD, HWND, BOOL&) {
	auto selected = m_List.GetSelectedIndex();
	ATLASSERT(selected >= 0);
	CString text, item;
	for (int c = 0;; c++) {
		if (!m_List.GetItemText(selected, c, item))
			break;
		text += item + L",";
	}
	ClipboardHelper::CopyText(*this, text.Left(text.GetLength() - 1));

	return 0;
}

LRESULT CEtwView::OnSave(WORD, WORD, HWND, BOOL&) {
	return 0;
}

LRESULT CEtwView::OnCopyAll(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(!m_IsMonitoring || GetFrame()->GetTraceManager().IsPaused());
	CWaitCursor wait;
	auto count = m_List.GetItemCount();
	CString text, item;
	for (int i = 0; i < count; i++) {
		for (int c = 0;; c++) {
			if (!m_List.GetItemText(i, c, item))
				break;
			text += item + L",";
		}
		text.SetAt(text.GetLength() - 1, L'\n');
	}
	ClipboardHelper::CopyText(*this, text);
	return 0;
}

LRESULT CEtwView::OnConfigureEvents(WORD, WORD, HWND, BOOL&) {
	return 0;
}

LRESULT CEtwView::OnAutoScroll(WORD, WORD, HWND, BOOL&) {
	m_AutoScroll = !m_AutoScroll;
	GetFrame()->GetUpdateUI()->UISetCheck(ID_VIEW_AUTOSCROLL, m_AutoScroll);

	return 0;
}

LRESULT CEtwView::OnConfigFilters(WORD, WORD, HWND, BOOL&) {
	return 0;
}

LRESULT CEtwView::OnItemChanged(int, LPNMHDR, BOOL&) {
	UpdateUI();

	return 0;
}

LRESULT CEtwView::OnFindNext(WORD, WORD, HWND, BOOL&) {
	return 0;
}

void CEtwView::UpdateUI() {
	auto ui = GetFrame()->GetUpdateUI();
	auto& tm = GetFrame()->GetTraceManager();


}

void CEtwView::ApplyFilters(const FilterConfiguration& config) {
	auto& tm = GetFrame()->GetTraceManager();
	tm.RemoveAllFilters();
	for (int i = 0; i < config.GetFilterCount(); i++) {
		auto desc = config.GetFilter(i);
		ATLASSERT(desc);
	}
}