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


#include "View.h"
#include "DeviceManagerView.h"
#include "WindowsView.h"

// c2061 在一个类还没实现前，就互相交叉使用，前置声明不能解决
enum class TabColumn :int {
	Process, KernelModule, 
	//Kernel, UserHook,
	// KernelHook,
	Network,Driver,Registry,Device,Windows
};
DEFINE_ENUM_FLAG_OPERATORS(TabColumn);


class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CAutoUpdateUI<CMainDlg>,
	public CMessageFilter, 
	public CIdleHandler
{
public:
	enum { IDD = IDD_MAINDLG };

	// 如果类中有引用类型，则必须在构造函数中初始化

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

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTcnSelChange(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/);


	void CloseDialog(int nVal);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

	void InitProcessTable();
	void InitNetworkTable();
	void InitKernelModuleTable();
	void InitDriverTable();
	void InitDriverInterface();
	void InitRegistryView();
	void InitDeviceView();
	void InitWindowsView();

	BEGIN_MSG_MAP_EX(CMainDlg)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		NOTIFY_HANDLER(IDC_TAB_VIEW, TCN_SELCHANGE, OnTcnSelChange)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainDlg>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()
public:
	CCommandBarCtrl m_CmdBar;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
public:

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
private:
	CTabCtrl m_TabCtrl;
	
	CProcessTable* m_ProcTable{ nullptr };
	CNetwrokTable* m_NetTable{ nullptr };
	CKernelModuleTable* m_KernelModuleTable{ nullptr };
	CDriverTable* m_DriverTable{ nullptr };
	CMultiPaneStatusBarCtrl m_StatusBar;

	CRegistryManagerView m_RegView;

	CDeviceManagerView m_DevView;
	CWindowsView m_WinView;
	CString m_StatusText;

	// table array
	HWND m_hwndArray[16];
	// current select tab
	int _index = 0;

	CommandManager m_CmdMgr;
	CEdit m_Edit;
	bool m_AllowModify{ true };
};
