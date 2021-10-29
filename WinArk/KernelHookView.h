#pragma once

class CKernelHookView:
	public CWindowImpl<CKernelHookView>{
public:
	DECLARE_WND_CLASS(nullptr);

	CKernelHookView() :m_TabCtrl(this) {
	}

	BEGIN_MSG_MAP(CKernelHookView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


private:
	// 动态创建出来的控件
	CContainedWindowT<CTabCtrl> m_TabCtrl;
};