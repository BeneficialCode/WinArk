#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include "Internals.h"
#include "SortHelper.h"

#include "IntValueDlg.h"
#include "ChangeValueCommand.h"
#include "StringValueDlg.h"
#include "MultiStringValueDlg.h"
#include "BinaryValueDlg.h"
#include "RenameValueCommand.h"
#include "ClipboardHelper.h"
#include "TreeHelper.h"
#include "ListViewHelper.h"
#include "Helpers.h"
#include "CreateKeyCommand.h"
#include "RenameKeyCommand.h"
#include "CreateValueCommand.h"


HWND CRegistryManagerView::GetHwnd() const {
	return m_hWnd;
}

AppSettings& CRegistryManagerView::GetSettings() {
	return m_Settings;
}

void CRegistryManagerView::RunOnUiThread(std::function<void()> f) {
	SendMessage(WM_RUN, 0, reinterpret_cast<LPARAM>(&f));
}

LRESULT CRegistryManagerView::OnFindUpdate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	TreeHelper th(m_Tree);
	HTREEITEM hItem;
	auto fd = reinterpret_cast<FindData*>(lParam);
	if (fd->Path[0] != L'\\') {
		hItem = th.FindItem(m_hStdReg, fd->Path);
	}
	else {
		hItem = th.FindItem(m_hRealReg, fd->Path);
	}
	ATLASSERT(hItem);
	m_Tree.SelectItem(hItem);
	m_Tree.EnsureVisible(hItem);
	if ((fd->Name == nullptr || *fd->Name == 0) && (fd->Data == nullptr || *fd->Data == 0)) {
		m_Tree.SetFocus();
	}
	else {
		UpdateList(true);
		int index = ListViewHelper::FindItem(m_List, fd->Data ? fd->Data : fd->Name, true);
		ATLASSERT(index >= 0);
		m_List.EnsureVisible(index, FALSE);
		m_List.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
		m_List.SetSelectionMark(index);
		m_List.SetFocus();
	}
	return 0;
}

void CRegistryManagerView::OnFindNext(PCWSTR path, PCWSTR name, PCWSTR data) {
	ATLASSERT(L"Found: %s, %s, %s\n", path, name, data);
	FindData fd{ path,name,data };
	SendMessage(WM_FIND_UPDATE, 0, reinterpret_cast<LPARAM>(&fd));
}

void CRegistryManagerView::OnFindStart() {
	::SetCursor(AtlLoadSysCursor(IDC_APPSTARTING));
}

void CRegistryManagerView::OnFindEnd(bool cancelled) {
	if (!cancelled) {
		RunOnUiThread([this]() {
			AtlMessageBox(m_hWnd, L"Finished searching the Registry.", IDS_TITLE, MB_ICONINFORMATION);
			});
	}
}

bool CRegistryManagerView::GoToItem(PCWSTR path, PCWSTR name, PCWSTR data) {
	TreeHelper th(m_Tree);
	auto hItem = th.FindItem(path[0] == L'\\' ? m_hRealReg : m_hStdReg, path);
	if (!hItem)
		return false;

	SetActiveWindow();
	m_UpdateNoDelay = true;
	m_Tree.SelectItem(hItem);
	UpdateList();
	if (name && *name) {
		int index = m_List.FindItem(name, false);
		if (index >= 0) {
			m_List.SetSelectionMark(index);
			m_List.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
			m_List.SetFocus();
		}
	}
	else {
		m_Tree.SetFocus();
	}
	return true;
}

LRESULT CRegistryManagerView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	::RegDeleteTree(HKEY_CURRENT_USER, DeletedPathBackup.Left(DeletedPathBackup.GetLength() - 1));

	::ChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	if (m_Settings.Load(L"Software\\ScorpioSoftware\\RegExp"))
		m_ReadOnly = m_Settings.ReadOnly();
	m_Locations.Load(L"Software\\ScorpioSoftware\\RegExp");

	CComPtr<IAutoComplete> spAC;
	auto hr = spAC.CoCreateInstance(CLSID_AutoComplete);
	if (SUCCEEDED(hr)) {
		m_AutoCompleteStrings->CreateInstance(&m_AutoCompleteStrings);
		CRect r(0, 0, 400, 20);
		CEdit edit;
		auto hEdit = edit.Create(m_hWnd, &r, nullptr, WS_CHILD | WS_VISIBLE | 
			ES_AUTOHSCROLL | ES_WANTRETURN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
		hr = spAC->Init(hEdit, m_AutoCompleteStrings->GetUnknown(), nullptr, nullptr);
		if (SUCCEEDED(hr)) {
			spAC->Enable(TRUE);
			CComQIPtr<IAutoComplete2> spAC2(spAC);
			if (spAC2) {
				spAC2->SetOptions(ACO_AUTOSUGGEST | ACO_USETAB | ACO_AUTOAPPEND);
			}
			
			ATLVERIFY(m_AddressBar.SubclassWindow(hEdit));
		}
		else {
			m_AddressBar.DestroyWindow();
		}
	}

	/*RECT rc;
	GetClientRect(&rc);*/

	m_MainSplitter.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

	m_Tree.Create(m_MainSplitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS, WS_EX_CLIENTEDGE, TreeId);
	m_Tree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER | TVS_EX_RICHTOOLTIP, 0);

	m_List.Create(m_MainSplitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
		| LVS_OWNERDATA | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_EDITLABELS, WS_EX_CLIENTEDGE);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	
	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"Name", LVCFMT_LEFT, 220, ColumnType::Name);
	cm->AddColumn(L"Type", LVCFMT_LEFT, 110, ColumnType::Type);
	cm->AddColumn(L"Size", LVCFMT_RIGHT, 70, ColumnType::Size);
	cm->AddColumn(L"Value", LVCFMT_LEFT, 250, ColumnType::Value);
	cm->AddColumn(L"Last Write", LVCFMT_LEFT, 120, ColumnType::TimeStamp);
	cm->AddColumn(L"Details", LVCFMT_LEFT, 250, ColumnType::Details);
	cm->UpdateColumns();

	m_MainSplitter.SetSplitterPanes(m_Tree, m_List);
	m_MainSplitter.SetSplitterPosPct(25);
	m_MainSplitter.UpdateSplitterLayout();

	PostMessage(WM_BUILD_TREE);

	return 0;
}

void CRegistryManagerView::SetStartKey(const CString& key) {
	m_StartKey = key;
}

CString CRegistryManagerView::GetCurrentKeyPath() {
	return GetFullNodePath(m_Tree.GetSelectedItem());
}

void CRegistryManagerView::DisplayError(PCWSTR msg, HWND hWnd, DWORD error) const {
	CString text;
	text.Format(L"%s (%s)", msg, (PCWSTR)Helpers::GetErrorText(error));
	AtlMessageBox(hWnd ? hWnd : m_hWnd, (PCWSTR)text, IDS_TITLE, MB_ICONERROR);
}

CString CRegistryManagerView::GetColumnText(HWND h, int row, int col) const {
	auto& item = m_Items[row];
	CString text;
	switch (static_cast<ColumnType>(GetColumnManager(h)->GetColumnTag(col)))
	{
		case ColumnType::Name:
			return item.Name.IsEmpty() ? CString(Helpers::DefaultValueName) : item.Name;

		case ColumnType::Type:
			return Registry::GetRegTypeAsString(item.Type);

		case ColumnType::Value:
			if (!item.Key) {
				if (item.Value.IsEmpty())
					item.Value = Registry::GetDataAsString(m_CurrentKey, item);
				return item.Value;
			}
			break;

		case ColumnType::Size:
			if (!item.Key) {
				// recalculate
				item.Size;
				m_CurrentKey.QueryValue(item.Name, nullptr, nullptr, &item.Size);
			}
			text.Format(L"%u",item.Size);
			break;

		case ColumnType::TimeStamp:
			if (item.TimeStamp.dwHighDateTime + item.TimeStamp.dwLowDateTime)
				return CTime(item.TimeStamp).Format(L"%x %X");
			break;
		
		case ColumnType::Details:
			return item.Key ? GetKeyDetails(item) : GetValueDetails(item);
	}
	return text;
}

void CRegistryManagerView::DoSort(const SortInfo* si) {
	if (si == nullptr || si->SortColumn < 0 || m_Items.empty())
		return;

	auto col = GetColumnManager(m_List)->GetColumnTag<ColumnType>(si->SortColumn);
	auto asc = si->SortAscending;
	auto compare = [&](auto& d1, auto& d2) {
		switch (col)
		{
			case ColumnType::Name:
				return SortHelper::SortStrings(d1.Name, d2.Name, asc);
			case ColumnType::Type:
				return SortHelper::SortNumbers(d1.Type, d2.Type, asc);
			case ColumnType::Size:
				return SortHelper::SortNumbers(d1.Size, d2.Size, asc);
			case ColumnType::TimeStamp:
				return SortHelper::SortNumbers(*(ULONGLONG*)&d1.TimeStamp, *(ULONGLONG*)&d2.TimeStamp, asc);
		}
		return false;
	};

	std::sort(m_Items.begin() + (m_Settings.ShowKeysInList() ? 1 : 0), m_Items.end(), compare);
}

bool CRegistryManagerView::IsSortable(HWND h, int col) const {
	auto tag = GetColumnManager(h)->GetColumnTag<ColumnType>(col);
	return tag == ColumnType::Name || tag == ColumnType::Size || tag == ColumnType::Type || tag == ColumnType::TimeStamp;
}

BOOL CRegistryManagerView::OnRightClickList(HWND h, int row, int col, const POINT& pt) {
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	int index = 3;
	if (row >= 0) {
		index = m_Items[row].Key ? 1 : 2;
		if (m_Items[row].Type == REG_KEY_UP)
			return FALSE;
	}

	return TrackPopupMenu(menu.GetSubMenu(index), TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, nullptr);
}

BOOL CRegistryManagerView::OnDoubleClickList(HWND, int row, int col, const POINT& pt) {
	if (row < 0)
		return FALSE;

	auto& item = m_Items[row];
	if (item.Key) {
		if (item.Type == REG_KEY_UP) {
			m_UpdateNoDelay = true;
			m_Tree.SelectItem(m_Tree.GetParentItem(m_Tree.GetSelectedItem()));
			m_Tree.EnsureVisible(m_Tree.GetSelectedItem());
			m_List.SetSelectionMark(0);
			return TRUE;
		}
		ATLASSERT(item.Type == REG_KEY);
		m_Tree.Expand(m_Tree.GetSelectedItem(), TVE_EXPAND);
		TreeHelper th(m_Tree);
		auto hItem = th.FindChild(m_Tree.GetSelectedItem(), item.Name);
		ATLASSERT(hItem);
		m_UpdateNoDelay = true;
		m_Tree.SelectItem(hItem);
		m_Tree.EnsureVisible(hItem);
		return TRUE;
	}
	else {
		
	}
	return FALSE;
}

LRESULT CRegistryManagerView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	m_Items.clear();
	bHandled = FALSE;
	return 1;
}

LRESULT CRegistryManagerView::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 2) {
		KillTimer(2);
		UpdateList();
	}
	return 0;
}

LRESULT CRegistryManagerView::OnEditPaint(UINT, WPARAM wp, LPARAM, BOOL& handled) {
	CDCHandle dc((HDC)wp);
	CRect rc;
	GetClientRect(&rc);
	dc.FillSolidRect(&rc, 255);
	return 1;
}

LRESULT CRegistryManagerView::OnGoToKeyExternal(UINT, WPARAM, LPARAM lp, BOOL&) {
	auto cds = reinterpret_cast<COPYDATASTRUCT*>(lp);
	if (cds->dwData == 0x1000) {
		SetActiveWindow();
		if (cds->lpData)
			GotoKey((PCWSTR)cds->lpData);
		return 1;
	}

	return 0;
}

void CRegistryManagerView::InitTree() {
	WCHAR name[32];
	DWORD len = _countof(name);
	::GetComputerName(name, &len);
	m_hLocalRoot = m_Tree.InsertItem(name + CString(L" (Local)"), 1, 1, TVI_ROOT, TVI_LAST);
	m_hLocalRoot.SetData(static_cast<DWORD_PTR>(NodeType::Machine));
	m_hStdReg = m_Tree.InsertItem(L"Standard Registry", 0, 0, m_hLocalRoot, TVI_LAST);
	SetNodeData(m_hStdReg, NodeType::StandardRoot);
	m_hRealReg = m_Tree.InsertItem(L"REGISTRY", 11, 11, m_hLocalRoot, TVI_LAST);
	SetNodeData(m_hRealReg, NodeType::RegistryRoot | NodeType::Predefined | NodeType::Key);
	m_hLocalRoot.Expand(TVE_EXPAND);
	m_Tree.SelectItem(m_hStdReg);
}

NodeType CRegistryManagerView::GetNodeData(HTREEITEM hItem) const {
	return static_cast<NodeType>(m_Tree.GetItemData(hItem));
}

void CRegistryManagerView::SetNodeData(HTREEITEM hItem, NodeType type) {
	m_Tree.SetItemData(hItem, static_cast<DWORD_PTR>(type));
}

HTREEITEM CRegistryManagerView::BuildTree(HTREEITEM hRoot, HKEY hKey, PCWSTR name) {
	if (name) {
		hRoot = m_Tree.InsertItem(name, 3, 2, hRoot, TVI_LAST);
		auto path = GetFullNodePath(hRoot);
		if (Registry::IsHiveKey(path)) {
			SetNodeData(hRoot, GetNodeData(hRoot) | NodeType::Hive);
		}
		auto subkeys = Registry::GetSubKeyCount(hKey);
		if (subkeys) {
			m_Tree.InsertItem(L"\\\\", hRoot, TVI_LAST);
		}
	}
	else {
		CRegKey key(hKey);
		Registry::EnumSubKeys(key, [&](auto name, const auto& ft) {
			auto hItem = m_Tree.InsertItem(name, 3, 2, hRoot, TVI_LAST);
			SetNodeData(hItem, NodeType::Key);
			CRegKey subKey;
			auto error = subKey.Open(hKey, name, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
			if (error == ERROR_SUCCESS) {
				DWORD subkeys = Registry::GetSubKeyCount(subKey);
				if (subkeys)
					m_Tree.InsertItem(L"\\\\", hItem, TVI_LAST);
				subKey.Close();
				CString linkPath;
				if (Registry::IsKeyLink(key, name, linkPath)) {
					// m_Tree.SetItemImage(hItem, 4, 4);
				}
			}

			else if (error == ERROR_ACCESS_DENIED) {
				SetNodeData(hItem, GetNodeData(hItem) | NodeType::AccessDenied);
			}
			auto path = GetFullNodePath(hItem);
			if (Registry::IsHiveKey(path)) {
				SetNodeData(hItem, GetNodeData(hItem) | NodeType::Hive);
			}
			return true;
			});

		m_Tree.SortChildren(hRoot);
		key.Detach();
	}

	return hRoot;
}

LRESULT CRegistryManagerView::OnBuildTree(UINT, WPARAM, LPARAM, BOOL&) {
	InitTree();
	auto showExtra = m_Settings.ShowExtraHive();
	m_Tree.LockWindowUpdate();

	int i = 0;
	for (auto& k : Registry::Keys) {
		if (!showExtra && ++i == 6)
			break;
		auto hItem = BuildTree(m_hStdReg, k.hKey, k.text);
		SetNodeData(hItem, NodeType::Key | NodeType::Predefined);
	}
	m_Tree.Expand(m_hStdReg, TVE_EXPAND);

	auto hKey = Registry::OpenRealRegistryKey();
	if (hKey) {
		CRegKey key(hKey);
		BuildTree(m_hRealReg, key);
		m_Tree.Expand(m_hRealReg, TVE_EXPAND);
	}
	m_Tree.LockWindowUpdate(FALSE);

	if (!m_StartKey.IsEmpty()) {
		auto hItem = GotoKey(m_StartKey);
		if (!hItem) {
			AtlMessageBox(m_hWnd, (PCWSTR)(L"Failed to locate key " + m_StartKey), IDS_TITLE, MB_ICONWARNING);
		}
	}
	m_Tree.SetFocus();

	return 0;
}

CString CRegistryManagerView::GetFullNodePath(HTREEITEM hItem) const {
	CString path;
	auto hPrev = hItem;
	CString text;

	while (hItem && ((GetNodeData(hItem) & (NodeType::Predefined | NodeType::Key)) != NodeType::None)) {
		ATLVERIFY(m_Tree.GetItemText(hItem, text));
		path = text + L"\\" + path;
		hPrev = hItem;
		hItem = m_Tree.GetParentItem(hItem);
	}
	path.TrimRight(L"\\");
	if (path.Left(8) == L"REGISTRY")
		path = L"\\" + path;
	if ((GetNodeData(hItem) & NodeType::RemoteRegistry) == NodeType::RemoteRegistry) {
		CString name;
		m_Tree.GetItemText(hItem, name);
		path = L"\\\\" + name + (path.IsEmpty() ? L"" : L"\\") + path;
	}
	return path;
}

CString CRegistryManagerView::GetFullParentNodePath(HTREEITEM hItem) const {
	auto path = GetFullNodePath(hItem);
	auto index = path.ReverseFind(L'\\');
	if (index < 0)
		return L"";

	return path.Left(index);
}

CString CRegistryManagerView::GetKeyDetails(const RegistryItem& item) const {
	if (item.Type == REG_KEY_UP)
		return L"";

	RegistryKey key;
	if (!m_CurrentKey)
		key = Registry::OpenKey(m_CurrentPath + (m_CurrentPath.IsEmpty() ? L"" : L"\\") + item.Name, KEY_QUERY_VALUE);
	else
		key.Open(m_CurrentKey.Get(), item.Name, KEY_QUERY_VALUE);
	CString text;
	if (key) {
		DWORD values = 0;
		auto subkeys = Registry::GetSubKeyCount(key, &values);
		text.Format(L"Subkeys: %u, Values: %u", subkeys, values);
	}
	return text;
}

CString CRegistryManagerView::GetValueDetails(const RegistryItem& item) const {
	ATLASSERT(!item.Key);
	CString text;
	if (item.Value.IsEmpty())
		item.Value = Registry::GetDataAsString(m_CurrentKey, item);
	switch (item.Type)
	{
		case REG_EXPAND_SZ:
			if (item.Value.Find(L"%") >= 0) {
				text = Registry::ExpandStrings(item.Value);
			}
			break;

		case REG_SZ:
			if (item.Value[0] == L'@') {
				static const CString paths[] = {
					L"",
					Helpers::GetSystemDir(),
					Helpers::GetSystemDir()+CString(L"\\Drivers"),
					Helpers::GetWindowsDir(),
				};
				for (auto& path : paths) {
					if (ERROR_FILE_NOT_FOUND != ::RegLoadMUIString(m_CurrentKey.Get(),
						item.Name, text.GetBufferSetLength(512), 512,
						nullptr, REG_MUI_STRING_TRUNCATE, path.IsEmpty() ? nullptr : (PCWSTR)path))
						break;
				}
			}
			break;
	}
	return text;
}

CTreeItem CRegistryManagerView::InsertKeyItem(HTREEITEM hParent, PCWSTR name, NodeType type) {
	bool accessDenied = (type & NodeType::AccessDenied) == NodeType::AccessDenied;
	auto item = m_Tree.InsertItem(name, hParent, TVI_LAST);
	SetNodeData(item, type);
	if ((type & NodeType::Key) == NodeType::Key) {
		auto key = Registry::OpenKey(GetFullNodePath(item), KEY_QUERY_VALUE);
		if (key) {
			if (Registry::GetSubKeyCount(key.Get()) > 0)
				m_Tree.InsertItem(L"\\\\", item, TVI_LAST);
		}
	}
	return item;
}

HTREEITEM CRegistryManagerView::BuildKeyPath(const CString& path, bool accessible) {
	auto hItem = path[0] == L'\\' ? m_hRealReg : m_hStdReg;
	CString name;
	int start = path[0] == L'\\' ? 10 : 0;
	TreeHelper th(m_Tree);
	while (!(name = path.Tokenize(L"\\", start)).IsEmpty()) {
		auto hChild = th.FindChild(hItem, name);
		if (hChild) {
			hItem = hChild;
			continue;
		}
		hItem = InsertKeyItem(hItem, name, NodeType::Key | (accessible ? NodeType::None : NodeType::AccessDenied));
	}
	return hItem;
}

HTREEITEM CRegistryManagerView::GotoKey(const CString& path, bool knownToExist) {
	CString spath(path);
	spath.MakeUpper();
	if (spath != L'\\') {
		if (spath.Find(L'\\') < 0)
			spath += L"\\";
		spath.Replace(L"HKLM\\", L"HKEY_LOCAL_MACHINE\\");
		spath.Replace(L"HKCU\\", L"HKEY_CURRENT_USER\\");
		spath.Replace(L"HKCR\\", L"HKEY_CLASSES_ROOT\\");
		spath.Replace(L"HKU\\", L"HKEY_USERS\\");
		spath.Replace(L"HKCC\\", L"HKEY_CURRENT_CONFIG\\");
	}
	auto hItem = TreeHelper(m_Tree).FindItem(spath[0] == L'\\' ? m_hRealReg : m_hStdReg, spath);
	if (!hItem || knownToExist) {
		auto key = Registry::OpenKey(path, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);
		hItem = BuildKeyPath(path, key.Get() != nullptr);
	}

	if (hItem) {
		m_Tree.SelectItem(hItem);
		m_Tree.EnsureVisible(hItem);
		m_Tree.SetFocus();
	}
	return hItem;
}

void CRegistryManagerView::UpdateList(bool force) {
	m_Items.clear();
	m_List.SetItemCount(0);

	auto hItem = m_Tree.GetSelectedItem();
	m_CurrentPath = GetFullNodePath(hItem);
	auto& path = m_CurrentPath;

	m_AddressBar.SetWindowText(path);

	m_CurrentKey.Close();

	if (hItem == m_hLocalRoot)
		return;

	if (hItem == m_hRealReg)
		m_CurrentKey.Attach(Registry::OpenRealRegistryKey());

	if (m_Settings.ShowKeysInList() && (hItem == m_hStdReg || hItem == m_hRealReg ||
		(GetNodeData(hItem) & NodeType::RemoteRegistry) == NodeType::RemoteRegistry)) {
		// special case for root of registry
		for (hItem = m_Tree.GetChildItem(hItem); hItem; hItem = m_Tree.GetNextSiblingItem(hItem)) {
			RegistryItem item;
			CString name;
			m_Tree.GetItemText(hItem, name);
			item.Name = name;
			if (m_CurrentPath[0] == L'\\')
				name = m_CurrentPath + L"\\" + name;
			auto key = Registry::OpenKey(name, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
			if (key) {
				Registry::GetSubKeyCount(key.Get(), nullptr, &item.TimeStamp);
			}
			item.Key = true;
			item.Type = REG_KEY;
			m_Items.push_back(std::move(item));
		}
		m_List.SetItemCount(static_cast<int>(m_Items.size()));
		DoSort(GetSortInfo(m_List));

		return;
	}

	if (!m_CurrentPath.IsEmpty()) {
		m_CurrentKey = Registry::OpenKey(m_CurrentPath, 
			KEY_QUERY_VALUE | (m_Settings.ShowKeysInList() ? KEY_ENUMERATE_SUB_KEYS : 0));
		ATLASSERT(m_CurrentKey.IsValid());
	}

	if (m_Settings.ShowKeysInList()) {
		// insert up directory
		RegistryItem up;
		up.Name = L"..";
		up.Type = REG_KEY_UP;
		up.Key = true;
		m_Items.push_back(up);

		if (m_CurrentKey) {
			Registry::EnumSubKeys(m_CurrentKey.Get(), [&](auto name, const auto& ft) {
				RegistryItem item;
				item.Name = name;
				item.TimeStamp = ft;
				item.Key = true;
				item.Type = REG_KEY;
				m_Items.push_back(std::move(item));
				return true;
				});
		}
	}

	if (m_CurrentKey) {
		Registry::EnumKeyValues(m_CurrentKey.Get(), [&](auto type, auto name, auto size) {
			RegistryItem item;
			item.Name = name;
			item.Type = type;
			item.Size = size;
			m_Items.push_back(item);
			return true;
		});
	}

	auto parentPath = GetFullParentNodePath(hItem);
	if (!parentPath.IsEmpty()) {
		auto temp = Registry::OpenKey(parentPath, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
		if (temp) {
			CString name, linkPath;
			m_Tree.GetItemText(hItem, name);
			if (Registry::IsKeyLink(temp.Get(), name, linkPath)) {
				RegistryItem item;
				item.Name = L"SymbolicLinkName";
				item.Type = REG_LINK;
				item.Size = (linkPath.GetLength() + 1) * sizeof(WCHAR);
				item.Value = linkPath;
				m_Items.push_back(item);
			}
		}
	}

	m_List.SetItemCount(static_cast<int>(m_Items.size()));
	DoSort(GetSortInfo(m_List));
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

LRESULT CRegistryManagerView::OnTreeItemExpanding(int, LPNMHDR hdr, BOOL&) {
	auto tv = (NMTREEVIEW*)hdr;
	CString text;
	auto h = tv->itemNew.hItem;
	m_Tree.GetItemText(m_Tree.GetChildItem(h), text);
	if (text == L"\\\\") {
		m_Tree.DeleteItem(m_Tree.GetChildItem(h));
		CWaitCursor wait;
		ExpandItem(h);
	}
	return FALSE;
}

void CRegistryManagerView::ExpandItem(HTREEITEM hItem) {
	auto path = GetFullNodePath(hItem);
	auto key = Registry::OpenKey(path, KEY_ENUMERATE_SUB_KEYS);

	if (key) {
		m_Tree.SetRedraw(FALSE);
		BuildTree(hItem, key.Get());
		m_Tree.SetRedraw(TRUE);
	}
}

LRESULT CRegistryManagerView::OnTreeSelChanged(int, LPNMHDR, BOOL&) {
	if (m_UpdateNoDelay) {
		m_UpdateNoDelay = false;
		UpdateList();
	}
	else {
		// short delay in case the user is scrolling fast
		SetTimer(2, 200, nullptr);
	}

	return 0;
}

HTREEITEM CRegistryManagerView::FindItemByPath(PCWSTR path) const {
	CTreeItem item;
	if (path[0] == L'\\') {
		// real registry
		item = m_hRealReg;
	}
	else {
		// standard registry
		item = m_hStdReg;
	}

	CString spath(path);
	int start = 0;
	while (true) {
		auto token = spath.Tokenize(L"\\", start);
		if (token.IsEmpty())
			break;
		TreeHelper th(m_Tree);
		item = th.FindChild(item, token);
	}
	return item;
}

LRESULT CRegistryManagerView::OnTreeEndEdit(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
	auto& item = ((NMTVDISPINFO*)hdr)->item;
	if (item.pszText == nullptr) {
		// cancelled
		if (m_CurrentOperation == Operation::CreateKey)
			m_Tree.DeleteItem(item.hItem);
		return FALSE;
	}

	switch (m_CurrentOperation)
	{
		case CRegistryManagerView::Operation::RenameKey:
		{
			CString name;
			m_Tree.GetItemText(item.hItem, name);
			auto cb = [this](auto& cmd, bool) {
				auto hParent = FindItemByPath(cmd.GetPath());
				ATLASSERT(hParent);
				TreeHelper th(m_Tree);
				auto hItem = th.FindChild(hParent, cmd.GetName());
				ATLASSERT(hItem);
				m_Tree.SetItemText(hItem, cmd.GetNewName());
				return true;
			};
			auto hParent = m_Tree.GetParentItem(item.hItem);
			auto cmd = std::make_shared<RenameKeyCommand>(GetFullNodePath(hParent), name, item.pszText);
			if (!m_CmdMgr.AddCommand(cmd)) {
				DisplayError(L"Failed to rename key");
				return FALSE;
			}
			cmd->SetCallback(cb);
			return TRUE;
		}
		case CRegistryManagerView::Operation::CreateKey:
		{
			auto hItem = item.hItem;
			auto hParent = m_Tree.GetParentItem(hItem);
			auto cmd = std::make_shared<CreateKeyCommand>(GetFullNodePath(hParent), item.pszText);
			if (!m_CmdMgr.AddCommand(cmd)) {
				m_Tree.DeleteItem(hItem);
				DisplayError(L"Failed to create key");
				return FALSE;
			}
			auto cb = [this](auto& cmd, bool execute) {
				if (execute) {
					auto hParent = FindItemByPath(cmd.GetPath());
					ATLASSERT(hParent);
					auto hItem = InsertKeyItem(hParent, cmd.GetName());
					m_Tree.EnsureVisible(hItem);
				}
				else {
					auto hItem = FindItemByPath(cmd.GetPath() + L"\\" + cmd.GetName());
					ATLASSERT(hItem);
					m_Tree.DeleteItem(hItem);
				}
				return true;
			};
			cmd->SetCallback(cb);
			if (AppSettings::Get().ShowKeysInList())
				UpdateList();
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CRegistryManagerView::OnTreeBeginEdit(int, LPNMHDR hdr, BOOL&) {
	auto tv = (NMTVDISPINFO*)hdr;
	auto prevent = m_ReadOnly || (GetNodeData(tv->item.hItem) & (NodeType::Key | NodeType::Predefined)) != NodeType::Key;
	if (!prevent && m_CurrentOperation == Operation::None)
		m_CurrentOperation = Operation::RenameKey;
	return prevent;
}

LRESULT CRegistryManagerView::OnListItemChanged(int, LPNMHDR, BOOL&)
{
	int selected = m_List.GetSelectionMark();
	if (selected != m_CurrentSelectedItem) {
		m_CurrentSelectedItem = selected;
	}
	return 0;
}

LRESULT CRegistryManagerView::OnListEndEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	auto lv = (NMLVDISPINFO*)pnmh;
	if (lv->item.pszText == nullptr) {
		// cancelled
		return FALSE;
	}

	std::shared_ptr<AppCommand> cmd;
	auto hItem = m_Tree.GetSelectedItem();
	auto index = lv->item.iItem;
	auto& item = m_Items[index];
	auto path = GetFullNodePath(hItem);
	switch (m_CurrentOperation)
	{
		case Operation::RenameKey:
			cmd = std::make_shared<RenameKeyCommand>(path, item.Name, lv->item.pszText);
			break;

		case Operation::CreateKey:
			cmd = std::make_shared<CreateKeyCommand>(path, lv->item.pszText);
			break;

		case Operation::CreateValue:
		{
			CString text(lv->item.pszText);
			if (text.IsEmpty()) {
				// default value
			}
			auto cb = [this](auto cmd, auto) {
				if (m_CurrentOperation == Operation::None && cmd.GetPath() == GetFullNodePath(m_Tree.GetSelectedItem())) {
					UpdateList(true);
				}
				return true;
			};
			cmd = std::make_shared<CreateValueCommand>(path, text, item.Type, cb);
			break;
		}

		case Operation::RenameValue:
			cmd = std::make_shared<RenameValueCommand>(GetFullNodePath(hItem), item.Name, lv->item.pszText);
			break;
	}
	LRESULT rv = FALSE;
	ATLASSERT(cmd);
	if (cmd) {
		auto op = m_CurrentOperation;
		m_CurrentOperation = Operation::None;
		if (!m_CmdMgr.AddCommand(cmd)) {
			DisplayError(L"Failed to " + cmd->GetCommandName());
		}
		else {
			switch (op) {
				case Operation::CreateValue:
					// 智能指针之间的转换
					item.Size = std::static_pointer_cast<CreateValueCommand>(cmd)->GetSize();
					break;

				case Operation::RenameKey:
				case Operation::RenameValue:
					item.Name = lv->item.pszText;
					break;
			}

			m_List.RedrawItems(index, index);
			m_List.SetSelectionMark(index);
			m_List.EnsureVisible(index, FALSE);
			rv = TRUE;
		}
	}
	return rv;
}

LRESULT CRegistryManagerView::OnListBeginEdit(int, LPNMHDR pnmh, BOOL&) {
	if (m_ReadOnly)
		return TRUE;

	if (m_CurrentOperation != Operation::None)
		return FALSE;

	auto lv = (NMLVDISPINFO*)pnmh;
	auto& item = m_Items[lv->item.iItem];
	m_CurrentOperation = item.Key ? Operation::RenameKey : Operation::RenameValue;
	return FALSE;
}

void CRegistryManagerView::InvokeTreeContextMenu(const CPoint& pt) {
	CPoint pt2(pt);
	m_Tree.ClientToScreen(&pt2);
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	TrackPopupMenu(menu.GetSubMenu(5), 0, pt2.x, pt2.y, 0, m_hWnd, nullptr);
}

LRESULT CRegistryManagerView::OnTreeKeyDown(int, LPNMHDR hdr, BOOL&) {
	auto tv = (NMTVKEYDOWN*)hdr;
	switch (tv->wVKey)
	{
		case VK_TAB:
			m_List.SetFocus();
			if (m_List.GetSelectedCount() == 0)
				m_List.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
			return TRUE;
		
		case VK_APPS:
			CRect rc;
			if (m_Tree.GetItemRect(m_Tree.GetSelectedItem(), &rc, TRUE)) {
				InvokeTreeContextMenu(rc.CenterPoint());
				return TRUE;
			}
			break;
	}
	return 0;
}

LRESULT CRegistryManagerView::OnEditKeyDown(UINT, WPARAM wp, LPARAM, BOOL& handled) {
	if (wp == VK_RETURN) {
		CString path;
		m_AddressBar.GetWindowText(path);
		GotoKey(path);
	}
	else if (wp == VK_ESCAPE) {
		m_List.SetFocus();
	}
	else {
		handled = FALSE;
	}
	return 0;
}

LRESULT CRegistryManagerView::OnRunOnUIThread(UINT, WPARAM, LPARAM lp, BOOL&){
	auto f = reinterpret_cast<std::function<void()>*>(lp);
	(*f)();
	return 0;
}

LRESULT CRegistryManagerView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	GetClientRect(&rc);
	int iX = rc.left;
	int width = rc.Width();
	int bottom = rc.bottom;
	int height = rc.Height();
	::GetClientRect(m_AddressBar.m_hWnd, &rc);
	int editHeight = rc.Height();
	::MoveWindow(m_AddressBar, rc.left, rc.top, width, editHeight, TRUE);


	bHandled = false;
	return 0;
}

LRESULT CRegistryManagerView::OnListKeyDown(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
	auto lv = (NMLVKEYDOWN*)hdr;
	switch (lv->wVKey)
	{
		case VK_TAB:
			m_Tree.SetFocus();
			break;

		case VK_RETURN:
			OnDoubleClickList(m_List, m_List.GetSelectionMark(), 0, POINT());
			break;
	}
	return 0;
}

LRESULT CRegistryManagerView::OnTreeContextMenu(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	CPoint pt;
	::GetCursorPos(&pt);
	CPoint pt2(pt);
	m_Tree.ScreenToClient(&pt);
	UINT flags;
	auto hItem = m_Tree.HitTest(pt, &flags);
	if (hItem) {
		m_Tree.SelectItem(hItem);
		if (hItem == m_hStdReg || hItem == m_hLocalRoot)
			return 0;

		CMenu menu;
		menu.LoadMenu(IDR_CONTEXT);
		auto type = GetNodeData(hItem);
		auto hMenu = menu.GetSubMenu((type & NodeType::RemoteRegistry) == NodeType::RemoteRegistry ? 6 : 5);
		if ((type & NodeType::Hive) == NodeType::Hive) {
			// add jump to hive file
			//hMenu.InsertMenu(0, MF_BYPOSITION, ID_KEY_JUMPTOHIVEFILE, L"Jump &Hive File...");
		}
		TrackPopupMenu(hMenu, 0, pt2.x, pt2.y, 0, m_hWnd, nullptr);
	}
	return 0;
}