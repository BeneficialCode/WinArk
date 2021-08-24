#pragma once
#include "VirtualListView.h"
#include "DeviceManager.h"

class CDeviceManagerView :
	public CWindowImpl<CDeviceManagerView,CListViewCtrl>,
	public CVirtualListView<CDeviceManagerView> {
public:
	// 根据这个会创建一个窗口句柄给m_hWnd
	DECLARE_WND_CLASS(nullptr);

	void DoSort(const SortInfo* si);

private:
	enum class ItemType {
		None,String,MultiString,Dword,Boolean,Guid,PowerData
	};
	struct ItemData {
		PCWSTR Name;
		ItemType Type;
		WinSys::DeviceRegistryPropertyType PropertyType;
		CString Value;
	};

	struct ClassItemData {
		PCWSTR Name;
		ItemType Type;
		WinSys::DeviceClassRegistryPropertyType PropertyType;
		CString Value;
	};

	BEGIN_MSG_MAP(CDeviceManagerView)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED,OnTreeSelectionChanged)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,OnListGetDispInfo)
		CHAIN_MSG_MAP(CVirtualListView<CDeviceManagerView>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTreeSelectionChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnListGetDispInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void Refresh();
	void UpdateList();
	CString GetDeviceProperty(WinSys::DeviceInfo& di, int index) const;
	LRESULT GetDeviceClassInfo(LVITEM& item) const;
	CString GetDeviceClassProperty(const GUID* guid, int index) const;
	LRESULT GetDeviceInfo(LVITEM& item) const;
	static bool CompareItems(const CString& i1, const CString& i2, bool asc);

	friend class CMainDlg;
private:
	std::unique_ptr<WinSys::DeviceManager> m_DevMgr;
	std::vector<WinSys::DeviceInfo> m_Devices;
	std::vector<ItemData> m_Items;
	std::vector<ClassItemData> m_ClassItems;
	std::unordered_map<HTREEITEM, GUID> m_DeviceClasses;
	WinSys::DeviceInfo* m_SelectedDevice;
	const GUID* m_SelectedClass;
	int m_ComputerIcon;
	CTreeViewCtrlEx m_Tree;
	CListViewCtrl m_List;
	CSplitterWindow m_Splitter;
};
