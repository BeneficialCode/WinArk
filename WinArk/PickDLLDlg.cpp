#include "stdafx.h"
#include "PickDLLDlg.h"
#include "Architecture.h"

CPickDLLDlg::CPickDLLDlg(std::vector<ModuleInfo>& moduleList):_moduleList(moduleList)
{
}

BOOL CPickDLLDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange();
	DlgResize_Init(true, true);

	AddColumnsToModuleList(_DLLsListView);
	DisplayModuleList(_DLLsListView);

	CenterWindow();
	return TRUE;
}

LRESULT CPickDLLDlg::OnListDllColumnClicked(NMHDR* pnmh)
{
	NMLISTVIEW* pListView = (NMLISTVIEW*)pnmh;
	int column = pListView->iSubItem;
	if (column == _prevColumn) {
		_ascending = !_ascending;
	}
	else {
		_prevColumn = column;
		_ascending = true;
	}

	_DLLsListView.SortItems(&ListViewCompareFunc, MAKEWORD(column, _ascending));
	return 0;
}

LRESULT CPickDLLDlg::OnListDllDoubleClick(NMHDR* pnmh)
{
	NMITEMACTIVATE* pItemActivate = (NMITEMACTIVATE*)pnmh;
	LVHITTESTINFO hit;
	hit.pt = pItemActivate->ptAction;
	int clicked = _DLLsListView.HitTest(&hit);
	if (clicked != -1) {
		_pSelectedModule = (ModuleInfo*)_DLLsListView.GetItemData(clicked);
		EndDialog(1);
	}
	return 0;
}

void CPickDLLDlg::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int index = _DLLsListView.GetSelectionMark();
	if (index != -1) {
		_pSelectedModule = (ModuleInfo*)_DLLsListView.GetItemData(index);
		EndDialog(1);
	}
}

void CPickDLLDlg::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void CPickDLLDlg::AddColumnsToModuleList(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(Name, L"Name", LVCFMT_LEFT);
	list.InsertColumn(ImageBase, L"ImageBase", LVCFMT_CENTER);
	list.InsertColumn(ImageSize, L"ImageSize", LVCFMT_CENTER);
	list.InsertColumn(Path, L"Path", LVCFMT_LEFT);
}

void CPickDLLDlg::DisplayModuleList(CListViewCtrl& list)
{
	WCHAR temp[32];

	_DLLsListView.DeleteAllItems();

	int count = 0;

	for (auto& info : _moduleList) {
		_DLLsListView.InsertItem(count, info.GetFileName());
		swprintf_s(temp, PRINTF_DWORD_PTR_FULL, info._modBaseAddr);
		_DLLsListView.SetItemText(count, ImageBase, temp);

		swprintf_s(temp, L"%08X", info._modBaseSize);
		_DLLsListView.SetItemText(count, ImageSize, temp);

		_DLLsListView.SetItemText(count, Path, info._fullPath);
		_DLLsListView.SetItemData(count, (DWORD_PTR)&info);
	}

	_DLLsListView.SetColumnWidth(Name, LVSCW_AUTOSIZE_USEHEADER);
	_DLLsListView.SetColumnWidth(ImageBase, LVSCW_AUTOSIZE_USEHEADER);
	_DLLsListView.SetColumnWidth(ImageSize, LVSCW_AUTOSIZE_USEHEADER);
	_DLLsListView.SetColumnWidth(Path, LVSCW_AUTOSIZE_USEHEADER);
}

int CALLBACK CPickDLLDlg::ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	const ModuleInfo* p1 = (ModuleInfo*)lParam1;
	const ModuleInfo* p2 = (ModuleInfo*)lParam2;

	int column = LOBYTE(lParamSort);
	bool ascending = (HIBYTE(lParamSort) == TRUE);

	int diff = 0;
	switch (column)
	{
		case Name:
			diff = _wcsicmp(p1->GetFileName(), p2->GetFileName());
			break;
		case ImageBase:
			diff = p1->_modBaseAddr < p2->_modBaseAddr ? -1 : 1;
			break;
		case ImageSize:
			diff = p1->_modBaseSize < p2->_modBaseSize ? -1 : 1;
			break;
		case Path:
			diff = _wcsicmp(p1->_fullPath, p2->_fullPath);
			break;
	}

	return ascending ? diff : -diff;
}