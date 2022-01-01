#pragma once
#include "PiDDBCacheTable.h"
#include "UnloadedDriverTable.h"

class CKernelView :
	public CWindowImpl<CKernelView> {
public:
	DECLARE_WND_CLASS(nullptr);

	const UINT TabId = 0x1236;
	CKernelView() :m_TabCtrl(this) {
	}

	BEGIN_MSG_MAP(CKernelView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		NOTIFY_HANDLER(TabId, TCN_SELCHANGE, OnTcnSelChange)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTcnSelChange(int, LPNMHDR hdr, BOOL&);


	enum class TabColumn : int {
		PiDDBCacheTable,UnloadedDriverTable
	};

	void InitPiDDBCacheTable();
	void InitUnloadedDriverTable();

private:
	// 动态创建出来的控件
	CContainedWindowT<CTabCtrl> m_TabCtrl;

	CPiDDBCacheTable* m_PiDDBCacheTable;
	CUnloadedDriverTable* m_UnloadedDriverTable;

	HWND m_hwndArray[16];
	int _index = 0;
};