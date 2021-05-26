#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include "TreeNodes.h"
#include "Internals.h"
#include "UICommon.h"
#include "SortHelper.h"

#include "IntValueDlg.h"
#include "ChangeValueCommand.h"
#include "StringValueDlg.h"
#include "MultiStringValueDlg.h"
#include "BinaryValueDlg.h"
#include "RenameValueCommand.h"
#include "ClipboardHelper.h"
#include "CreateNewValueCommand.h"

#include <algorithm>

bool CRegistryManagerView::CompareItems(const ListItem& i1, const ListItem& i2, int col, bool asc) {
	
	switch (col) {
		case 0:return SortHelper::SortStrings(i1.GetName(), i2.GetName(), asc);
		case 1:return SortHelper::SortStrings(i1.GetName(), i2.GetName(), asc);
		case 2:return SortHelper::SortNumbers(i1.ValueSize, i2.ValueSize, asc);
		case 5:return SortHelper::SortNumbers(i1.LastWriteTime.QuadPart, i2.LastWriteTime.QuadPart, asc);
	}
	return false;
}

LRESULT CRegistryManagerView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {

	LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

	SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

	using PGetDpiForWindow = UINT(__stdcall*)(HWND);
	static PGetDpiForWindow pGetDpiForWindow = (PGetDpiForWindow)::GetProcAddress(::GetModuleHandle(L"user32"), "GetDpiForWindow");
	auto dpi = pGetDpiForWindow ? pGetDpiForWindow(m_hWnd) : 96;
	InsertColumn(0, L"Name", LVCFMT_LEFT, 300 * dpi / 96);
	InsertColumn(1, L"Type", LVCFMT_LEFT, 100 * dpi / 96);
	InsertColumn(2, L"Size (Bytes)", LVCFMT_RIGHT, 100 * dpi / 96);
	InsertColumn(3, L"Value", LVCFMT_LEFT, 230 * dpi / 96);
	InsertColumn(4, L"Details", LVCFMT_LEFT, 200 * dpi / 96);
	InsertColumn(5, L"Last Write", LVCFMT_LEFT, 150 * dpi / 96);

	return 0;
}

const std::wstring& ListItem::GetName() const {
	if (Name.empty()) {
		if (TreeNode) {
			if (UpDir) // 上级目录
				Name = L"..";
			else
				Name = TreeNode->GetText();
		}
		else
			Name = *ValueName == L'\0' ? L"(Default)": ValueName.GetString();
	}
	return Name;
}

const std::wstring& ListItem::GetType() const {
	if (Type.empty()) {
		if (TreeNode)
			Type = L"Key";
		else
			Type = CRegistryManagerView::GetRegTypeAsString(ValueType);
	}
	return Type;
}

void CRegistryManagerView::Update(TreeNodeBase* node, bool ifTheSame) {
	if (node == nullptr)
		node = m_CurrentNode;
	if (ifTheSame && m_CurrentNode != node)
		return;

	auto currentSelected = GetSelectedIndex();

	m_CurrentNode = node;
	node->Expand(true);
	auto& nodes = node->GetChildNodes();
	m_Items.clear();

	// add up dir
	if (m_CurrentNode->GetParent()) {
		ListItem item;
		item.TreeNode = m_CurrentNode->GetParent();
		item.UpDir = true;
		m_Items.push_back(item);
	}

	if (nodes.empty()) {
		SetItemCount(static_cast<int>(m_Items.size()));
	}

	m_Items.reserve(nodes.size() + 64);
	BYTE buffer[1 << 12]; // 4kb

	auto info = reinterpret_cast<KEY_FULL_INFORMATION*>(buffer);
	if (m_ViewKeys) {
		for (auto node : nodes) {
			ListItem item;
			item.TreeNode = node;
			if (node->GetNodeType() == TreeNodeType::RegistryKey) {
				auto trueNode = static_cast<RegKeyTreeNode*>(node);
				auto key = trueNode->GetKey();
				if (key) {
					ULONG len;
					auto status = ::NtQueryKey(key->m_hKey, KeyFullInformation, info, sizeof(buffer), &len);
					if (NT_SUCCESS(status))
						item.LastWriteTime = info->LastWriteTime;
				}
			}
			m_Items.emplace_back(item);
		}
	}
	if (m_CurrentNode->GetNodeType() == TreeNodeType::RegistryKey) {
		auto trueNode = static_cast<RegKeyTreeNode*>(m_CurrentNode);
		auto key = trueNode->GetKey();
		if (key && key->m_hKey) {
			WCHAR valueName[256];
			DWORD valueType, size = 0, nameLen;
			for (int index = -1;; ++index) {
				nameLen = 256;
				if (ERROR_NO_MORE_ITEMS == ::RegEnumValue(key->m_hKey, index >= 0 ? index : 0,
					valueName, &nameLen, nullptr, &valueType, nullptr, &size)) {
					if (index >= 0)
						break;
				}
				if (index < 0 && *valueName != L'\0') {
					// add a default value
					valueName[0] = L'\0';
					valueType = REG_SZ;
					size = 0;
				}
				else if (index < 0) {
					index++;
				}
				ListItem item;
				item.ValueName = valueName;
				item.ValueType = valueType;
				item.ValueSize = size;
				m_Items.push_back(item);
			}
			if (!trueNode->GetLinkPath().IsEmpty()) {
				// add link path
				ListItem item;
				item.ValueName = L"SymbolicLinkName";
				item.ValueType = REG_LINK;
				item.ValueSize = trueNode->GetLinkPath().GetLength() * sizeof(WCHAR);
				m_Items.push_back(item);
			}
		}
	}
	int count = static_cast<int>(m_Items.size());
	SetItemCountEx(count, ifTheSame?(LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL) : 0);
	RedrawItems(0, min(count, GetCountPerPage()));
	DoSort(GetSortInfo());
}

const ListItem& CRegistryManagerView::GetItem(int index) const {
	ATLASSERT(index >= 0 && index < m_Items.size());
	return m_Items[index];
}

ListItem& CRegistryManagerView::GetItem(int index) {
	ATLASSERT(index >= 0 && index < m_Items.size());
	return m_Items[index];
}

void CRegistryManagerView::Init(ITreeOperations* to, IMainApp* app) {
	m_TreeOperations = to;
	m_App = app;
}

CString CRegistryManagerView::GetDataAsString(const ListItem& item) const {
	ATLASSERT(m_CurrentNode && m_CurrentNode->GetNodeType() == TreeNodeType::RegistryKey);
	auto regNode = static_cast<RegKeyTreeNode*>(m_CurrentNode);

	ULONG realSize = item.ValueSize;
	ULONG size = (realSize > (1 << 12) ? (1 << 12) : realSize) / sizeof(WCHAR);
	LRESULT status;
	CString text;

	switch (item.ValueType) {
		case REG_SZ:
		case REG_EXPAND_SZ:
			text.Preallocate(size + 1);
			status = regNode->GetKey()->QueryStringValue(item.ValueName, text.GetBuffer(), &size);
			break;

		case REG_LINK:
			text = regNode->GetLinkPath();
			break;

		case REG_MULTI_SZ:
			text.Preallocate(size + 1);
			status = regNode->GetKey()->QueryMultiStringValue(item.ValueName, text.GetBuffer(), &size);
			if (status == ERROR_SUCCESS) {
				auto p = text.GetBuffer();
				while (*p) {
					p += ::wcslen(p);
					*p = L' ';
					p++;
				}
			}
			break;

		case REG_DWORD:
		{
			DWORD value;
			if (ERROR_SUCCESS == regNode->GetKey()->QueryDWORDValue(item.ValueName, value)) {
				text.Format(L"0x%08X (%u)", value, value);
			}
			break;
		}

		case REG_QWORD:
		{
			ULONGLONG value;
			if (ERROR_SUCCESS == regNode->GetKey()->QueryQWORDValue(item.ValueName, value)) {
				auto fmt = value < (1LL << 32) ? L"%0x%08X (%llu)" : L"0x%016llX (%llu)";
				text.Format(fmt, value, value);
			}
			break;
		}

		case REG_BINARY:
		{
			CString digit;
			auto buffer = std::make_unique<BYTE[]>(item.ValueSize);
			if (buffer == nullptr)
				break;
			ULONG bytes = item.ValueSize;
			auto status = regNode->GetKey()->QueryBinaryValue(item.ValueName, buffer.get(), &bytes);
			if (status == ERROR_SUCCESS) {
				for (DWORD i = 0; i < min(bytes, 64); i++) {
					digit.Format(L"%02X", buffer[i]);
					text += digit;
				}
			}
		}
		break;
	}

	return text.GetLength() < 1024 ? text : text.Mid(0, 1024);
}

CString CRegistryManagerView::GetKeyDetails(TreeNodeBase* node) {
	CString text;
	if (node->GetNodeType() == TreeNodeType::RegistryKey) {
		auto keyNode = static_cast<RegKeyTreeNode*>(node);
		BYTE buffer[1024];
		auto info = reinterpret_cast<KEY_FULL_INFORMATION*>(buffer);
		ULONG len;
		auto status = ::NtQueryKey(*keyNode->GetKey(), KeyFullInformation, info, sizeof(info), &len);
		if (NT_SUCCESS(status)) {
			text.Format(L"SubKeys: %d Values: %d\n", info->SubKeys, info->Values);
		}
	}
	return text;
}

CString CRegistryManagerView::GetColumnText(HWND hWnd, int row, int column) const {
	auto& data = GetItem(row);
	CString text;

	switch (column) {
		case 0: // name
			return data.GetName().c_str();
		
		case 1: // type
			return data.GetType().c_str();

		case 2:// size
			if (data.TreeNode == nullptr) {
				text.Format(L"%u", data.ValueSize);
			}
			break;

		case 3:// data
			if (data.TreeNode == nullptr)
				return GetDataAsString(data);
			break;

		case 4: // details
			if (data.TreeNode && !data.UpDir)
				return GetKeyDetails(data.TreeNode);
			break;

		case 5:// last write
			if (data.TreeNode && data.LastWriteTime.QuadPart > 0) {
				CTime dt((FILETIME&)data.LastWriteTime);
				return dt.Format(L"%c");//适用于区域设置的日期和时间表示
			}
			break;
	}

	return text;
}

void CRegistryManagerView::DoSort(const SortInfo* si) {
	if (si == nullptr)
		return;

	std::sort(m_Items.begin(), m_Items.end(), [si](const auto& i1, const auto& i2) {
		return CompareItems(i1, i2, si->SortColumn, si->SortAscending);
		});
	RedrawItems(GetTopIndex(), GetTopIndex() + GetCountPerPage());
}

PCWSTR CRegistryManagerView::GetRegTypeAsString(DWORD type) {
	switch (type) {
		case REG_SZ: return L"REG_SZ";
		case REG_DWORD: return L"REG_DWORD";
		case REG_MULTI_SZ: return L"REG_MULTI_SZ";
		case REG_QWORD: return L"REG_QDWORD";
		case REG_EXPAND_SZ: return L"REG_EXPAND_SZ";
		case REG_NONE: return L"REG_NONE";
		case REG_LINK: return L"REG_LINK";
		case REG_BINARY: return L"REG_BINARY";
		case REG_RESOURCE_REQUIREMENTS_LIST: return L"REG_RESOURCE_REQUIREMENTS_LIST";
		case REG_RESOURCE_LIST: return L"REG_RESOURCE_LIST";
		case REG_FULL_RESOURCE_DESCRIPTOR: return L"REG_FULL_RESOURCE_DESCRIPTOR";
	}
	return L"Unknown";
}

LRESULT CRegistryManagerView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	CPoint pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	CPoint client(pt);
	ScreenToClient(&client);

	UINT flags;
	int index = HitTest(client, &flags);
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	CMenuHandle handle = menu.GetSubMenu(index < 0 ? 2 : 3);
	m_App->TrackPopupMenu(handle, pt.x, pt.y);
	return 0;
}

LRESULT CRegistryManagerView::OnReturnKey(int, LPNMHDR, BOOL& handled) {
	int selected = GetSelectedIndex();
	const auto& item = GetItem(selected);
	if (item.TreeNode) {
		if (item.UpDir)
			m_TreeOperations->SelectNode(m_CurrentNode->GetParent(), nullptr);
		else
			// key
			m_TreeOperations->SelectNode(m_CurrentNode, item.TreeNode->GetText());
	}
	else {
		return OnModifyValue(0, 0, nullptr, handled);
	}
	return 0;
}

LRESULT CRegistryManagerView::OnModifyValue(WORD, WORD, HWND, BOOL&) {
	auto selected = GetSelectedIndex();
	ATLASSERT(selected >= 0);
	auto& item = GetItem(selected);
	ATLASSERT(item.TreeNode == nullptr);
	ATLASSERT(m_CurrentNode->GetNodeType() == TreeNodeType::RegistryKey);

	auto regNode = static_cast<RegKeyTreeNode*>(m_CurrentNode);
	auto key = regNode->GetKey();

	switch (item.ValueType) {
	case REG_DWORD:
	case REG_QWORD:
	{
		CIntValueDlg dlg(m_App->IsAllowModify());
		ULONGLONG value = 0;
		auto error = (item.ValueType == REG_DWORD) ? key->QueryDWORDValue(item.ValueName, (DWORD&)value) : key->QueryQWORDValue(item.ValueName, value);
		if (error != ERROR_SUCCESS) {
			m_App->ShowCommandError(L"Failed to read value");
			return 0;
		}
		dlg.SetValue(value);
		dlg.SetName(item.ValueName, true);
		if (dlg.DoModal() == IDOK && value != dlg.GetRealValue()) {
			auto cmd = std::make_shared<ChangeValueCommand<ULONGLONG>>(m_CurrentNode->GetFullPath(), item.ValueName, dlg.GetRealValue(), item.ValueType);
			if (!m_App->AddCommand(cmd))
				m_App->ShowCommandError(L"Failed to change value");
		}
		break;
	}

	case REG_SZ:
	case REG_EXPAND_SZ:
	{
		CStringValueDlg dlg(m_App->IsAllowModify());
		dlg.SetName(item.ValueName, true);
		auto currentType = item.ValueType == REG_SZ ? 0 : 1;
		dlg.SetType(currentType);
		WCHAR value[2048] = { 0 };
		ULONG chars = 2048;
		auto error = key->QueryStringValue(item.ValueName, value, &chars);
		if (error != ERROR_SUCCESS && !item.ValueName.IsEmpty()) {
			m_App->ShowCommandError(L"Failed to read value");
			return 0;
		}
		dlg.SetValue(value);
		if (dlg.DoModal() == IDOK && (dlg.GetValue() != value || dlg.GetType() != currentType)) {
			auto type = dlg.GetType() == 0 ? REG_SZ : REG_EXPAND_SZ;
			auto cmd = std::make_shared<ChangeValueCommand<CString>>(m_CurrentNode->GetFullPath(), item.ValueName, dlg.GetValue(), type);
			if (!m_App->AddCommand(cmd))
				m_App->ShowCommandError(L"Failed to change value");
			else {
				item.ValueType = type;
				item.ValueSize = (1 + dlg.GetValue().GetLength()) * sizeof(WCHAR);
			}
		}
		break;
	}

	case REG_MULTI_SZ:
	{
		ULONG chars = item.ValueSize / sizeof(WCHAR);
		auto buffer = std::make_unique<WCHAR[]>(chars);
		if (ERROR_SUCCESS != key->QueryMultiStringValue(item.ValueName, buffer.get(), &chars)) {
			m_App->ShowCommandError(L"Failed to read value. Try Refreshing");
			break;
		}
		for (size_t i = 0; i < chars - 1; i++)
			if (buffer[i] == L'\0')
				buffer[i] = L'\n';

		CMultiStringValueDlg dlg(m_App->IsAllowModify());
		auto value = CString(buffer.get());
		value.Replace(L"\n", L"\r\n");
		dlg.SetName(item.ValueName, true);
		dlg.SetValue(value);
		if (dlg.DoModal() == IDOK) {
			value = dlg.GetValue();
			value.TrimLeft(L"\r\n");
			value.Replace(L"\r\n", L"\x01");
			auto buffer = value.GetBuffer();
			for (int i = 0; i < value.GetLength(); i++)
				if (buffer[i] == 1) {
					buffer[i] = 0;
				}

			auto cmd = std::make_shared<ChangeValueCommand<CString>>(m_CurrentNode->GetFullPath(), item.ValueName, value, item.ValueType);
			if (!m_App->AddCommand(cmd))
				m_App->ShowCommandError(L"Failed to change value");
			else
				item.ValueSize = (1 + dlg.GetValue().GetLength()) * sizeof(WCHAR);
		}
		break;
	}

	case REG_BINARY:
	{
		auto data = std::make_unique<BYTE[]>(item.ValueSize);
		ATLASSERT(data);
		ULONG size = item.ValueSize;
		if (ERROR_SUCCESS != key->QueryBinaryValue(item.ValueName, data.get(), &size)) {
			m_App->ShowCommandError(L"Failed to read binary value");
			break;
		}
		CBinaryValueDlg dlg(m_App->IsAllowModify());
		InMemoryBuffer buffer;
		buffer.SetData(0, data.get(), item.ValueSize);
		dlg.SetBuffer(&buffer);
		dlg.SetName(item.ValueName, true);
		if (dlg.DoModal() == IDOK) {
			BinaryValue value;
			value.Size = static_cast<DWORD>(buffer.GetSize());
			value.Buffer = std::make_unique<BYTE[]>(buffer.GetSize());
			::memcpy(value.Buffer.get(), buffer.GetRawData(0), value.Size);
			auto cmd = std::make_shared<ChangeValueCommand<BinaryValue>>(m_CurrentNode->GetFullPath(), item.ValueName, value);
			if (m_App->AddCommand(cmd))
				item.ValueSize = static_cast<DWORD>(buffer.GetSize());
			else
				m_App->ShowCommandError(L"Failed to change binary value");
		}
		break;
	}

	default:
		ATLASSERT(false);
		break;
	}

	RedrawItems(selected, selected);

	return 0;
}

LRESULT CRegistryManagerView::OnDoubleClick(int, LPNMHDR nmhdr, BOOL& handled) {
	auto lv = reinterpret_cast<NMITEMACTIVATE*>(nmhdr);
	if (m_Items.empty())
		return 0;

	const auto& item = GetItem(lv->iItem);
	if (item.TreeNode) {
		if (item.UpDir)
			m_TreeOperations->SelectNode(m_CurrentNode->GetParent(), nullptr);
		else
			// key
			m_TreeOperations->SelectNode(m_CurrentNode, item.TreeNode->GetText());
	}
	else {
		return OnModifyValue(0, 0, nullptr, handled);
	}

	return 0;
}

LRESULT CRegistryManagerView::OnRightClick(int, LPNMHDR hdr, BOOL&) {
	auto lv = (NMITEMACTIVATE*)hdr;
	CMenu menu;
	menu.LoadMenuW(IDR_CONTEXT);
	CPoint pt;
	::GetCursorPos(&pt);
	int index = lv->iItem < 0 ? 1 : 2;
	m_App->TrackPopupMenu(menu.GetSubMenu(index), pt.x, pt.y);

	return 0;
}

LRESULT CRegistryManagerView::OnBeginRename(int, LPNMHDR, BOOL&) {
	if (!m_App->IsAllowModify())
		return TRUE;

	m_Edit = GetEditControl();
	ATLASSERT(m_Edit.IsWindow());
	return 0;
}

LRESULT CRegistryManagerView::OnEndRename(int, LPNMHDR, BOOL&) {
	ATLASSERT(m_Edit.IsWindow());
	CString newName;
	m_Edit.GetWindowText(newName);

	int index = GetSelectedIndex();
	auto& item = GetItem(index);
	if (newName.CompareNoCase(item.ValueName) == 0)
		return 0;

	auto cmd = std::make_shared<RenameValueCommand>(m_CurrentNode->GetFullPath(), item.ValueName, newName);
	if (!m_App->AddCommand(cmd))
		m_App->ShowCommandError(L"Failed to rename value");
	else {
		item.ValueName = newName;
		Update(m_CurrentNode);
	}
	return 0;
}

LRESULT CRegistryManagerView::OnItemChanged(int, LPNMHDR, BOOL&) {
	auto selected = GetSelectedCount();
	auto ui = m_App->GetUIUpdate();
	ui->UIEnable(ID_EDIT_COPY, selected >= 0);

	return 0;
}

LRESULT CRegistryManagerView::OnDelete(WORD, WORD, HWND, BOOL&) {
	return LRESULT();
}

LRESULT CRegistryManagerView::OnEditRename(WORD, WORD, HWND, BOOL&) {
	EditLabel(GetSelectedIndex());

	return 0;
}

LRESULT CRegistryManagerView::OnEditCopy(WORD, WORD, HWND, BOOL&) {
	auto index = GetSelectedIndex();
	ATLASSERT(index >= 0);
	CString item, text;
	for (int i = 0; i < 5; i++) {
		GetItemText(index, i, item);
		if (!item.IsEmpty())
			text += item + L", ";
	}
	ClipboardHelper::CopyText(*this, text.Left(text.GetLength() - 2));

	return 0;
}

LRESULT CRegistryManagerView::OnNewDwordValue(WORD, WORD, HWND, BOOL&) {
	return HandleNewIntValue(4);
}

LRESULT CRegistryManagerView::OnNewQwordValue(WORD, WORD, HWND, BOOL&) {
	return HandleNewIntValue(8);
}

LRESULT CRegistryManagerView::OnNewStringValue(WORD, WORD, HWND, BOOL&) {
	return HandleNewStringValue(REG_SZ);
}

LRESULT CRegistryManagerView::OnNewExpandStringValue(WORD, WORD, HWND, BOOL&) {
	return HandleNewStringValue(REG_EXPAND_SZ);
}

LRESULT CRegistryManagerView::OnNewMultiStringValue(WORD, WORD, HWND, BOOL&) {
	ATLASSERT(m_App->IsAllowModify());
	CMultiStringValueDlg dlg(true);
	dlg.SetName(L"", false);
	if (dlg.DoModal() == IDOK) {
		auto cmd = std::make_shared<CreateNewValueCommand<CString>>(m_CurrentNode->GetFullPath(),
			dlg.GetName(), dlg.GetValue(), REG_MULTI_SZ);
		if (!m_App->AddCommand(cmd))
			m_App->ShowCommandError(L"Failed to create value");
		else {
			auto index = FindItem(dlg.GetName(), false, true);
			ATLASSERT(index >= 0);
			SelectItem(index);
		}
	}
	return 0;
}

LRESULT CRegistryManagerView::OnNewBinaryValue(WORD, WORD, HWND, BOOL&) {
	CBinaryValueDlg dlg(true);
	dlg.SetName(L"", false);
	InMemoryBuffer buffer;
	dlg.SetBuffer(&buffer);
	if (dlg.DoModal() == IDOK) {
		BinaryValue value;
		value.Size = (unsigned)buffer.GetSize();
		value.Buffer = std::make_unique<BYTE[]>(value.Size);
		buffer.GetData(0, value.Buffer.get(), value.Size);
		auto cmd = std::make_shared<CreateNewValueCommand<BinaryValue>>(m_CurrentNode->GetFullPath(),
			dlg.GetName(), value, REG_BINARY);
		if (!m_App->AddCommand(cmd))
			m_App->ShowCommandError(L"Failed to create value");
		else {
			auto index = FindItem(dlg.GetName(), false, true);
			ATLASSERT(index >= 0);
			SelectItem(index);
		}
	}
	return 0;
}

LRESULT CRegistryManagerView::HandleNewIntValue(int size) {
	ATLASSERT(size == 4 || size == 8);

	CIntValueDlg dlg(true);
	dlg.SetValue(0);
	dlg.SetName(L"", false);
	if (dlg.DoModal() == IDOK) {
		if (m_CurrentNode->FindChild(dlg.GetName())) {
			m_App->ShowCommandError(L"Value name already exists");
			return 0;
		}

		auto cmd = std::make_shared<CreateNewValueCommand<ULONGLONG>>(m_CurrentNode->GetFullPath(),
			dlg.GetName(), dlg.GetRealValue(), size == 4 ? REG_DWORD : REG_QWORD);
		if (!m_App->AddCommand(cmd))
			m_App->ShowCommandError(L"Failed to create value");
		else {
			SelectItem(static_cast<int>(m_Items.size()) - 1);
		}
	}

	return 0;
}

LRESULT CRegistryManagerView::HandleNewStringValue(DWORD type) {
	ATLASSERT(type == REG_SZ || type == REG_EXPAND_SZ);

	CStringValueDlg dlg(true);
	dlg.SetName(L"", false);
	dlg.SetType(type == REG_SZ ? 0 : 1);
	if (dlg.DoModal() == IDOK) {
		if (m_CurrentNode->FindChild(dlg.GetName())) {
			m_App->ShowCommandError(L"Value name already exists");
			return 0;
		}

		auto cmd = std::make_shared<CreateNewValueCommand<CString>>(m_CurrentNode->GetFullPath(),
			dlg.GetName(), dlg.GetValue(), dlg.GetType() == 0 ? REG_SZ : REG_EXPAND_SZ);
		if (!m_App->AddCommand(cmd))
			m_App->ShowCommandError(L"Failed to create value");
		else {
			auto index = FindItem(dlg.GetName(), false, true);
			ATLASSERT(index >= 0);
			SelectItem(index);
		}

	}

	return 0;
}

bool CRegistryManagerView::CanEditValue() const {
	auto selected = GetSelectedIndex();
	if (selected < 0)
		return false;

	return GetItem(selected).TreeNode == nullptr;
}

bool CRegistryManagerView::IsViewKeys() const {
	return m_ViewKeys;
}

bool CRegistryManagerView::CanDeleteSelected() const {
	if (GetSelectedCount() == 0)
		return false;

	auto& item = GetItem(GetSelectedIndex());
	if (item.TreeNode == nullptr)
		return true;
	return item.TreeNode->CanDelete();
}

LRESULT CRegistryManagerView::OnFindItem(int, LPNMHDR hdr, BOOL&) {
	auto fi = (NMLVFINDITEM*)hdr;

	auto count = GetItemCount();
	auto str = fi->lvfi.psz;
	for (int i = fi->iStart; i < fi->iStart + count; i++) {
		auto& item = m_Items[i % count];
		if (item.TreeNode && ::_wcsnicmp(item.TreeNode->GetText(), str, ::wcslen(str)) == 0)
			return i % count;
		else if (item.TreeNode == nullptr && ::_wcsnicmp(item.ValueName, str, ::wcslen(str)) == 0)
			return i % count;
	}

	return -1;
}