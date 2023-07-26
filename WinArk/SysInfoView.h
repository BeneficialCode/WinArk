#pragma once

#include "ViewBase.h"
#include "VirtualListView.h"
#include "SystemInformation.h"

class CSysInfoView :
	public CVirtualListView<CSysInfoView>,
	public CViewBase<CSysInfoView> {
public:
	CSysInfoView(IMainFrame* frame);

	CString GetColumnText(HWND, int row, int col) const;

	void DoSort(const SortInfo* si);
	bool IsSortable(HWND, int row) const;

	void OnUpdate();

	BEGIN_MSG_MAP(CSysInfoView)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		CHAIN_MSG_MAP(CViewBase<CSysInfoView>)
		CHAIN_MSG_MAP(CVirtualListView<CSysInfoView>)
	END_MSG_MAP()

public:

	void Refresh();

	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);

	enum class RowType {
		Version,TotalRAM,TotalCPUs,BootTime,ProcessorBrand,BootMode,
		SecureBoot,HVCIStatus,Vendor,Hypervisor,
		COUNT,
	};

	enum class ItemDataFlags {
		None = 0,
		Const = 1,
		Bits32 = 2,
		Time = 4,
		TimeSpan = 8,
		MemoryInPages = 16,
		MemorySize = 32,
		Special = 64
	};

	struct ItemData {
		ItemData(RowType type, PCWSTR name, uint32_t* pCurrent, uint32_t* pPrevious, ItemDataFlags flags = ItemDataFlags::None);
		ItemData(RowType type, PCWSTR name, DWORD* pCurrent, DWORD* pPrevious, ItemDataFlags flags = ItemDataFlags::None);
		ItemData(RowType type, PCWSTR name, uint64_t* pCurrent, uint64_t* pPrevious, ItemDataFlags flags = ItemDataFlags::None);
		ItemData(RowType type, PCWSTR name, int64_t* pCurrent, int64_t* pPrevious, ItemDataFlags flags = ItemDataFlags::None);
		ItemData(RowType type, PCWSTR name, ItemDataFlags flags = ItemDataFlags::None);

		RowType Type;
		union {
			uint32_t* Value32;
			uint64_t* Value64;
		} Current;
		union {
			uint32_t* Value32;
			uint64_t* Value64;
		} Previous;
		CString Name;
		ItemDataFlags Flags;
	};

private:
	std::vector<ItemData> m_Items;
	CListViewCtrl m_List;
	WinSys::BasicSystemInfo m_BasicalSysInfo;
	DWORD m_CpuCount;
};

DEFINE_ENUM_FLAG_OPERATORS(CSysInfoView::ItemDataFlags);