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

	it_module = m_ModuleMap.begin();
	while (it_module != m_ModuleMap.end())
	{
		ImportModuleThunk& moduleThunk = it_module->second;

		it_import = moduleThunk.m_ThunkMap.begin();
		while (it_import != moduleThunk.m_ThunkMap.end())
		{
			ImportThunk& importThunk = it_import->second;

			m_ThunkCount++;
			if (!importThunk.m_Valid)
				m_InvalidThunkCount++;
			else if (importThunk.m_Suspect)
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

CTreeItem ImportsHandling::AddApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, ImportThunk* pImportThunk)
{
	CTreeItem item = idTreeView.InsertItem(L"", parentDll, TVI_LAST);
	item.SetData(ItemDataMap.size());

	TreeItemData data;
	data.IsModule = false;
	data.Import = pImportThunk;

	SetItemData(item, &data);

	UpdateImportInTreeView(pImportThunk, item);
	return item;
}

void ImportsHandling::UpdateImportInTreeView(const ImportThunk* pImportThunk, CTreeItem item)
{
	if (pImportThunk->m_Valid) {
		if (pImportThunk->m_Name[0] != 0x00) {
			swprintf_s(Text, L"ord: %04X name: %S", pImportThunk->m_Ordinal, pImportThunk->m_Name);
		}
		else {
			swprintf_s(Text, L"ord: %04X", pImportThunk->m_Ordinal);
		}

		swprintf_s(Text, L" rva: " PRINTF_DWORD_PTR_HALF L" mod: %s %s", pImportThunk->m_RVA, pImportThunk->m_ModuleName, Text);
	}
	else {
		swprintf_s(Text, L" rva: " PRINTF_DWORD_PTR_HALF L" ptr: " PRINTF_DWORD_PTR_FULL, pImportThunk->m_RVA, pImportThunk->m_ApiAddressVA);
	}

	item.SetText(Text);
	int icon = (int)GetAppropiateIcon(pImportThunk);
	item.SetImage(icon, icon);
}

void ImportsHandling::UpdateModuleInTreeView(const ImportModuleThunk* importThunk, CTreeItem item) {
	swprintf_s(Text, L"%s (%lld) FThunk: " PRINTF_DWORD_PTR_HALF, importThunk->m_ModuleName, importThunk->m_ThunkMap.size(), importThunk->m_FirstThunk);

	item.SetText(Text);
	int icon = (int)GetAppropiateIcon(importThunk->IsValid());
	item.SetImage(icon, icon);
}

ImportsHandling::Icon ImportsHandling::GetAppropiateIcon(const ImportThunk* pImportThunk)
{
	if (pImportThunk->m_Valid) {
		return pImportThunk->m_Suspect ? Icon::Warning : Icon::Check;
	}
	else {
		return Icon::Error;
	}
}

ImportsHandling::Icon ImportsHandling::GetAppropiateIcon(bool valid) {
	return valid ? ImportsHandling::Icon::Check : ImportsHandling::Icon::Error;
}

bool ImportsHandling::AddModuleToModuleList(const WCHAR* pModuleName, DWORD_PTR firstThunk)
{
	ImportModuleThunk module;
	module.m_FirstThunk = firstThunk;
	wcscpy_s(module.m_ModuleName, pModuleName);
	module.m_Key = module.m_FirstThunk;
	m_ModuleMapNew[module.m_Key] = module;
	return true;
}

void ImportsHandling::AddUnknownModuleToModuleList(DWORD_PTR firstThunk)
{
	ImportModuleThunk module;
	module.m_FirstThunk = firstThunk;
	wcscpy_s(module.m_ModuleName, L"?");
	module.m_Key = module.m_FirstThunk;
	m_ModuleMapNew[module.m_Key] = module;
}

bool ImportsHandling::AddNotFoundApiToModuleList(const ImportThunk* pApiNotFound)
{
	ImportThunk import;
	ImportModuleThunk* pModule = nullptr;
	DWORD_PTR rva = pApiNotFound->m_RVA;

	if (m_ModuleMapNew.size() > 0) {
		auto it_module = m_ModuleMapNew.begin();
		while (it_module != m_ModuleMapNew.end()) {
			if (rva >= it_module->second.m_FirstThunk) {
				it_module++;
				if (it_module == m_ModuleMapNew.end()) {
					it_module--;
					//new unknown module
					if (it_module->second.m_ModuleName[0] == L'?')
					{
						pModule = &(it_module->second);
					}
					else
					{
						AddUnknownModuleToModuleList(pApiNotFound->m_RVA);
						pModule = &(m_ModuleMapNew.find(rva)->second);
					}

					break;
				}
				else if (rva < it_module->second.m_FirstThunk) {
					it_module--;
					pModule = &(it_module->second);
					break;
				}
			}
			else {
				break;
			}
		}
	}
	else {
		AddUnknownModuleToModuleList(pApiNotFound->m_RVA);
		pModule = &(m_ModuleMapNew.find(rva)->second);
	}

	if (!pModule) {
		return false;
	}

	import.m_Suspect = true;
	import.m_Valid = false;
	import.m_VA = pApiNotFound->m_VA;
	import.m_ApiAddressVA = pApiNotFound->m_ApiAddressVA;
	import.m_Ordinal = 0;

	wcscpy_s(import.m_ModuleName, L"?");
	strcpy_s(import.m_Name, "?");

	import.m_Key = import.m_RVA;
	pModule->m_ThunkMap[import.m_Key] = import;
	return true;
}

bool ImportsHandling::AddFunctionToModuleList(const ImportThunk* pApiFound)
{
	ImportThunk import;
	ImportModuleThunk* pModule = nullptr;
	DWORD_PTR rva = pApiFound->m_RVA;
	auto it_module = m_ModuleMapNew.begin();
	if (m_ModuleMapNew.size() > 1) {
		while (it_module != m_ModuleMapNew.end()) {
			if (rva >= it_module->second.m_FirstThunk) {
				it_module++;
				if (it_module == m_ModuleMapNew.end()) {
					it_module--;
					pModule = &(it_module->second);
					break;
				}
				else if (rva < it_module->second.m_FirstThunk) {
					it_module--;
					pModule = &(it_module->second);
					break;
				}
			}
			else {
				break;
			}
		}
	}
	else {
		it_module = m_ModuleMapNew.begin();
		pModule = &(it_module->second);
	}

	if (!pModule) {
		return false;
	}

	import.m_Suspect = pApiFound->m_Suspect;
	import.m_Valid = pApiFound->m_Valid;
	import.m_VA = pApiFound->m_VA;
	import.m_ApiAddressVA = pApiFound->m_ApiAddressVA;
	import.m_Ordinal = pApiFound->m_Ordinal;
	import.m_Hint = pApiFound->m_Hint;

	wcscpy_s(import.m_ModuleName,pApiFound->m_ModuleName);
	strcpy_s(import.m_Name, pApiFound->m_Name);

	import.m_Key = import.m_RVA;
	pModule->m_ThunkMap[import.m_Key] = import;
	return true;
}

bool ImportsHandling::IsNewModule(const WCHAR* pModuleName)
{
	auto it_module = m_ModuleMapNew.begin();
	while (it_module != m_ModuleMapNew.end()) {
		if (!_wcsicmp(it_module->second.m_ModuleName, pModuleName))
			return false;
		it_module++;
	}
	return true;
}

void ImportsHandling::ChangeExpandStateOfTreeNodes(UINT flag)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	it_module = m_ModuleMap.begin();
	while (it_module != m_ModuleMap.end()) {
		ImportModuleThunk& moduleThunk = it_module->second;
		moduleThunk.m_TreeItem.Expand(flag);
		it_module++;
	}
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

bool ImportsHandling::IsModule(CTreeItem item)
{
	return (nullptr != GetModuleThunk(item));
}

bool ImportsHandling::IsImport(CTreeItem item)
{
	return (nullptr != GetImportThunk(item));
}

ImportModuleThunk* ImportsHandling::GetModuleThunk(CTreeItem item)
{
	std::unordered_map<HTREEITEM, TreeItemData>::const_iterator iter;
	iter = ItemDataMap.find(item);
	if (iter != ItemDataMap.end()) {
		const TreeItemData* pData = &iter->second;
		if (pData->IsModule) {
			return pData->Module;
		}
	}
	return nullptr;
}

ImportThunk* ImportsHandling::GetImportThunk(CTreeItem item)
{
	std::unordered_map<HTREEITEM, TreeItemData>::const_iterator iter;
	TreeItemData* pData = GetItemData(item);
	if (pData && !pData->IsModule) {
		return pData->Import;
	}
	return nullptr;
}

void ImportsHandling::DisplayAllImports()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	TreeImports.DeleteAllItems();
	ItemDataMap.clear();
	TreeImports.SetImageList(TreeIcons);

	it_module = m_ModuleMap.begin();
	while (it_module != m_ModuleMap.end()) {
		ImportModuleThunk& moduleThunk = it_module->second;

		moduleThunk.m_Key = moduleThunk.m_FirstThunk;
		moduleThunk.m_TreeItem = AddDllToTreeView(TreeImports, &moduleThunk);

		it_import = moduleThunk.m_ThunkMap.begin();
		while (it_import != moduleThunk.m_ThunkMap.end()) {
			ImportThunk& importThunk = it_import->second;

			importThunk.m_Key = importThunk.m_RVA;
			importThunk.m_TreeItem = AddApiToTreeView(TreeImports, moduleThunk.m_TreeItem, &importThunk);
			it_import++;
		}

		it_module++;
	}

	UpdateCounts();
}

void ImportsHandling::ClearAllImports()
{
	TreeImports.DeleteAllItems();
	ItemDataMap.clear();
	m_ModuleMap.clear();
	UpdateCounts();
}

void ImportsHandling::SelectImports(bool invalid, bool suspect)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	// remove selection
	TreeImports.SelectAllItems(false);

	it_module = m_ModuleMap.begin();
	while (it_module != m_ModuleMap.end())
	{
		ImportModuleThunk& moduleThunk = it_module->second;

		it_import = moduleThunk.m_ThunkMap.begin();
		while (it_import != moduleThunk.m_ThunkMap.end())
		{
			ImportThunk& importThunk = it_import->second;

			if ((invalid && !importThunk.m_Valid) || (suspect && importThunk.m_Suspect))
			{
				TreeImports.SelectItem(importThunk.m_TreeItem, TRUE);
				importThunk.m_TreeItem.EnsureVisible();
			}

			it_import++;
		}

		it_module++;
	}
}

bool ImportsHandling::InvalidateImport(CTreeItem item)
{
	ImportThunk* pImport = GetImportThunk(item);
	if (pImport) {
		CTreeItem parent = item.GetParent();
		if (!parent.IsNull()) {
			const ImportModuleThunk* pModule = GetModuleThunk(parent);
			if (pModule) {
				pImport->Invalidate();
				UpdateImportInTreeView(pImport, pImport->m_TreeItem);
				UpdateModuleInTreeView(pModule, pModule->m_TreeItem);

				UpdateCounts();
				return true;
			}
		}
	}
	return false;
}

bool ImportsHandling::InvalidateModule(CTreeItem item)
{
	ImportModuleThunk* pModule = GetModuleThunk(item);
	if (pModule) {
		std::map<DWORD_PTR, ImportThunk>::iterator it_import;

		it_import = pModule->m_ThunkMap.begin();
		while (it_import != pModule->m_ThunkMap.end()) {
			ImportThunk* pImport = &it_import->second;
			pImport->Invalidate();
			UpdateImportInTreeView(pImport, pImport->m_TreeItem);
			it_import++;
		}

		UpdateModuleInTreeView(pModule, pModule->m_TreeItem);

		UpdateCounts();
		return true;
	}
	return false;
}

bool ImportsHandling::SetImport(CTreeItem item, const WCHAR* pModuleName, const CHAR* pApiName, WORD ordinal, WORD hint, bool valid, bool suspect)
{
	ImportThunk* pImport = GetImportThunk(item);
	if (pImport)
	{
		CTreeItem parent = item.GetParent();
		if (!parent.IsNull()) {
			ImportModuleThunk* pModule = GetModuleThunk(parent);
			if (pModule) {
				wcscpy_s(pImport->m_ModuleName, pModuleName);
				strcpy_s(pImport->m_Name, pApiName);
				pImport->m_Hint = hint;
				pImport->m_Valid = valid;
				pImport->m_Suspect = suspect;

				if (pModule->IsValid()) {
					ScanAndFixModuleList();
					DisplayAllImports();
				}
				else {
					UpdateImportInTreeView(pImport, item);
					UpdateCounts();
				}
				return true;
			}
		}
	}
	return false;
}

bool ImportsHandling::CutImport(CTreeItem item)
{
	ImportThunk* pImport = GetImportThunk(item);
	if (pImport) {
		CTreeItem parent = item.GetParent();
		if (!parent.IsNull()) {
			ImportModuleThunk* pModule = GetModuleThunk(parent);
			if (pModule) {
				ItemDataMap.erase(item);
				pImport->m_TreeItem.Delete();
				pModule->m_ThunkMap.erase(pImport->m_Key);
				pImport = nullptr;

				if (pModule->m_ThunkMap.empty()) {
					ItemDataMap.erase(parent);
					pModule->m_TreeItem.Delete();
					m_ModuleMap.erase(pModule->m_Key);
					pModule = nullptr;
				}
				else {
					if (pModule->IsValid() && pModule->m_ModuleName[0] == L'?')
					{
						wcscpy_s(pModule->m_ModuleName, pModule->m_ThunkMap.begin()->second.m_ModuleName);
					}
					pModule->m_FirstThunk = pModule->m_ThunkMap.begin()->second.m_RVA;
					UpdateModuleInTreeView(pModule, pModule->m_TreeItem);
				}

				UpdateCounts();
				return true;
			}
		}
	}
	return false;
}

bool ImportsHandling::CutModule(CTreeItem item)
{
	ImportModuleThunk* pModule = GetModuleThunk(item);
	if (pModule)
	{
		CTreeItem child = item.GetChild();
		while (!child.IsNull()) {
			ItemDataMap.erase(child);
			child = child.GetNextSibling();
		}
		ItemDataMap.erase(item);
		pModule->m_TreeItem.Delete();
		m_ModuleMap.erase(pModule->m_Key);
		pModule = nullptr;
		UpdateCounts();
		return true;
	}
	return false;
}

DWORD_PTR ImportsHandling::GetApiAddressByNode(CTreeItem item)
{
	const ImportThunk* pImport = GetImportThunk(item);
	if (pImport) {
		return pImport->m_ApiAddressVA;
	}
	return 0;
}

void ImportsHandling::ScanAndFixModuleList()
{
	WCHAR prevModuleName[MAX_PATH] = { 0 };
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	it_module = m_ModuleMap.begin();
	while (it_module != m_ModuleMap.end()) {
		ImportModuleThunk& moduleThunk = it_module->second;
		it_import = moduleThunk.m_ThunkMap.begin();

		ImportThunk* pImportThunkPrev = &it_import->second;

		while (it_import != moduleThunk.m_ThunkMap.end()) {
			ImportThunk& importThunk = it_import->second;
			if (importThunk.m_ModuleName[0] == 0 || importThunk.m_ModuleName[0] == L'?') {
				AddNotFoundApiToModuleList(&importThunk);
			}
			else {
				if (_wcsicmp(importThunk.m_ModuleName, prevModuleName))
				{
					AddModuleToModuleList(importThunk.m_ModuleName, importThunk.m_RVA);
				}
				AddFunctionToModuleList(&importThunk);
			}

			wcscpy_s(prevModuleName, importThunk.m_ModuleName);
			it_import++;
		}

		moduleThunk.m_ThunkMap.clear();
		it_module++;
	}

	m_ModuleMap = m_ModuleMapNew;
	m_ModuleMap.clear();
}

void ImportsHandling::ExpandAllTreeNodes()
{
	ChangeExpandStateOfTreeNodes(TVE_EXPAND);
}

void ImportsHandling::CollapseAllTreeNodes()
{
	ChangeExpandStateOfTreeNodes(TVE_COLLAPSE);
}
