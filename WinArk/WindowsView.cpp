#include "stdafx.h"
#include "resource.h"
#include "WindowsView.h"
#include "ProcessHelper.h"
#include "WindowHelper.h"
#include "SortHelper.h"
#include "FormatHelper.h"

CString CWindowsView::GetColumnText(HWND, int row, int col) const {
	const auto h = m_Items[row];
	if (!::IsWindow(h)) {
		return L"";
	}

	CWindow win(h);
	
	CString text;
	
	DataItemType type = GetColumnManager(m_List)->GetColumnTag<DataItemType>(col);
	switch (type)
	{
	case DataItemType::Handle:
		text.Format(L"0x%zX", (ULONG_PTR)h);
		break;

	case DataItemType::ClassName:
		return WindowHelper::GetWindowClassName(h);

	case DataItemType::Text:
		win.GetWindowText(text);
		break;

	case DataItemType::Style:
		text.Format(L"0x%08X",win.GetStyle());
		break;

	case DataItemType::ExtendedStyle:
		text.Format(L"0x%08X", win.GetExStyle());
		break;

	case DataItemType::ProcessName:
	{
		DWORD pid;
		::GetWindowThreadProcessId(win, &pid);
		return ProcessHelper::GetProcessName(pid);
	}

	case DataItemType::ProcessId:
	{
		DWORD pid;
		::GetWindowThreadProcessId(win, &pid);
		text.Format(L"%u", pid);
		break;
	}

	case DataItemType::ThreadId:
	{
		auto tid = ::GetWindowThreadProcessId(win, nullptr);
		text.Format(L"%u", tid);
		break;
	}

	case DataItemType::ParentWindow:
		return FormatHelper::FormatHWndOrNone(::GetAncestor(win, GA_PARENT));
		
	case DataItemType::FirstChildWindow:
		return FormatHelper::FormatHWndOrNone(win.GetWindow(GW_CHILD));

		
	case DataItemType::NextWindow:
		return FormatHelper::FormatHWndOrNone(win.GetWindow(GW_HWNDNEXT));

	case DataItemType::PrevWindow:
		return FormatHelper::FormatHWndOrNone(win.GetWindow(GW_HWNDPREV));

	case DataItemType::OwnerWindow:
		return FormatHelper::FormatHWndOrNone(win.GetWindow(GW_OWNER));

	case DataItemType::WindowProc:
		text.Format(L"0x%zX", win.GetWindowLongPtr(GWLP_WNDPROC));
		break;

	case DataItemType::UserData:
		text.Format(L"0x%zX", win.GetWindowLongPtr(GWLP_USERDATA));
		break;

	case DataItemType::ID:
		text.Format(L"0x%zX", win.GetWindowLongPtr(GWLP_ID));
		break;

	case DataItemType::Rectangle:
		return WindowHelper::WindowRectToString(h);
		break;

	case DataItemType::ClassAtom:
		text.Format(L"0x%04X", (DWORD)::GetClassLongPtr(win, GCW_ATOM));
		break;

	case DataItemType::ClassStyle:
		text.Format(L"0x%04X", (DWORD)::GetClassLongPtr(win, GCL_STYLE));
		break;

	case DataItemType::ClassExtra:
		text.Format(L"%u", (ULONG)::GetClassLongPtr(win, GCL_CBCLSEXTRA));
		break;

	case DataItemType::WindowExtra:
		text.Format(L"%u", (ULONG)::GetClassLongPtr(win, GCL_CBWNDEXTRA));
		break;

	default:
		break;
	}

	return text;
}

int CWindowsView::GetRowImage(HWND, int row,int col) const {
	auto h = m_Items[row];
	if (auto it = s_IconMap.find(h); it != s_IconMap.end()) {
		return it->second;
	}
	auto hIcon = WindowHelper::GetWindowIcon(h);
	if (hIcon) {
		int image = s_Images.AddIcon(hIcon);
		s_IconMap.insert({ h,image });
		return image;
	}
	return 0;
}

void CWindowsView::OnActivate(bool activate) {
	if (activate) {
		SetTimer(1, 2000, nullptr);
	}
	else {
		KillTimer(1);
	}
}

void CWindowsView::DoSort(const SortInfo* si) {
	if (si == nullptr)
		return;

	std::sort(m_Items.begin(), m_Items.end(), [&](const auto& h1, const auto& h2)->bool {
		switch (GetColumnManager(m_List)->GetColumnTag<DataItemType>(si->SortColumn)){
		case DataItemType::ClassName:
			return SortHelper::SortStrings(WindowHelper::GetWindowClassName(h1),
				WindowHelper::GetWindowClassName(h2), si->SortAscending);
		}
		return false;
		});
}

bool CWindowsView::IsSortable(HWND,int col) const {
	return col == 0;
}

//DWORD CWindowsView::OnPrePaint(int, LPNMCUSTOMDRAW cd) {
//	if (cd->hdr.hwndFrom == m_List)
//		return CDRF_NOTIFYITEMDRAW;
//
//	return CDRF_DODEFAULT;
//}
//
//DWORD CWindowsView::OnItemPrePaint(int, LPNMCUSTOMDRAW cd) {
//	ATLASSERT(cd->hdr.hwndFrom == m_List);
//
//	auto h = m_Items[(int)cd->dwItemSpec];
//	auto lv = (LPNMLVCUSTOMDRAW)cd;
//	lv->clrTextBk = ::IsWindowVisible(h) ? CLR_INVALID : RGB(192, 192, 192);
//
//	return CDRF_DODEFAULT;
//}


void CWindowsView::Refresh() {
	if (!m_SelectedHwnd.IsWindow()) {
		m_Tree.DeleteItem(m_Selected);
		return;
	}

	UpdateList();
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetCountPerPage() + m_List.GetTopIndex());
}

void CWindowsView::InitTree() {
	m_TotalVisibleWindows = m_TotalWindows = m_TopLevelWindows = 0;
	m_DesktopNode = nullptr;
	HWND hDesktop = ::GetDesktopWindow();
	if (!hDesktop)
		return;

	CWaitCursor wait;

	m_Tree.LockWindowUpdate(TRUE);
	m_Deleting = true;
	m_Tree.DeleteAllItems();
	m_WindowsMap.clear();
	m_Deleting = false;

	m_DesktopNode = AddNode(hDesktop, TVI_ROOT);

	::EnumWindows([](auto hWnd, auto lp)->BOOL {
		auto pThis = (CWindowsView*)lp;
		pThis->AddNode(hWnd, pThis->m_DesktopNode);
		return TRUE;
		}, reinterpret_cast<LPARAM>(this));

	m_DesktopNode.Expand(TVE_EXPAND);
	m_Tree.LockWindowUpdate(FALSE);
	m_DesktopNode.Select();
	m_DesktopNode.EnsureVisible();
	m_Tree.SetScrollPos(SB_HORZ, 0);
}

void CWindowsView::AddChildWindows(HTREEITEM hParent) {
	auto hWnd = (HWND)m_Tree.GetItemData(hParent);
	ATLASSERT(hWnd);

	m_hCurrentNode = hParent;
	::EnumChildWindows(hWnd, [](auto hChild, auto p)->BOOL {
		auto pThis = (CWindowsView*)p;
		return pThis->AddChildNode(hChild);
		}, reinterpret_cast<LPARAM>(this));
}

CTreeItem CWindowsView::AddNode(HWND hWnd, HTREEITEM hParent) {
	CString text, name;
	CWindow win(hWnd);
	m_TotalWindows++;
	if (win.IsWindowVisible())
		m_TotalVisibleWindows++;

	if (m_DesktopNode) {
		if (::GetAncestor(hWnd, GA_PARENT) == (HWND)m_DesktopNode.GetData())
			m_TopLevelWindows++;

		if (!m_ShowHiddenWindows && !win.IsWindowVisible())
			return nullptr;
		win.GetWindowText(name);
		if (!m_ShowNoTitleWindows && name.IsEmpty())
			return nullptr;
	}

	if (name.GetLength() > 64)
		name = name.Left(64) + L"...";
	if (!name.IsEmpty())
		name = L"[" + name + L"]";
	WCHAR className[64] = { 0 };
	::GetClassName(hWnd, className, _countof(className));
	text.Format(L"0x%zX (%s) %s", (DWORD_PTR)hWnd, className, (PCWSTR)name);

	HICON hIcon{ nullptr };
	int image = 0;
	if ((win.GetStyle() & WS_CHILD) == 0) {
		// 如果不存在，就插入map
		if (auto it = s_IconMap.find(hWnd); it == s_IconMap.end()) {
			hIcon = WindowHelper::GetWindowOrProcessIcon(hWnd);
			if (hIcon) {
				s_IconMap.insert({ hWnd,image = s_Images.AddIcon(hIcon) });
			}
		}
		else {
			image = it->second;
		}
	}

	auto node = m_Tree.InsertItem(text, image, image, hParent, TVI_LAST);
	node.SetData((DWORD_PTR)hWnd);
	m_WindowsMap.insert({ hWnd,node });

	if (!win.IsWindowVisible())
		node.SetState(TVIS_CUT, TVIS_CUT);

	if (m_DesktopNode && win.GetWindow(GW_CHILD)) {
		// add a "plus" button
		node.AddTail(L"*", 0);
	}
	return node;
}

BOOL CWindowsView::AddChildNode(HWND hChild) {
	if (::GetAncestor(hChild, GA_PARENT) == (HWND)m_Tree.GetItemData(m_hCurrentNode)) {
		AddNode(hChild, m_hCurrentNode);
	}
	return TRUE;
}

void CWindowsView::AddChildWindows(std::vector<HWND>& v, HWND hParent, bool directOnly) {
	struct LocalInfo {
		std::vector<HWND>& v;
		CWindowsView* pThis;
		bool directOnly;
	};

	LocalInfo info{ v,this,directOnly };

	::EnumChildWindows(hParent, [](auto hWnd, auto param) {
		auto info = reinterpret_cast<LocalInfo*>(param);
		if (info->pThis->m_ShowHiddenWindows || ::IsWindowVisible(hWnd)) {
			info->v.push_back(hWnd);
			if (!info->directOnly)
				info->pThis->AddChildWindows(info->v, hWnd, false);
		}
		return TRUE;
		}, reinterpret_cast<LPARAM>(&info));
}

void CWindowsView::UpdateList() {
	m_Items.clear();
	m_Items.push_back(m_SelectedHwnd);
	AddChildWindows(m_Items, m_SelectedHwnd, true);

	m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOSCROLL);
	DoSort(GetSortInfo(m_List));
}

CString CWindowsView::GetDetails(const DataItem& item) const {
	CString text;
	switch (item.Type) {
		case DataItemType::ProcessId:
		{
			DWORD pid = 0;
			::GetWindowThreadProcessId(m_SelectedHwnd, &pid);
			return ProcessHelper::GetFullProcessName(pid);
		}

		case DataItemType::Text:
			text.Format(L"Length: %d", m_SelectedHwnd.GetWindowTextLength());
			break;

		case DataItemType::Style: return WindowHelper::WindowStyleToString(m_SelectedHwnd);
		case DataItemType::ClassStyle: return WindowHelper::ClassStyleToString(m_SelectedHwnd);
		case DataItemType::ExtendedStyle: return WindowHelper::WindowStyleToString(m_SelectedHwnd);
		case DataItemType::Rectangle:
			if (m_SelectedHwnd.IsZoomed())
				return L"Maximized";
			if (m_SelectedHwnd.IsIconic())
				return L"Minimized";
			break;
		case DataItemType::NextWindow: return GetWindowClassAndTitle(m_SelectedHwnd.GetWindow(GW_HWNDNEXT));
		case DataItemType::PrevWindow: return GetWindowClassAndTitle(m_SelectedHwnd.GetWindow(GW_HWNDPREV));
		case DataItemType::OwnerWindow: return GetWindowClassAndTitle(m_SelectedHwnd.GetWindow(GW_OWNER));
		case DataItemType::ParentWindow: return GetWindowClassAndTitle(::GetAncestor(m_SelectedHwnd, GA_PARENT));
		case DataItemType::FirstChildWindow: return GetWindowClassAndTitle(m_SelectedHwnd.GetWindow(GW_CHILD));
	}
	return text;
}

CString CWindowsView::GetWindowClassAndTitle(HWND hWnd) {
	if (hWnd == nullptr || !::IsWindow(hWnd))
		return L"";

	WCHAR className[64];
	CString text;
	if (::GetClassName(hWnd, className, _countof(className)))
		text.Format(L"[%s]",className);
	CString title;
	CWindow(hWnd).GetWindowText(title);
	if (!title.IsEmpty())
		text.Format(L"%s (%s)", text, title);
	return text;
}

LRESULT CWindowsView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// 两个窗格一起改变大小
	m_Splitter.SetSplitterExtendedStyle(SPLIT_FLATBAR | SPLIT_PROPORTIONAL);
	m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);

	m_List.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | 
		WS_CLIPCHILDREN |WS_CLIPSIBLINGS |
		LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHAREIMAGELISTS);
	m_Tree.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE
		| WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_WINDOW_TREE);

	

	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_Tree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	if (s_Images == nullptr) {
		s_Images.Create(16, 16, ILC_COLOR32, 32, 32);
		s_Images.AddIcon(AtlLoadIconImage(IDI_WINDOW, 0, 16, 16));
	}
	m_Tree.SetImageList(s_Images, TVSIL_NORMAL);
	m_List.SetImageList(s_Images, LVSIL_SMALL);

	m_Splitter.SetSplitterPanes(m_Tree, m_List);
	// 设置分割条的位置
	m_Splitter.SetSplitterPosPct(35);

	auto cm = GetColumnManager(m_List);

	struct {
		PCWSTR text;
		DataItemType type;
		int width;
		int format = LVCFMT_LEFT;
		ColumnFlags flags = ColumnFlags::Visible;
	}columns[] = {
		{ L"Class Name", DataItemType::ClassName, 140 },
		{ L"Handle", DataItemType::Handle, 80, LVCFMT_RIGHT },
		{ L"Text", DataItemType::Text, 150 },
		{ L"Style", DataItemType::Style, 80, LVCFMT_RIGHT },
		{ L"Ex Style", DataItemType::ExtendedStyle, 80, LVCFMT_RIGHT },
		{ L"PID", DataItemType::ProcessId, 70, LVCFMT_RIGHT },
		{ L"Process Name", DataItemType::ProcessName, 120 },
		{ L"TID", DataItemType::ThreadId, 70, LVCFMT_RIGHT },
		{ L"Rectangle", DataItemType::Rectangle, 170, LVCFMT_LEFT, ColumnFlags::None },
		{ L"Parent HWND", DataItemType::ParentWindow, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Owner HWND", DataItemType::OwnerWindow, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Next HWND", DataItemType::NextWindow, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Previous HWND", DataItemType::PrevWindow, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"First Child HWND", DataItemType::FirstChildWindow, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"WndProc", DataItemType::WindowProc, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"User Data", DataItemType::UserData, 100, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"ID", DataItemType::ID, 80, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Class Atom", DataItemType::ClassAtom, 90, LVCFMT_RIGHT },
		{ L"Class Style", DataItemType::ClassStyle, 80, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Class Extra Bytes", DataItemType::ClassExtra, 70, LVCFMT_RIGHT, ColumnFlags::Fixed },
		{ L"Window Extra Bytes", DataItemType::WindowExtra, 70, LVCFMT_RIGHT, ColumnFlags::Fixed },
	};

	for (auto& col : columns) {
		cm->AddColumn(col.text, col.format, col.width, col.type, col.flags);
	}

	cm->UpdateColumns();

	m_WindowsMap.reserve(256);
	InitTree();

	SetTimer(1, 2000, nullptr);

	return 0;
}

LRESULT CWindowsView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1) {
		Refresh();
	}
	else if (id == 3) {
		KillTimer(3);
		m_Selected = m_Tree.GetSelectedItem();
		m_SelectedHwnd.Detach();
		auto hWnd = (HWND)m_Selected.GetData();
		if (!::IsWindow(hWnd)) // window is probably destroyed
			m_Selected.Delete();
		else {
			m_SelectedHwnd.Attach(hWnd);
			UpdateList();
		}
	}
	return 0;
}

LRESULT CWindowsView::OnNodeExpanding(int, LPNMHDR hdr, BOOL&) {
	auto tv = (NMTREEVIEW*)hdr;
	if (tv->action == TVE_EXPAND) {
		auto hItem = tv->itemNew.hItem;

		auto child = m_Tree.GetChildItem(hItem);
		if (child.GetData() == 0) {
			child.Delete();
			AddChildWindows(hItem);
		}
	}
	return 0;
}

LRESULT CWindowsView::OnNodeDeleted(int, LPNMHDR hdr, BOOL&) {
	if (!m_Deleting) {
		auto tv = (NMTREEVIEW*)hdr;
		m_WindowsMap.erase((HWND)tv->itemOld.lParam);
	}
	return 0;
}

LRESULT CWindowsView::OnNodeSelected(int, LPNMHDR, BOOL&) {
	// short delay before update in case the user moves quickly through the tree
	SetTimer(3, 250, nullptr);
	return 0;
}

LRESULT CWindowsView::OnWindowShow(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	if (m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(SW_SHOW);

	return 0;
}

LRESULT CWindowsView::OnWindowHide(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	if (m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(SW_HIDE);
	return 0;
}

LRESULT CWindowsView::OnWindowClose(WORD, WORD, HWND, BOOL&) {
	if (m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(WM_CLOSE);
	return 0;
}

LRESULT CWindowsView::OnWindowMinimize(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	if(m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(SW_MINIMIZE);
	return 0;
}

LRESULT CWindowsView::OnWindowMaximize(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	if(m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(SW_MAXIMIZE);

	return 0;
}

LRESULT CWindowsView::OnWindowRestore(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	if(m_SelectedHwnd)
		m_SelectedHwnd.ShowWindowAsync(SW_RESTORE);
	return 0;
}

LRESULT CWindowsView::OnWindowBringToFront(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	m_SelectedHwnd.BringWindowToTop();
	return 0;
}

//LRESULT CWindowsView::OnToggleHiddenWindows(WORD, WORD, HWND, BOOL&) {
//	m_ShowHiddenWindows = !m_ShowHiddenWindows;
//	InitTree();
//	return 0;
//}

LRESULT CWindowsView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	InitTree();
	::InvalidateRect(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	return 0;
}

//LRESULT CWindowsView::OnToggleEmptyTitleWindows(WORD, WORD, HWND, BOOL&) {
//	m_ShowNoTitleWindows = !m_ShowNoTitleWindows;
//	InitTree();
//	return 0;
//}

//LRESULT CWindowsView::OnToggleChildWindows(WORD, WORD, HWND, BOOL&) {
//	m_ShowChildWindows = !m_ShowChildWindows;
//	InitTree();
//	return 0;
//}

LRESULT CWindowsView::OnWindowFlash(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_Selected);
	FLASHWINFO info = { sizeof(info) };
	info.dwFlags = FLASHW_CAPTION;
	info.uCount = 3;
	info.hwnd = m_SelectedHwnd;
	::FlashWindowEx(&info);

	return 0;
}

LRESULT CWindowsView::OnTreeNodeRightClick(int, LPNMHDR, BOOL&) {
	ATLASSERT(m_Selected);
	if (!m_Selected)
		return 0;

	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	CPoint pt;
	::GetCursorPos(&pt);

	TrackPopupMenu(menu.GetSubMenu(4), 0, pt.x, pt.y, 0, m_hWnd, nullptr);
	return 0;
}

LRESULT CWindowsView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
	m_Splitter.MoveWindow(0, 0, cx, cy);
	return 0;
}