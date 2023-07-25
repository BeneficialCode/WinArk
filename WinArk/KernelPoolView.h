#pragma once

#include "NtDll.h"
#include "resource.h"
#include "Interfaces.h"
#include "ThemeColor.h"

struct TagItem {
	SYSTEM_POOLTAG TagInfo;
	PCWSTR SourceName = L"";
	PCWSTR SourceDesc = L"";
	CStringA Tag;
	int Index;
};



class CKernelPoolView :
	public CWindowImpl<CKernelPoolView, CListViewCtrl>,
	public CCustomDraw<CKernelPoolView>,
	public CIdleHandler,
	public IView {
public:
	enum ColumnType {
		TagName,
		PagedAllocs,
		PagedFrees,
		PagedDiff,
		PagedUsage,

		NonPagedAllocs,
		NonPagedFrees,
		NonPagedDiff,
		NonPagedUsage,

		SourceName,
		SourceDescription,
		NumColumns
	};

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	CKernelPoolView(IMainFrame* pFrame) :m_pFrame(pFrame) {}

	void LoadPoolTagText();
	void UpdatePoolTags();
	ULONG UpdateSystemPoolTags();
	ULONG UpdateSessionPoolTags(ULONG sessionId);
	void AddTag(const SYSTEM_POOLTAG& info, int index);
	void UpdateVisible();
	bool CompareItems(const TagItem& i1, const TagItem& i2);
	void DoSort();
	void SetToolBar(HWND hWnd);

	void AddCellColor(CellColor& cell, DWORD64 targetTime = 0);
	void RemoveCellColor(const CellColorKey& cell);
	int GetChange(const SYSTEM_POOLTAG& info, const SYSTEM_POOLTAG& newinfo, ColumnType type) const;

	size_t GetTotalPaged() const {
		return m_TotalPaged;
	}

	size_t GetTotalNonPaged() const {
		return m_TotalNonPaged;
	}

	BOOL PreTranslateMessage(MSG* pMsg);

	// CCustomDraw
	DWORD OnPrePaint(int id, LPNMCUSTOMDRAW cd);
	DWORD OnItemPrePaint(int id, LPNMCUSTOMDRAW cd);
	DWORD OnSubItemPrePaint(int id, LPNMCUSTOMDRAW cd);

	BOOL OnIdle() override;

	// IView
	bool IsFindSupported() const override {
		return true;
	}
	void DoFind(const CString& text, DWORD flags) override;

	BEGIN_MSG_MAP(CKernelPoolView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRBtnDown)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CKernelPoolView>, 1)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDisplayInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ODFINDITEM, OnFindItem)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		COMMAND_ID_HANDLER(ID_POOLTAG_REFRESH, OnRefresh)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRBtnDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnGetDisplayInfo(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnColumnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR /*nymph*/, BOOL& /*bHandled*/);
	void UpdatePaneText();
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


private:
	std::unordered_map<CellColorKey, CellColor> m_CellColors;

	int m_SortColumn = -1;
	CImageList m_Images;
	int m_UpdateInterval = 1000;
	size_t m_TotalPaged = 0, m_TotalNonPaged = 0;
	std::unordered_map<ULONG, std::shared_ptr<TagItem>> m_TagsMap;
	std::vector<std::shared_ptr<TagItem>> m_Tags;
	std::map<CStringA, std::pair<CString, CString>> m_TagSource;

	SYSTEM_POOLTAG_INFORMATION* m_PoolTags{ nullptr };
	SYSTEM_SESSION_POOLTAG_INFORMATION* m_SessionPoolTags{ nullptr };
	bool m_Running{ true };
	bool m_Ascending = true;

	IMainFrame* m_pFrame;
};