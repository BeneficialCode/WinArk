#pragma once
#include "SysInfoView.h"
#include "TaskSchedView.h"
#include "LogonSessionsView.h"
#include "BypassDlg.h"

class CMiscView :
	public CWindowImpl<CMiscView> {
public:
	DECLARE_WND_CLASS(nullptr);

	const UINT TabId = 0x1237;
	CMiscView(IMainFrame* pFrame) :m_TabCtrl(this), m_pFrame(pFrame) {
	}

	BEGIN_MSG_MAP(CKernelView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		NOTIFY_HANDLER(TabId, TCN_SELCHANGE, OnTcnSelChange)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTcnSelChange(int, LPNMHDR hdr, BOOL&);

	enum class TabColumn :int {
		LogonSessions, BypassDetect, SystemInformation, TaskScheduler
	};

	void InitLogonSessionsView();
	void InitBypassDectectView();
	void InitSysInfoView();
	void InitTaskSchedView();

private:
	CContainedWindowT<CTabCtrl> m_TabCtrl;

	IMainFrame* m_pFrame;
	HWND m_hwndArray[16];
	int _index = 0;

	CLogonSessionsView* m_pLogonSessionView{ nullptr };

	CTaskSchedView m_TaskSchedView;

	CSysInfoView* m_SysInfoView{ nullptr };

	CBypassDlg m_BypassView;
};