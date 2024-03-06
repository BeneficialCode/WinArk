#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include "Internals.h"
#include "SortHelper.h"

#include "NumberValueDlg.h"
#include "ChangeValueCommand.h"
#include "StringValueDlg.h"
#include "MultiStringValueDlg.h"
#include "BinaryValueDlg.h"
#include "ExportDlg.h"
#include "RenameValueCommand.h"
#include "ClipboardHelper.h"
#include "TreeHelper.h"
#include "ListViewHelper.h"
#include "RegHelpers.h"
#include "CreateKeyCommand.h"
#include "RenameKeyCommand.h"
#include "CreateValueCommand.h"
#include "CopyKeyCommand.h"
#include "CopyValueCommand.h"
#include "SecurityHelper.h"
#include "RegExportImport.h"
#include "GotoKeyDlg.h"

HWND CRegistryManagerView::GetHwnd() const {
	return m_hWnd;
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
	ATLTRACE(L"Found: %s, %s, %s\n", path, name, data);
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

	using PChangeWindowMessageFilterEx = BOOL(WINAPI*)(HWND, UINT, DWORD, void*);
	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	if (hUser32 != nullptr) {
		PChangeWindowMessageFilterEx pChangeWindowMessageFilterEx
			= (PChangeWindowMessageFilterEx)GetProcAddress(hUser32, "ChangeWindowMessageFilterEx");
		if (pChangeWindowMessageFilterEx != nullptr) {
			pChangeWindowMessageFilterEx(m_hWnd, WM_COPYDATA, 1, nullptr);
		}
	}

	m_Locations.Load(L"Software\\YuanOS\\WinArk");
	if (AppSettings::Get().Load(L"Software\\YuanOS\\WinArk"))
		m_ReadOnly = AppSettings::Get().ReadOnly();


	CComPtr<IAutoComplete> spAC;
	auto hr = spAC.CoCreateInstance(CLSID_AutoComplete);
	if (SUCCEEDED(hr)) {
		m_AutoCompleteStrings->CreateInstance(&m_AutoCompleteStrings);
		CRect r(0, 0, 800, 20);
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

	m_MainSplitter.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

	m_Tree.Create(m_MainSplitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS, WS_EX_CLIENTEDGE, TreeId);
	m_Tree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER | TVS_EX_RICHTOOLTIP, 0);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32 | ILC_COLOR | ILC_MASK, 16, 4);
	UINT icons[] = {
		IDI_COMPUTER,IDI_FOLDER,IDI_FOLDER_CLOSED,IDI_FOLDER_LINK,
		IDI_FOLDER_ACCESSDENIED,IDI_HIVE,IDI_HIVE_ACCESSDENIED,IDI_FOLDER_UP,
		IDI_BINARY,IDI_TEXT,IDI_REGISTRY,IDI_DWORD,IDI_QWORD,
	};
	for (auto icon : icons) {
		images.AddIcon(AtlLoadIconImage(icon, 0, 16, 16));
	}
	m_Tree.SetImageList(images, TVSIL_NORMAL);

	m_List.Create(m_MainSplitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
		| LVS_OWNERDATA | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_EDITLABELS, WS_EX_CLIENTEDGE);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_List.SetImageList(images, LVSIL_SMALL);

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

	m_AddressBar.SetFont(m_Tree.GetFont());

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
	text.Format(L"%s (%s)", msg, (PCWSTR)RegHelpers::GetErrorText(error));
	AtlMessageBox(hWnd ? hWnd : m_hWnd, (PCWSTR)text, IDS_TITLE, MB_ICONERROR);
}

CString CRegistryManagerView::GetColumnText(HWND h, int row, int col) const {
	auto& item = m_Items[row];
	CString text;
	switch (static_cast<ColumnType>(GetColumnManager(h)->GetColumnTag(col)))
	{
		case ColumnType::Name:
			return item.Name.IsEmpty() ? CString(RegHelpers::DefaultValueName) : item.Name;

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
			text.Format(L"%u", item.Size);
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

	std::sort(m_Items.begin() + (AppSettings::Get().ShowKeysInList() ? 1 : 0), m_Items.end(), compare);
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
		if (index == 2 && m_Items[row].Type == REG_BINARY) {
			menu.GetSubMenu(index).AppendMenu(MF_STRING, ID_EXPORT_BIN, L"Export Bin To File");
		}
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
	m_Tree.SetItemImage(m_hLocalRoot, 0, 0);
	m_hStdReg = m_Tree.InsertItem(L"Standard Registry", 0, 0, m_hLocalRoot, TVI_LAST);
	m_Tree.SetItemImage(m_hStdReg, 10, 10);
	SetNodeData(m_hStdReg, NodeType::StandardRoot);
	m_hRealReg = m_Tree.InsertItem(L"REGISTRY", 11, 11, m_hLocalRoot, TVI_LAST);
	SetNodeData(m_hRealReg, NodeType::RegistryRoot | NodeType::Predefined | NodeType::Key);
	m_Tree.SetItemImage(m_hRealReg, 10, 10);
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
			m_Tree.SetItemImage(hRoot, 5, 5);
			SetNodeData(hRoot, GetNodeData(hRoot) | NodeType::Hive);
		}
		else {
			m_Tree.SetItemImage(hRoot, 2, 2);
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
					m_Tree.SetItemImage(hItem, 3, 3);
				}
				else {
					m_Tree.SetItemImage(hItem, 2, 2);
				}
			}
			else if (error == ERROR_ACCESS_DENIED) {
				SetNodeData(hItem, GetNodeData(hItem) | NodeType::AccessDenied);
				m_Tree.SetItemImage(hItem, 4, 4);
			}
			auto path = GetFullNodePath(hItem);
			if (Registry::IsHiveKey(path)) {
				int image = error == ERROR_ACCESS_DENIED ? 6 : 5;
				m_Tree.SetItemImage(hItem, image, image);
				SetNodeData(hItem, GetNodeData(hItem) | NodeType::Hive);
			}
			else {
				m_Tree.SetItemImage(hItem, 2, 2);
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
	auto showExtra = AppSettings::Get().ShowExtraHive();
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
					RegHelpers::GetSystemDir(),
					RegHelpers::GetSystemDir() + CString(L"\\Drivers"),
					RegHelpers::GetWindowsDir(),
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

	int start = path[0] == L'\\' ? 9 : 0;

	if (path.Left(2) == L"\\\\") {
		// remote Registry
		start = path.Find(L"\\", 2);
		auto remoteName = path.Mid(2, start - 2);
		hItem = m_Tree.GetRootItem().GetNextSibling();

		while (hItem) {
			CString text;
			m_Tree.GetItemText(hItem, text);
			if (text.CompareNoCase(remoteName) == 0)
				break;

			hItem = m_Tree.GetNextSiblingItem(hItem);
		}

		if (!hItem)
			return nullptr;
	}

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

	if (AppSettings::Get().ShowKeysInList() && (hItem == m_hStdReg || hItem == m_hRealReg ||
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
			KEY_QUERY_VALUE | (AppSettings::Get().ShowKeysInList() ? KEY_ENUMERATE_SUB_KEYS : 0));
		ATLASSERT(m_CurrentKey.IsValid());
	}

	if (AppSettings::Get().ShowKeysInList()) {
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
		static const std::wstring computer(L"Computer\\");
		static const std::wstring computerName(L"计算机\\");

		std::wstring regPath(path);
		if (regPath.find(computer) == 0) {
			regPath = regPath.substr(computer.size());
		}
		else if (regPath.find(computerName) == 0) {
			regPath = regPath.substr(computerName.size());
		}
		GotoKey(regPath.c_str());
	}
	else if (wp == VK_ESCAPE) {
		m_List.SetFocus();
	}
	else {
		handled = FALSE;
	}
	return 0;
}

LRESULT CRegistryManagerView::OnRunOnUIThread(UINT, WPARAM, LPARAM lp, BOOL&) {
	auto f = reinterpret_cast<std::function<void()>*>(lp);
	(*f)();
	return 0;
}

LRESULT CRegistryManagerView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	if (m_AddressBar.IsWindow()) {
		int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
		CRect rc;
		m_AddressBar.GetClientRect(&rc);
		m_AddressBar.MoveWindow(0, 0, rc.right, rc.bottom);
		m_MainSplitter.MoveWindow(0, rc.bottom, cx, cy - rc.bottom);
	}
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

void CRegistryManagerView::RefreshFull(HTREEITEM hItem) {
	hItem = m_Tree.GetChildItem(hItem);
	TreeHelper th(m_Tree);
	while (hItem) {
		auto state = m_Tree.GetItemState(hItem, TVIS_EXPANDED | TVIS_EXPANDEDONCE);
		if (state) {
			if (state == TVIS_EXPANDEDONCE) {
				CString text;
				if (m_Tree.GetChildItem(hItem) && m_Tree.GetItemText(m_Tree.GetChildItem(hItem), text) && text != L"\\\\") {
					// not expanded now,delete all items and insert a dummy item
					th.DeleteChildren(hItem);
					m_Tree.InsertItem(L"\\\\", hItem, TVI_LAST);
				}
			}
			else {
				// really expanded
				RefreshFull(hItem);
				auto key = Registry::OpenKey(GetFullNodePath(hItem), KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
				if (key) {
					auto keys = th.GetChildItems(hItem);
					Registry::EnumSubKeys(key.Get(), [&](auto name, const auto&) {
						if (!th.FindChild(hItem, name)) {
							// new sub key
							auto hChild = InsertKeyItem(hItem, name);
						}
						else {
							keys.erase(name);
						}
						return true;
						});

					for (auto& [name, h] : keys)
						m_Tree.DeleteItem(h);

					if (m_Tree.GetChildItem(hItem) == nullptr) {
						// remove children indicator
						TVITEM tvi;
						tvi.hItem = hItem;
						tvi.mask = TVIF_CHILDREN;
						tvi.cChildren = 0;
						m_Tree.SetItem(&tvi);
					}
					else {
						m_Tree.SortChildren(hItem);
					}
				}
			}
		}
		else if (m_Tree.GetChildItem(hItem) == nullptr && (GetNodeData(hItem) & NodeType::AccessDenied) == NodeType::None) {
			// no children - check if new exist
			auto key = Registry::OpenKey(GetFullNodePath(hItem), KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
			if (Registry::GetSubKeyCount(key.Get()) > 0) {
				m_Tree.InsertItem(L"\\\\", hItem, TVI_LAST);
				TVITEM tvi;
				tvi.hItem = hItem;
				tvi.mask = TVIF_CHILDREN;
				tvi.cChildren = 1;
				m_Tree.SetItem(&tvi);
			}
		}
		hItem = m_Tree.GetNextSiblingItem(hItem);
	}
}

LRESULT CRegistryManagerView::OnViewRefresh(WORD, WORD, HWND, BOOL&) {
	auto path = GetCurrentKeyPath();
	TreeHelper th(m_Tree);
	th.DoForEachItem(m_hStdReg, 0, [this](auto hItem, auto state) {
		RefreshItem(hItem);
		});
	th.DoForEachItem(m_hRealReg, 0, [this](auto hItem, auto state) {
		RefreshItem(hItem);
		});
	GotoKey(path);
	return 0;
}

LRESULT CRegistryManagerView::OnNewKey(WORD, WORD, HWND, BOOL&) {
	m_Tree.GetSelectedItem().Expand(TVE_EXPAND);
	auto hItem = InsertKeyItem(m_Tree.GetSelectedItem(), L"(NewKey)");
	hItem.EnsureVisible();
	m_CurrentOperation = Operation::CreateKey;
	m_Tree.EditLabel(hItem);
	return 0;
}

bool CRegistryManagerView::RefreshItem(HTREEITEM hItem) {
	auto expanded = m_Tree.GetItemState(hItem, TVIS_EXPANDED);
	m_Tree.LockWindowUpdate();
	m_Tree.Expand(hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
	TreeHelper th(m_Tree);
	th.DeleteChildren(hItem);
	m_Tree.InsertItem(L"\\\\", hItem, TVI_LAST);
	if (expanded)
		m_Tree.Expand(hItem, TVE_EXPAND);
	m_Tree.LockWindowUpdate(FALSE);
	UpdateList(true);
	return true;
}


LRESULT CRegistryManagerView::OnTreeRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	RefreshItem(m_Tree.GetSelectedItem());
	return 0;
}

LRESULT CRegistryManagerView::OnNewValue(WORD /*wNotifyCode*/, WORD id, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	DWORD type = REG_NONE;
	switch (id)
	{
		case ID_NEW_BINARYVALUE:
			type = REG_BINARY;
			break;

		case ID_NEW_DWORDVALUE:
			type = REG_DWORD;
			break;

		case ID_NEW_QWORDVALUE:
			type = REG_QWORD;
			break;

		case ID_NEW_STRINGVALUE:
			type = REG_SZ;
			break;

		case ID_NEW_MULTIPLESTRINGVALUE:
			type = REG_MULTI_SZ;
			break;

		case ID_NEW_EXPANDSTRINGVALUE:
			type = REG_EXPAND_SZ;
			break;
	}

	RegistryItem item;
	item.Name = L"NewValue";
	item.Type = type;
	item.Key = false;
	m_Items.push_back(item);
	m_List.SetItemCountEx(m_List.GetItemCount() + 1, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
	m_CurrentOperation = Operation::CreateValue;
	m_List.EditLabel((int)m_Items.size() - 1);
	return 0;
}

LRESULT CRegistryManagerView::OnEditCopy(WORD, WORD, HWND, BOOL&){
	if (::GetFocus() == m_Tree) {
		ClipboardItem item;
		item.Key = true;
		item.Path = GetFullParentNodePath(m_Tree.GetSelectedItem());
		m_Tree.GetItemText(m_Tree.GetSelectedItem(), item.Name);
		m_Clipboard.Items.clear();
		m_Clipboard.Items.push_back(item);
		m_Clipboard.Operation = ClipboardOperation::Copy;
	}
	else if (::GetFocus() == m_List) {
		ATLASSERT(::GetFocus() == m_List);
		auto count = m_List.GetSelectedCount();
		ATLASSERT(count >= 1);
		int index = -1;
		auto path = GetFullNodePath(m_Tree.GetSelectedItem());
		m_Clipboard.Items.clear();
		CString text;
		for (UINT i = 0; i < count; i++) {
			index = m_List.GetNextItem(index, LVIS_SELECTED);
			ATLASSERT(index >= 0);
			text += ListViewHelper::GetRowAsString(m_List, index, L'\t') + L"\n";
			ClipboardItem ci;
			ci.Path = path;
			auto& item = m_Items[index];
			ci.Key = item.Key;
			ci.Name = item.Name;
			m_Clipboard.Items.push_back(ci);
		}
		ClipboardHelper::CopyText(m_hWnd, text);
		m_Clipboard.Operation = ClipboardOperation::Copy;
	}
	else {
		m_AddressBar.Copy();
	}
	

	return 0;
}

LRESULT CRegistryManagerView::OnEditCut(WORD code, WORD id, HWND hWndCtl, BOOL& bHandled) {
	OnEditCopy(code, id, hWndCtl, bHandled);
	if (m_Clipboard.Operation == ClipboardOperation::Copy)
		m_Clipboard.Operation = ClipboardOperation::Cut;
	return 0;
}

AppCommandCallback<DeleteKeyCommand> CRegistryManagerView::GetDeleteKeyCommandCallback() {
	static const auto cb = [this](auto& cmd, bool execute) {
		TreeHelper th(m_Tree);
		auto real = cmd.GetPath()[0] == L'\\';
		auto hParent = th.FindItem(real ? m_hRealReg : m_hStdReg, cmd.GetPath());
		ATLASSERT(hParent);
		if (execute) {
			auto hItem = th.FindChild(hParent, cmd.GetName());
			ATLASSERT(hItem);
			m_Tree.DeleteItem(hItem);
		}
		else {
			// create the item
			auto hItem = InsertKeyItem(hParent, cmd.GetName());
		}
		return true;
	};

	return cb;
}

LRESULT CRegistryManagerView::OnEditPaste(WORD, WORD, HWND, BOOL&){
	if (m_Clipboard.Items.empty()) {
		return 0;
	}
	ATLASSERT(!m_Clipboard.Items.empty());
	auto cb = [this](auto& cmd, bool) {
		RefreshItem(m_Tree.GetSelectedItem());
		UpdateList();
		return TRUE;
	};

	auto list = std::make_shared<AppCommandList>(nullptr, cb);
	auto path = GetFullNodePath(m_Tree.GetSelectedItem());
	for (auto& item : m_Clipboard.Items) {
		std::shared_ptr<AppCommand> cmd;
		if (item.Key)
			cmd = std::make_shared<CopyKeyCommand>(item.Path, item.Name, path);
		else
			cmd = std::make_shared<CopyValueCommand>(item.Path, item.Name, path);
		list->AddCommand(cmd);
		if (m_Clipboard.Operation == ClipboardOperation::Cut) {
			if (item.Key)
				cmd = std::make_shared<DeleteKeyCommand>(item.Path, item.Name, GetDeleteKeyCommandCallback());
			else
				cmd = std::make_shared<DeleteValueCommand>(item.Path, item.Name);
			list->AddCommand(cmd);
		}
	}
	if (list->GetCount() == 1)
		list->SetCommandName(list->GetCommand(0)->GetCommandName());
	else
		list->SetCommandName(L"Paste");
	if (!m_CmdMgr.AddCommand(list))
		DisplayError(L"Paste failed");
	return 0;
}

LRESULT CRegistryManagerView::OnEditRename(WORD, WORD, HWND, BOOL&){
	if (::GetFocus() == m_Tree) {
		m_CurrentOperation = Operation::RenameKey;
		m_Tree.EditLabel(m_Tree.GetSelectedItem());
	}
	else if (::GetFocus() == m_List) {
		auto index = m_List.GetSelectionMark();
		if (index >= 0) {
			m_CurrentOperation = m_Items[index].Key ? Operation::RenameKey : Operation::RenameValue;
			m_List.EditLabel(index);
		}
	}
	
	return 0;
}

LRESULT CRegistryManagerView::OnEditDelete(WORD, WORD, HWND, BOOL&){
	if (::GetFocus() == m_Tree) {
		auto hItem = m_Tree.GetSelectedItem();
		auto path = GetFullParentNodePath(hItem);
		CString name;
		m_Tree.GetItemText(hItem, name);
		auto cmd = std::make_shared<DeleteKeyCommand>(path, name, GetDeleteKeyCommandCallback());
		if (!m_CmdMgr.AddCommand(cmd))
			DisplayError(L"Failed to delete key");
	}
	else if (::GetFocus() == m_List) {
		auto count = m_List.GetSelectedCount();
		ATLASSERT(count >= 1);
		int index = -1;
		auto cb = [this](auto& cmd, auto) {
			auto cmd0 = std::static_pointer_cast<DeleteValueCommand>(cmd.GetCommand(0));
			if (GetFullNodePath(m_Tree.GetSelectedItem()) == cmd0->GetPath()) {
				RefreshItem(m_Tree.GetSelectedItem());
				UpdateList();
			}
			return true;
		};

		auto list = std::make_shared<AppCommandList>(L"", cb);
		auto path = GetFullNodePath(m_Tree.GetSelectedItem());
		for (UINT i = 0; i < count; i++) {
			index = m_List.GetNextItem(index, LVIS_SELECTED);
			ATLASSERT(index >= 0);
			auto& item = m_Items[index];
			if (item.Type == REG_KEY_UP) {
				count--;
				continue;
			}
			std::shared_ptr<AppCommand> cmd;
			if (item.Key) {
				cmd = std::make_shared<DeleteKeyCommand>(path, item.Name);
			}
			else {
				cmd = std::make_shared<DeleteValueCommand>(path, item.Name);
			}
			list->AddCommand(cmd);
		}
		if (count == 1) // only up key selected
			return 0;

		if (count == 1)
			list->SetCommandName(list->GetCommand(0)->GetCommandName());
		else
			list->SetCommandName(L"Delete");
		if (!m_CmdMgr.AddCommand(list))
			DisplayError(L"Delete failed.");
	}
	return 0;
}

LRESULT CRegistryManagerView::OnCopyFullKeyName(WORD, WORD, HWND, BOOL&){
	ClipboardHelper::CopyText(m_hWnd, GetFullNodePath(m_Tree.GetSelectedItem()));
	return 0;
}

LRESULT CRegistryManagerView::OnCopyKeyName(WORD, WORD, HWND, BOOL&){
	CString text;
	m_Tree.GetItemText(m_Tree.GetSelectedItem(), text);
	ClipboardHelper::CopyText(m_hWnd, text);
	return 0;
}

LRESULT CRegistryManagerView::OnKeyPermissions(WORD, WORD, HWND, BOOL&){
	auto path = GetFullNodePath(m_Tree.GetSelectedItem());
	SecurityHelper::EnablePrivilege(SE_TAKE_OWNERSHIP_NAME, true);
	if (::GetFocus() == m_List) {
		auto& item = m_Items[m_List.GetSelectionMark()];
		ATLASSERT(item.Key);
		path += L"\\" + item.Name;
	}
	auto readonly = m_ReadOnly;
	auto key = Registry::OpenKey(path, READ_CONTROL | (readonly ? 0 : (WRITE_DAC | WRITE_OWNER)));
	if (!key && ::GetLastError() == ERROR_ACCESS_DENIED) {
		key = Registry::OpenKey(path, READ_CONTROL);
		readonly = true;
	}
	if (!key && ::GetLastError() == ERROR_ACCESS_DENIED) {
		key = Registry::OpenKey(path, MAXIMUM_ALLOWED);
	}
	if (!key) {
		DisplayError(L"Failed to open key");
	}
	else {
		CSecurityInformation si(key, path, readonly);
		::EditSecurity(m_hWnd, &si);
	}
	SecurityHelper::EnablePrivilege(SE_TAKE_OWNERSHIP_NAME, false);
	return 0;
}

LRESULT CRegistryManagerView::OnProperties(WORD, WORD, HWND, BOOL&){
	if (::GetFocus() == m_List) {
		auto index = m_List.GetSelectionMark();
		if (index >= 0 && !m_Items[index].Key)
			return (LRESULT)ShowValueProperties(m_Items[index], index);
	}
	return 0;
}

INT_PTR CRegistryManagerView::ShowValueProperties(RegistryItem& item, int index) {
	auto cb = [this](auto& cmd, bool) {
		if (GetFullNodePath(m_Tree.GetSelectedItem()) == cmd.GetPath()) {
			int index = m_List.FindItem(cmd.GetName(), false);
			ATLASSERT(index >= 0);
			if (index >= 0) {
				m_Items[index].Value.Empty();
				m_Items[index].Size -= 1;
				m_List.RedrawItems(index, index);
				m_List.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
			}
		}
		return true;
	};
	bool success = false;
	bool result = false;
	switch (item.Type)
	{
		case REG_LINK:
			AtlMessageBox(m_hWnd, L"No special properties available for a symbolic link", IDS_TITLE, MB_ICONINFORMATION);
			return 0;

		case REG_SZ:
		case REG_EXPAND_SZ:
		{
			CStringValueDlg dlg(m_CurrentKey, item.Name, m_ReadOnly);
			result = dlg.DoModal() == IDOK && dlg.IsModified();
			if (result) {
				auto hItem = m_Tree.GetSelectedItem();
				DWORD type = dlg.GetType();
				CString value = dlg.GetValue();
				LONG size = (1 + value.GetLength()) * sizeof(WCHAR);
				auto cmd = std::make_shared<ChangeValueCommand>(GetFullNodePath(hItem), 
					item.Name, type, (PVOID)(PCWSTR)value,size);
				success = m_CmdMgr.AddCommand(cmd);
				if (success) {
					cmd->SetCallback(cb);
					item.Value.Empty();
					item.Size -= 1;
					m_List.RedrawItems(index, index);
				}
			}
			break;
		}

		case REG_MULTI_SZ:
		{
			CMultiStringValueDlg dlg(m_CurrentKey, item.Name, m_ReadOnly);
			result = dlg.DoModal() == IDOK && dlg.IsModified();
			if (result) {
				auto hItem = m_Tree.GetSelectedItem();
				auto value = dlg.GetValue();
				// 清除的目标字符集合中的任一字符，直到遇到第一个不属于目标字符串子集的字符为止
				value.TrimRight(L"\r\n");
				value.Replace(L"\r\n", L"\n");
				auto len = value.GetLength();
				for (int i = 0; i < len; i++) {
					if (value[i] == L'\n')
						value.SetAt(i, 0);
				}

				auto cmd = std::make_shared<ChangeValueCommand>(
					GetFullNodePath(hItem),item.Name,REG_MULTI_SZ,
					(PVOID)(PCWSTR)value,
					(1+len)*(LONG)sizeof(WCHAR));

				success = m_CmdMgr.AddCommand(cmd);
				if (success)
					cmd->SetCallback(cb);
			}
			break;
		}
		case REG_DWORD:
		case REG_QWORD:
		{
			CNumberValueDlg dlg(m_CurrentKey, item.Name, item.Type, m_ReadOnly);
			result = dlg.DoModal() == IDOK && dlg.IsModified();
			if (result) {
				auto hItem = m_Tree.GetSelectedItem();
				auto value = dlg.GetValue();
				auto cmd = std::make_shared<ChangeValueCommand>(
					GetFullNodePath(hItem), item.Name, item.Type, &value,
					dlg.GetType() == REG_DWORD ? 4 : 8);
				success = m_CmdMgr.AddCommand(cmd);
				if (success)
					cmd->SetCallback(cb);
			}
			break;
		}
		case REG_BINARY:
		case REG_FULL_RESOURCE_DESCRIPTOR:
		case REG_RESOURCE_REQUIREMENTS_LIST:
		case REG_RESOURCE_LIST:
		{
			CBinaryValueDlg dlg(m_CurrentKey, item.Name, m_ReadOnly, this);
			result = dlg.DoModal() == IDOK && dlg.IsModified();
			if (result) {
				auto hItem = m_Tree.GetSelectedItem();
				auto cmd = std::make_shared<ChangeValueCommand>(
					GetFullNodePath(hItem), item.Name, item.Type, (const PVOID)dlg.GetValue().data(),
					(LONG)dlg.GetValue().size());
				success = m_CmdMgr.AddCommand(cmd);
				if (success) {
					cmd->SetCallback(cb);
					item.Size = (DWORD)dlg.GetValue().size();
				}
			}
			break;
		}
	}
	if (!result)
		return 0;

	if (!success)
		DisplayError(L"Failed to changed value");
	else {
		item.Value.Empty();
		auto index = m_List.GetSelectionMark();
		m_List.RedrawItems(index, index);
	}

	return 0;
}

LRESULT CRegistryManagerView::OnImport(WORD, WORD, HWND, BOOL&) {
	if (!SecurityHelper::EnablePrivilege(SE_BACKUP_NAME, true) ||
		!SecurityHelper::EnablePrivilege(SE_RESTORE_NAME, true)) {
		AtlMessageBox(m_hWnd, L"Exporting, importing, and loading hives require the Backup/Restore privileges. \
			Running elevated will allow it.", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	CSimpleFileDialog dlg(TRUE, L"dat", nullptr, OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER,
		L"All Files\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto error = ::RegRestoreKey(m_CurrentKey.Get(), dlg.m_szFileName, REG_FORCE_RESTORE);
		if (ERROR_SUCCESS != error) {
			DisplayError(L"Failed to import file", nullptr, error);
		}
		else {
			RefreshItem(m_Tree.GetSelectedItem());
		}
	}

	SecurityHelper::EnablePrivilege(SE_BACKUP_NAME, false);
	SecurityHelper::EnablePrivilege(SE_RESTORE_NAME, false);

	return 0;
}

LRESULT CRegistryManagerView::OnExport(WORD, WORD, HWND, BOOL&) {
	CExportDlg dlg;
	dlg.SetKeyPath(GetFullNodePath(m_Tree.GetSelectedItem()));
	if (dlg.DoModal() == IDOK) {
		auto filename = dlg.GetFileName();
		const auto& path = dlg.GetSelectedKey();
		if (filename.Right(4).CompareNoCase(L".reg") == 0) {
			CWaitCursor wait;
			RegExportImport reg;
			if (reg.Export(path, filename))
				AtlMessageBox(m_hWnd, L"Export successful.", IDS_TITLE, MB_ICONINFORMATION);
			else
				AtlMessageBox(m_hWnd, L"Export failed.", IDS_TITLE, MB_ICONERROR);
			return 0;
		}
		if (!SecurityHelper::EnablePrivilege(SE_BACKUP_NAME, true)) {
			AtlMessageBox(m_hWnd, L"Exporting, importing, and loading hives require the Backup/Restore privileges. \
			Running elevated will allow it.", IDS_TITLE, MB_ICONERROR);
			return 0;
		}

		HKEY hKey;
		if (path.IsEmpty())
			hKey = Registry::OpenRealRegistryKey();
		else {
			auto key = Registry::OpenKey(path, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
			hKey = key.Detach();
		}
		if (!hKey)
			DisplayError(L"Failed to open key to export");
		else {
			LSTATUS error;
			CWaitCursor wait;
			::DeleteFile(dlg.GetFileName());
			if (path == "HKEY_CLASSES_ROOT")
				error = ::RegSaveKey(hKey, filename, nullptr);
			else
				error = ::RegSaveKeyEx(hKey, filename, nullptr, REG_LATEST_FORMAT);
			::SetLastError(error);
			if (error != ERROR_SUCCESS)
				DisplayError(L"Failed to export key");
			else
				AtlMessageBox(m_hWnd, L"Export successful.", IDS_TITLE, MB_ICONINFORMATION);
			SecurityHelper::EnablePrivilege(SE_BACKUP_NAME, false);
			::RegCloseKey(hKey);
		}
	}
	return 0;
}

LRESULT CRegistryManagerView::OnGotoKey(WORD, WORD, HWND, BOOL&) {
	CGotoKeyDlg dlg;
	dlg.SetKey(GetFullNodePath(m_Tree.GetSelectedItem()));
	if (dlg.DoModal() == IDOK) {
		CWaitCursor wait;
		auto hItem = GotoKey(dlg.GetKey());
		if(!hItem)
			AtlMessageBox(m_hWnd, L"Failed to locate key", IDS_TITLE, MB_ICONERROR);
	}
	return 0;
}

LRESULT CRegistryManagerView::OnExportBin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, nullptr,
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"Binary File\0*.bin\0Native Format\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return -1;

		int row = m_List.GetSelectionMark();
		
		CString path = GetFullNodePath(m_Tree.GetSelectedItem());

		auto key = Registry::OpenKey(path, KEY_READ);
		if (!key)
			return -1;

		Registry::EnumKeyValues(key, [&](auto type, auto name, auto size) {
			CString sname(name);
			
			auto data = std::make_unique<BYTE[]>(size + 3);
			auto count{ size };
			if (ERROR_SUCCESS != key.QueryValue(name, nullptr, data.get(), &count))
				return true;

			if (type != REG_BINARY) {
				return true;
			}

			if (sname == m_Items[row].Name) {
				WriteFile(hFile, data.get(), size, nullptr, nullptr);
			}

			return true;
			});

		::CloseHandle(hFile);
		if ((INT_PTR)::ShellExecute(nullptr, L"open", L"explorer",
			L"/select,\"" + CString(dlg.m_szFileName) + L"\"",
			nullptr, SW_SHOWDEFAULT) < 32)
			AtlMessageBox(*this, L"Failed to locate bin", IDS_TITLE, MB_ICONERROR);
	}
	return 0;
}

int CRegistryManagerView::GetKeyImage(const RegistryItem& item) const {
	int image = 2;
	if (!m_CurrentKey)
		return image;

	CString linkPath;
	if (Registry::IsKeyLink(m_CurrentKey.Get(), item.Name, linkPath))
		return 3;

	RegistryKey key;
	auto error = key.Open(m_CurrentKey.Get(), item.Name, READ_CONTROL);
	if (Registry::IsHiveKey(GetFullNodePath(m_Tree.GetSelectedItem()) + L"\\" + item.Name))
		image = error == ERROR_ACCESS_DENIED ? 6 : 5;
	else if (error == ERROR_ACCESS_DENIED)
		image = 4;

	return image;
}

int CRegistryManagerView::GetRowImage(HWND hWnd, int row, int col) const {
	switch (m_Items[row].Type) {
	case REG_KEY:
		return GetKeyImage(m_Items[row]);
	case REG_KEY_UP:
		return 7;
	case REG_DWORD:
		return 11;
	case REG_QWORD:
		return 12;
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
	case REG_NONE:
		return 9;
	}
	return 8;
}