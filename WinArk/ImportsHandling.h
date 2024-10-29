#pragma once
#include "Thunks.h"
#include <unordered_map>
#include "MultiTree.h"

class ImportsHandling
{
public:
	std::map<DWORD_PTR, ImportModuleThunk> m_ModuleMap;
	std::map<DWORD_PTR, ImportModuleThunk> m_ModuleMapNew;

	ImportsHandling(CMultiSelectTreeViewCtrl& treeImports);
	~ImportsHandling();

	unsigned int ThunkCount() const { return m_ThunkCount; }
	unsigned int InvalidThunkCount() const { return m_InvalidThunkCount; }
	unsigned int SuspectThunkCount() const { return m_SuspectThunkCount; }

	bool IsModule(CTreeItem item);
	bool IsImport(CTreeItem item);

	ImportModuleThunk* GetModuleThunk(CTreeItem item);
	ImportThunk* GetImportThunk(CTreeItem item);
	
	void DisplayAllImports();
	void ClearAllImports();
	void SelectImports(bool invalid, bool suspect);

	bool InvalidateImport(CTreeItem item);
	bool InvalidateModule(CTreeItem item);
	bool SetImport(CTreeItem item, const WCHAR* pModuleName, const CHAR* pApiName, 
		WORD ordinal = 0, WORD hint = 0, bool valid = true, bool suspect = false);
	bool CutImport(CTreeItem item);
	bool CutModule(CTreeItem item);

	DWORD_PTR GetApiAddressByNode(CTreeItem selectedTreeNode);
	void ScanAndFixModuleList();
	void ExpandAllTreeNodes();
	void CollapseAllTreeNodes();

private:
	DWORD m_NumberOfFunctions = 0;

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
	enum class Icon {
		Check = 0,
		Warning,
		Error
	};

	void UpdateCounts();

	CTreeItem AddDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, ImportModuleThunk* pModuleThunk);
	CTreeItem AddApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, ImportThunk* pImportThunk);
	
	void UpdateImportInTreeView(const ImportThunk* pImportThunk, CTreeItem item);
	void UpdateModuleInTreeView(const ImportModuleThunk* pImportThunk, CTreeItem item);

	Icon GetAppropiateIcon(const ImportThunk* pImportThunk);
	Icon GetAppropiateIcon(bool valid);

	bool AddModuleToModuleList(const WCHAR* pModuleName, DWORD_PTR firstThunk);
	void AddUnknownModuleToModuleList(DWORD_PTR firstThunk);
	bool AddNotFoundApiToModuleList(const ImportThunk* pApiNotFound);
	bool AddFunctionToModuleList(const ImportThunk* pApiFound);
	bool IsNewModule(const WCHAR* pModuleName);

	void ChangeExpandStateOfTreeNodes(UINT flag);
};
