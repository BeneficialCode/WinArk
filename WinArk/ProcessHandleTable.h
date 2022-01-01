#pragma once
#include "Table.h"
#include "resource.h"
#include "ObjectManager.h"
#include "ProcessManager.h"
#include "ProcessHandleTracker.h"


class CProcessHandleTable :
	public CTable<std::shared_ptr<WinSys::HandleInfo>>,
	public CWindowImpl<CProcessHandleTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	BEGIN_MSG_MAP(CProcessHandleTable)
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
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLBtnUp)
		MESSAGE_HANDLER(WM_USER_STS, OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
	END_MSG_MAP()

	CProcessHandleTable(BarInfo& bars, TableInfo& table, PCWSTR type, DWORD pid = 0);

	int ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::HandleInfo>& info, int column);

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


	void Refresh();
	static CString HandleAttributesToString(ULONG attributes);


private:
	struct Change {
		ULONG Handle;
		DWORD64 TargetTime;
		COLORREF Color;
		bool IsNewHandle;
	};

	WinSys::ObjectManager m_ObjMgr;
	WinSys::ProcessManager m_ProcMgr;
	std::unique_ptr<WinSys::ProcessHandleTracker> m_HandleTracker;
	CString m_HandleType;
	int m_ColumnCount;
	int m_Pid;
	std::unordered_map<WinSys::HandleInfo*,CString> m_DetailsCache;
	DWORD64 m_TargetUpdateTime = 0;
	std::vector<Change> m_Changes;
	wil::unique_handle m_hProcess;
	bool m_Paused = false;
	bool m_NamedObjectsOnly = false;
};