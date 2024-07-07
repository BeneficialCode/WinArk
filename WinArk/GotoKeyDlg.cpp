#include "stdafx.h"
#include "resource.h"
#include "GotoKeyDlg.h"
#include "SortHelper.h"


const CString& CGotoKeyDlg::GetKey() const {
	return m_Key;
}

void CGotoKeyDlg::SetKey(const CString& key) {
	m_Key = key;
}

LRESULT CGotoKeyDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());

	GetDlgItem(IDOK).EnableWindow(!m_Key.IsEmpty());

	m_List.Attach(GetDlgItem(IDC_KEY_LIST));
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	::SetWindowTheme(m_List, L"Explorer", L"");

	auto cm = GetColumnManager(m_List);

	cm->AddColumn(L"Name", 0, 150);
	cm->AddColumn(L"Path", 0, 650);
	cm->UpdateColumns();

	const struct {
		PCWSTR name;
		PCWSTR path;
	} locations[] = {
		{ L"Services", LR"(HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services)" },
		{ L"Hardware", LR"(HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum)" },
		{ L"Device Classes", LR"(HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class)" },
		{ L"Hive List", LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\hivelist)" },
		{ L"Image File Execution Options", LR"(HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options)" },
		{ L"Explorer Context Menu 1",LR"(HKEY_CLASSES_ROOT\*\shell)"},
		{ L"Explorer Context Menu 2",LR"(HKEY_CLASSES_ROOT\*\shellex)"},
		{ L"Explorer Context Menu 3",LR"(HKEY_CLASSES_ROOT\AllFileSystemObjects\ShellEx\ContextMenuHandlers)"},
		{ L"Explorer Context Menu 4",LR"(HKEY_CLASSES_ROOT\Folder\shell)" },
		{ L"Explorer Context Menu 5",LR"(HKEY_CLASSES_ROOT\Folder\shellex\ContextMenuHandlers)" },
		{ L"Explorer Context Menu 6",LR"(HKEY_CLASSES_ROOT\Directory\shell)" },
		{ L"Explorer Context Menu 7",LR"(HKEY_CLASSES_ROOT\Directory\Background\shell)" },
		{ L"Explorer Context Menu 8",LR"(HKEY_CLASSES_ROOT\Directory\Background\shellex\ContextMenuHandlers)" },
		{ L"Run 1",LR"(HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)"},
		{ L"Run 2",LR"(HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)"},
		{ L"Run 3",LR"(HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run)"},
		{ L"Run 4",LR"(HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run)"},
		{ L"RunOnce 1",LR"(HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce)"},
		{ L"RunOnce 2",LR"(HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce)"},
		{ L"RunOnceEx 1",LR"(HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnceEx)"},
		{ L"RunOnceEx 2",LR"(HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnceEx)"},
		{ L"Application Registration",LR"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths)"},
		{ L"Eventlog",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog)"},
		{ L"ServiceGroupOrder",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ServiceGroupOrder)"},
		{ L"Winlogon",LR"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon)"},
		{ L"Winmgmt",LR"(HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\Winmgmt)"},
		{ L"DisallowRun",LR"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun)"},
		{ L"Session Manager",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager)"},
		{ L"Zip New Shell",LR"(HKEY_CLASSES_ROOT\.zip\CompressedFolder\ShellNew)"},
		{ L"tab completion",LR"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Command Processor)"},
		{ L"Service Group Order",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ServiceGroupOrder)"},
		{ L"Group Order List",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GroupOrderList)"},
		{ L"WinHttpAutoProxySvc",LR"(HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\WinHttpAutoProxySvc)"},
		{ L"VulnerableDriverBlocklistEnable",LR"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\CI\Config)"},
		{ L"Known Dlls",LR"(HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs)"},
		{ L"Lsa",LR"(HKLM\SYSTEM\CurrentControlSet\Control\Lsa)"},
		{ L"LogonUI",LR"(HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI)"},
		{ L"Credential Providers",LR"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers)"},
		{ L"DisallowRun",LR"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun)"},
		{ L"DisablePath",LR"(HKEY_CURRENT_USER\SOFTWARE\Policies\Microsoft\Windows\Safer\CodeIdentifiers\0\Paths)"},
		{ L"Internet Settings",LR"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings)"},
		{ L"Session Manager",LR"(HKLM\System\CurrentControlSet\Control\Session Manager)"},
	};

	for (const auto& [name, path] : locations) {
		ListItem item{ name,path};
		m_Items.push_back(item);
	}
	UpdateList(m_List, static_cast<int>(m_Items.size()));

	return 0;
}

LRESULT CGotoKeyDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDOK) {
		auto index = m_List.GetSelectedIndex();
		ATLASSERT(index >= 0);
		auto location = m_Items[index];
		ATLASSERT(location.Path.GetLength() > 0);
		SetKey(location.Path);
	}
	EndDialog(wID);
	return 0;
}

void CGotoKeyDlg::DoSort(const SortInfo* si) {
	if (si == nullptr)
		return;

	auto compare = [&](auto& i1, auto& i2) {
		switch (si->SortColumn) {
			case 0: return SortHelper::SortStrings(i1.Name, i2.Name, si->SortColumn);
			case 1: return SortHelper::SortStrings(i1.Path, i2.Path, si->SortColumn);
		}
		return false;
	};

	std::sort(m_Items.begin(), m_Items.end(), compare);
}

BOOL CGotoKeyDlg::OnDoubleClickList(HWND, int row, int, const POINT&) {
	if (row < 0)
		return FALSE;

	auto& item = m_Items[row];
	SetKey(item.Path);
	EndDialog(IDOK);
	return 0;
}

CString CGotoKeyDlg::GetColumnText(HWND, int row, int col) const {
	auto& item = m_Items[row];

	switch (col){
		case 0: return item.Name;
		case 1: return item.Path;
	}
	ATLASSERT(false);
	return L"";
}

LRESULT CGotoKeyDlg::OnKeyItemChanged(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
	auto index = m_List.GetSelectedIndex();
	BOOL enable = FALSE;
	if (index >= 0) {
		auto& location = m_Items[index];
		enable = location.Path.GetLength() > 0 ? TRUE : FALSE;
		SetKey(location.Path);
	}
	GetDlgItem(IDOK).EnableWindow(enable);
	return 0;
}