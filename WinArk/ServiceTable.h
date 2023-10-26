#pragma once
#include "Table.h"
#include "resource.h"
#include "VirtualListView.h"
#include <ServiceInfo.h>
#include <ServiceManager.h>
#include <Service.h>
#include <ProcessManager.h>
#include "ServiceInfoEx.h"

class CServiceTable :
	public CTable<WinSys::ServiceInfo>,
	public CWindowImpl<CServiceTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	CServiceTable(BarInfo& bars, TableInfo& table);
	int ParseTableEntry(CString& s, char& mask, int& select, WinSys::ServiceInfo& info, int column);
	bool CompareItems(const WinSys::ServiceInfo& s1, const WinSys::ServiceInfo& s2, int col, bool asc);

	BEGIN_MSG_MAP(CServiceTable)
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
		MESSAGE_HANDLER(WM_RBUTTONDOWN,OnRBtnDown)
		MESSAGE_HANDLER(WM_USER_STS, OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		COMMAND_ID_HANDLER(ID_SERVICE_START,OnServiceStart)
		COMMAND_ID_HANDLER(ID_SERVICE_STOP,OnServiceStop)
		COMMAND_ID_HANDLER(ID_SERVICE_PAUSE, OnServicePause)
		COMMAND_ID_HANDLER(ID_SERVICE_CONTINUE, OnServiceContinue)
		COMMAND_ID_HANDLER(ID_SERVICE_PROPERTIES, OnServiceProperties)
		COMMAND_ID_HANDLER(ID_SERVICE_UNINSTALL, OnServiceDelete)
		COMMAND_ID_HANDLER(ID_SERVICE_START_ALL,OnServiceStartAll)
		COMMAND_ID_HANDLER(ID_SERVICE_EXPORT_BY_PID,OnExportByPid)
		COMMAND_ID_HANDLER(ID_SERVICE_REFRESH,OnRefresh)
	END_MSG_MAP()

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
	LRESULT OnServiceStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServicePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnServiceStartAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExportByPid(WORD, WORD, HWND, BOOL&);
	std::wstring GetSingleServiceInfo(WinSys::ServiceInfo& info);
	LRESULT OnRefresh(WORD, WORD, HWND, BOOL&);


private:
	enum class ServiceColumn {
		Name,DisplayName,State,Type,PID,ProcessName,StartType,BinaryPath,AccountName,
		ErrorControl,Description,Privileges,Triggers,Dependencies,ControlsAccepted,SID,SidType
	};

	void Refresh();

	// static int ServiceStatusToImage(WinSys::ServiceState state);
	static PCWSTR ServiceStateToString(WinSys::ServiceState state);
	static CString ServiceStartTypeToString(const WinSys::ServiceConfiguration&);
	static CString ErrorControlToString(WinSys::ServiceErrorControl ec);
	static CString ServiceTypeToString(WinSys::ServiceType type);
	static PCWSTR ServiceSidTypeToString(WinSys::ServiceSidType type);
	static PCWSTR TriggerToText(const WinSys::ServiceTrigger& trigger);
	static CString DependenciesToString(const std::vector<std::wstring>& deps);
	static CString ServiceControlsAcceptedToString(WinSys::ServiceControlsAccepted accepted);

	ServiceInfoEx& GetServiceInfoEx(const std::wstring& name) const;

	mutable std::unordered_map<std::wstring, ServiceInfoEx> m_ServicesEx;
	CListViewCtrl m_List;
	WinSys::ProcessManager m_ProcMgr;
	int m_SelectedHeader;
	bool m_ViewServices;
};