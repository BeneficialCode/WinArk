// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "View.h"
#include "DeviceManagerView.h"
#include "CommandManager.h"
#include "ProcessTable.h"
#include "NetworkTable.h"
#include "KernelModuleTable.h"
#include "DriverTable.h"
#include "ServiceTable.h"

#include "View.h"
#include "DeviceManagerView.h"
#include "WindowsView.h"
#include "KernelHookView.h"
#include "KernelView.h"


// c2061 在一个类还没实现前，就互相交叉使用，前置声明不能解决
enum class TabColumn :int {
	Process, KernelModule, 
	Kernel, 
	//UserHook,
	KernelHook,
	Network,Driver,Registry,Device,Windows,Service
};

DEFINE_ENUM_FLAG_OPERATORS(TabColumn);


class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>,
	public CAutoUpdateUI<CMainFrame>,
	public CMessageFilter, 
	public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(nullptr,IDR_MAINFRAME)

	CMainFrame() :m_TabCtrl(this) {};
	// 如果类中有引用类型，则必须在构造函数中初始化

	const UINT TabId = 1234;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	void UpdateUI();

	void SetStartKey(const CString& key);
	void SetStatusText(PCWSTR text);

	CUpdateUIBase* GetUIUpdate() {
		return this;
	}

	UINT TrackPopupMenu(CMenuHandle menu, int x, int y);

	bool CanPaste() const;

	LRESULT OnTcnSelChange(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

	void InitProcessTable();
	void InitNetworkTable();
	void InitKernelModuleTable();
	void InitDriverTable();
	void InitServiceTable();
	void InitDriverInterface();
	void InitRegistryView();
	void InitDeviceView();
	void InitWindowsView();
	void InitKernelHookView();
	void InitKernelView();

	BEGIN_MSG_MAP_EX(CMainFrame)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		NOTIFY_HANDLER(TabId, TCN_SELCHANGE, OnTcnSelChange)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()
public:
	CCommandBarCtrl m_CmdBar;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
public:

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
private:
	CContainedWindowT<CTabCtrl> m_TabCtrl;
	// CTabCtrl m_TabCtrl;
	
	CProcessTable* m_ProcTable{ nullptr };
	CNetwrokTable* m_NetTable{ nullptr };
	CKernelModuleTable* m_KernelModuleTable{ nullptr };
	CDriverTable* m_DriverTable{ nullptr };
	CServiceTable* m_ServiceTable{ nullptr };
	
	CMultiPaneStatusBarCtrl m_StatusBar;

	CRegistryManagerView m_RegView;
	CDeviceManagerView m_DevView;
	CWindowsView m_WinView;
	CKernelHookView m_KernelHookView;
	CKernelView m_KernelView;

	CString m_StatusText;

	// table array
	HWND m_hwndArray[16];
	// current select tab
	int _index = 0;

	CommandManager m_CmdMgr;
	CEdit m_Edit;
	bool m_AllowModify{ true };
};
