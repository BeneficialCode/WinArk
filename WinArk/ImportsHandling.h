#pragma once
#include "Thunks.h"
#include <unordered_map>
#include "MultiTree.h"

class ImportsHandling
{
public:
	std::map<DWORD_PTR, ImportModuleThunk> ModuleMap;
	std::map<DWORD_PTR, ImportModuleThunk> NewModuleMap;

	ImportsHandling(CMultiSelectTreeViewCtrl& treeImports);
	~ImportsHandling();
private:
	DWORD NumberOfFunctions = 0;

	unsigned int m_ThunkCount = 0;
	unsigned int m_InvalidThunkCount = 0;
	unsigned int m_SuspectThunkCount = 0;

	struct TreeItemData
	{
		bool IsModule;
		union
		{
			ImportModuleThunk* Module;
			ImportThunk* Import;
		};
	};

	std::unordered_map<HTREEITEM, TreeItemData> ItemDataMap;

	void SetItemData(CTreeItem item, const TreeItemData* pData);

	TreeItemData* GetItemData(CTreeItem item);

	WCHAR Text[512] = { 0 };

	CMultiSelectTreeViewCtrl& TreeImports;
	CImageList TreeIcons;
	CIcon hIconCheck;
	CIcon hIconWarning;
	CIcon hIconError;

	// They have to be added to the image list in that order!
	enum Icon {
		iconCheck = 0,
		iconWarning,
		iconError
	};

	void UpdateCounts();

	CTreeItem AddDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, ImportModuleThunk* moduleThunk);

	void UpdateModuleInTreeView(const ImportModuleThunk* importThunk, CTreeItem item);

	Icon GetAppropiateIcon(bool valid);
};
