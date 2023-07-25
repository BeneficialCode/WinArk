#pragma once
#include "PiDDBCacheTable.h"
#include "UnloadedDriverTable.h"
#include "KernelPoolView.h"
#include "Interfaces.h"
#include "BigPoolView.h"
#include "DpcTimerTable.h"
#include "IoTimerTable.h"
#include "WinExtHostsTable.h"

class CKernelView :
	public CWindowImpl<CKernelView> {
public:
	DECLARE_WND_CLASS(nullptr);

	const UINT TabId = 0x1236;
	CKernelView(IMainFrame* pFrame) :m_TabCtrl(this), m_pFrame(pFrame) {
	}

	BEGIN_MSG_MAP(CKernelView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		NOTIFY_HANDLER(TabId, TCN_SELCHANGE, OnTcnSelChange)
		if (m_KernelPoolView != nullptr)
			CHAIN_MSG_MAP_MEMBER((*m_KernelPoolView))
			if (m_BigPoolView != nullptr)
				CHAIN_MSG_MAP_MEMBER((*m_BigPoolView))
				REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTcnSelChange(int, LPNMHDR hdr, BOOL&);

	IView* GetCurView();

	enum class TabColumn : int {
		PiDDBCacheTable, UnloadedDriverTable, KernelPoolTable, BigPoolTable, DpcTimer, IoTimer, WinExtHosts
	};

	void InitPiDDBCacheTable();
	void InitUnloadedDriverTable();
	void InitDpcTimerTable();
	void InitIoTimerTable();
	void InitMiniFilterTable();
	void InitWinExtHostsTable();

private:
	// 动态创建出来的控件
	CContainedWindowT<CTabCtrl> m_TabCtrl;

	CPiDDBCacheTable* m_PiDDBCacheTable{ nullptr };
	CUnloadedDriverTable* m_UnloadedDriverTable{ nullptr };
	CKernelPoolView* m_KernelPoolView{ nullptr };
	CBigPoolView* m_BigPoolView{ nullptr };
	CDpcTimerTable* m_DpcTimerTable{ nullptr };
	CIoTimerTable* m_IoTimerTable{ nullptr };
	CWinExtHostsTable* m_WinExtHostsTable{ nullptr };

	IMainFrame* m_pFrame;
	HWND m_hwndArray[16];
	int _index = 0;
};