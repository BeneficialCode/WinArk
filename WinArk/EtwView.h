#pragma once

#include "resource.h"
#include "VirtualListView.h"
#include "Interfaces.h"
#include "ViewBase.h"
#include <EventData.h>
#include <shared_mutex>
#include <SymbolHandler.h>
#include "EventConfiguration.h"
#include "FilterConfiguration.h"


class CEtwView :
	public CViewBase<CEtwView>,
	public CVirtualListView<CEtwView>,
	public CCustomDraw<CEtwView>
{
public:
	CEtwView(IMainFrame* frame);

	DECLARE_WND_CLASS(NULL)

	void Activate(bool active);
	void AddEvent(std::shared_ptr<EventData> data);
	void StartMonitoring(TraceManager& tm, bool start);
	CString GetColumnText(HWND, int row, int col) const;
	int GetRowImage(HWND,int row,int col) const;
	PCWSTR GetColumnTextPointer(HWND, int row, int col) const;
	bool OnRightClickList(HWND,int row, int col, POINT& pt);
	bool OnDoubleClickList(HWND,int row, int col, POINT& pt);

	bool IsSortable(HWND,int col) const;
	void DoSort(const SortInfo* si);

	BOOL PreTransalteMessage(MSG* pMsg);

	// custom draw
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	static CImageList GetEventImageList();
	static int GetImageFromEventName(PCWSTR name);

	virtual void OnFinalMessage(HWND /*hWnd*/);

	BEGIN_MSG_MAP(CEtwView)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		MESSAGE_HANDLER(WM_TIMER,OnTimer)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		COMMAND_ID_HANDLER(ID_EVENT_CALLSTACK,OnCallStack)
		MESSAGE_HANDLER(WM_CLOSE,OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_CONFIGURE_EVENTS,OnConfigureEvents)
		COMMAND_ID_HANDLER(ID_MONITOR_CLEARALL,OnClear)
		COMMAND_ID_HANDLER(ID_CONFIGURE_FILTERS,OnConfigFilters)
		COMMAND_ID_HANDLER(ID_EVENT_PROPERTIES,OnEventProperties)
		COMMAND_ID_HANDLER(ID_VIEW_AUTOSCROLL,OnAutoScroll)
		COMMAND_ID_HANDLER(ID_SEARCH_FINDNEXT,OnFindNext)
		CHAIN_MSG_MAP(CVirtualListView<CEtwView>)
		CHAIN_MSG_MAP(CCustomDraw<CEtwView>)
		CHAIN_MSG_MAP(CViewBase<CEtwView>)
	END_MSG_MAP()

private:
	std::wstring ProcessSpecialEvent(EventData* data) const;
	DWORD AddNewProcess(DWORD tid) const;
	std::wstring GetEventDetails(EventData* data) const;
	void UpdateEventStatus();
	void UpdateUI();
	void ApplyFilters(const FilterConfiguration& config);

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClear(WORD /*wNotifyCode*/, WORD /*wId*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCallStack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEventProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnConfigureEvents(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAutoScroll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnConfigFilters(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
private:
	CListViewCtrl m_List;
	inline static CImageList s_Images;
	inline static std::unordered_map<std::wstring, int> s_IconsMap;
	std::vector<std::shared_ptr<EventData>> m_Events;
	std::vector<std::shared_ptr<EventData>> m_OrgEvents;
	std::vector<std::shared_ptr<EventData>> m_TempEvents;
	std::mutex m_EventsLock;
	EventsConfiguration m_EventsConfig;
	FilterConfiguration m_FilterConfig;
	HFONT m_hFont;
	bool m_IsMonitoring{ false };
	bool m_IsDraining{ false };
	bool m_AutoScroll{ false };
	bool m_IsActive{ false };
	mutable std::unordered_map<uint32_t, uint32_t> _threadById;
	mutable std::unordered_map<uint32_t, uint32_t> _processById;
	mutable std::unordered_map<uint32_t, std::wstring> _processNameById;
};