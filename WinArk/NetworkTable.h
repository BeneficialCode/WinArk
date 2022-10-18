#pragma once
#include "Table.h"
#include "resource.h"
#include "ActiveConnectionTracker.h"
#include "Keys.h"
#include "ProcessManager.h"
#include "SortedFilteredVector.h"

struct Connection;
class CNetwrokTable :
	public CTable<std::shared_ptr<WinSys::Connection>>, 
	public CWindowImpl<CNetwrokTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	BEGIN_MSG_MAP(CNetwrokTable)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
		MESSAGE_HANDLER(WM_USER_VABS, OnUserVabs)
		MESSAGE_HANDLER(WM_USER_VREL, OnUserVrel)
		MESSAGE_HANDLER(WM_USER_CHGS, OnUserChgs)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLBtnDown)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRBtnDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLBtnUp)
		MESSAGE_HANDLER(WM_USER_STS, OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		COMMAND_ID_HANDLER(ID_NETWORK_REFRESH,OnRefresh)
	END_MSG_MAP()

	CNetwrokTable(BarInfo& bars, TableInfo& table);

	int ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::Connection>& info, int column);
	bool CompareItems(const std::shared_ptr<WinSys::Connection>& c1, const std::shared_ptr<WinSys::Connection>& c2, int col, bool asc = true);

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD, WORD, HWND, BOOL&);

	enum class ItemState{None,New,Deleted,DeletePending = 4 };
	struct ItemEx {
		ItemState State{ ItemState::None };
		DWORD64 TargetTime;
		CString ProcessName;
		int Image{ 0 };
	};

	ItemEx* GetItemEx(WinSys::Connection* conn) const;

	void AddConnection(std::vector<std::shared_ptr<WinSys::Connection>>& vec, std::shared_ptr<WinSys::Connection> conn, bool reallyNew = true);

	const CString GetProcessName(WinSys::Connection* conn) const;
	void DoRefresh();
	int GetUpdateInterval() const {
		return m_UpdateInterval;
	}

	u_short HostByteOrderToNetworkByteOrder(UINT port);

	static PCWSTR ConnectionTypeToString(WinSys::ConnectionType type);
	static PCWSTR ConnectionStateToString(MIB_TCP_STATE state);
	static CString IPAddressToString(DWORD ip);
	static CString IPAddressToString(const IN6_ADDR& ip);
	static DWORD SwapBytes(DWORD x);

private:
	//SortedFilteredVector<std::shared_ptr<WinSys::Connection>> m_Items;
	std::vector<std::shared_ptr<WinSys::Connection>> m_NewConnections, m_OldConnections;
	mutable std::unordered_map<WinSys::Connection, ItemEx> m_ItemsEx;
	std::unordered_map<HICON, int> m_IconMap;
	WinSys::ActiveConnectionTracker m_Tracker;
	WinSys::ProcessManager m_pm;
	int m_UpdateInterval{ 1000 };
};