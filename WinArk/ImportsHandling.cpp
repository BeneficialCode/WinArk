#include "stdafx.h"
#include "resource.h"
#include "ImportsHandling.h"
#include "Architecture.h"

void ImportsHandling::SetItemData(CTreeItem item, const TreeItemData* pData)
{
	ItemDataMap[item] = *pData;
}

ImportsHandling::TreeItemData* ImportsHandling::GetItemData(CTreeItem item) {
	std::unordered_map<HTREEITEM, TreeItemData>::iterator it;
	it = ItemDataMap.find(item);
	if (it != ItemDataMap.end())
	{
		return &it->second;
	}
	return nullptr;
}

void ImportsHandling::UpdateCounts()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	m_ThunkCount = m_InvalidThunkCount = m_SuspectThunkCount = 0;

	it_module = ModuleMap.begin();
	while (it_module != ModuleMap.end())
	{
		ImportModuleThunk& moduleThunk = it_module->second;

		it_import = moduleThunk.ThunkMap.begin();
		while (it_import != moduleThunk.ThunkMap.end())
		{
			ImportThunk& importThunk = it_import->second;

			m_ThunkCount++;
			if (!importThunk.Valid)
				m_InvalidThunkCount++;
			else if (importThunk.Suspect)
				m_SuspectThunkCount++;

			it_import++;
		}

		it_module++;
	}
}

CTreeItem ImportsHandling::AddDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, ImportModuleThunk* moduleThunk) {
	CTreeItem item = idTreeView.InsertItem(L"", NULL, TVI_ROOT);

	item.SetData(ItemDataMap.size());

	TreeItemData data;
	data.IsModule = true;
	data.Module = moduleThunk;

	SetItemData(item, &data);

	UpdateModuleInTreeView(moduleThunk, item);
	return item;
}

void ImportsHandling::UpdateModuleInTreeView(const ImportModuleThunk* importThunk, CTreeItem item) {
	swprintf_s(Text, L"%s (%d) FThunk: " PRINTF_DWORD_PTR_HALF, importThunk->ModuleName, importThunk->ThunkMap.size(), importThunk->FirstThunk);

	item.SetText(Text);
	Icon icon = GetAppropiateIcon(importThunk->IsValid());
	
}

ImportsHandling::Icon ImportsHandling::GetAppropiateIcon(bool valid) {
	return valid ? iconCheck : iconError;
}

ImportsHandling::ImportsHandling(CMultiSelectTreeViewCtrl& treeImports):TreeImports(treeImports) {
	CDCHandle dc = CWindow(::GetDesktopWindow()).GetDC();
	int bits = dc.GetDeviceCaps(BITSPIXEL);
	const UINT flags = bits > 16 ? ILC_COLOR32 : (ILC_COLOR24 | ILC_MASK);
	TreeIcons.Create(16, 16, flags, 3, 1);

	UINT icons[] = {
		IDI_CHECK,IDI_SCYLLA_WARNING,IDI_SCYLLA_ERROR
	};

	for (UINT icon : icons) {
		TreeIcons.AddIcon(AtlLoadIconImage(icon, 0, 16, 16));
	}
}

ImportsHandling::~ImportsHandling() {
	TreeIcons.Destroy();
}