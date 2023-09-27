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
#include "WindowsView.h"
#include "KernelHookView.h"
#include "KernelView.h"
#include "SystemConfigDlg.h"
#include "Interfaces.h"
#include "EtwView.h"
#include "TraceManager.h"
#include "QuickFindDlg.h"
#include "ExplorerView.h"
#include "MiscView.h"



// c2061 在一个类还没实现前，就互相交叉使用，前置声明不能解决
enum class TabColumn :int {
	Process, KernelModule, 
	Kernel, 
	KernelHook,
	Network,Driver,Registry,Device,Windows,Service,Config,Etw,
	Explorer,Misc
};

DEFINE_ENUM_FLAG_OPERATORS(TabColumn);

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>,
	public CAutoUpdateUI<CMainFrame>,
	public CMessageFilter, 
	public CIdleHandler,
	public IMainFrame,
	public IQuickFind
{
public:
	DECLARE_FRAME_WND_CLASS(nullptr,IDR_MAINFRAME)

	CMainFrame() :m_TabCtrl(this) {};
	// 如果类中有引用类型，则必须在构造函数中初始化

	const UINT TabId = 1234;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	// Inherited via IQuickFind
	void DoFind(PCWSTR text, const QuickFindOptions& options) override;
	void WindowClosed() override;

	// Inherited via IMainFrame
	BOOL TrackPopupMenu(HMENU hMenu, HWND hWnd, POINT* pt = nullptr, UINT flags = 0) override;
	HFONT GetMonoFont() override;
	void ViewDestroyed(void*) override;
	TraceManager& GetTraceManager() override;
	BOOL SetPaneText(int index, PCWSTR text) override;
	BOOL SetPaneIcon(int index, HICON hIcon) override;
	CUpdateUIBase* GetUpdateUI() override;

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
	void InitConfigView();
	void InitEtwView();
	
	void InitExplorerView();
	void InitMiscView();


	void LoadSettings(PCWSTR filename = nullptr);
	void SaveSettings(PCWSTR filename = nullptr);

	BEGIN_MSG_MAP_EX(CMainFrame)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CLOSE,OnClose)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_MONITOR_START, OnMonitorStart)
		COMMAND_ID_HANDLER(ID_MONITOR_STOP, OnMonitorStop)
		COMMAND_ID_HANDLER(ID_MONITOR_PAUSE, OnMonitorPause)
		COMMAND_ID_HANDLER(ID_SEARCH_QUICKFIND,OnQuickFind)
		COMMAND_ID_HANDLER(ID_OPTIONS_COLORS,OnColors)
		COMMAND_ID_HANDLER(ID_OPTIONS_FONT, OnOptionsFont)
		COMMAND_ID_HANDLER(ID_RUNAS_SYSTEM,OnRunAsSystem)
		COMMAND_ID_HANDLER(ID_EXIT,OnFileExit)
		MESSAGE_HANDLER(CFindReplaceDialog::GetFindReplaceMsg(), OnFindReplaceMessage)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
		NOTIFY_HANDLER(TabId, TCN_SELCHANGE, OnTcnSelChange)
		COMMAND_RANGE_HANDLER(0x8000, 0xefff, OnForwardToActiveView)
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
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRunAsSystem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	LRESULT OnFindReplaceMessage(UINT /*uMsg*/, WPARAM id, LPARAM lParam, BOOL& handled);


	LRESULT OnMonitorStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMonitorPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMonitorStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnForwardToActiveView(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnQuickFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnColors(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOptionsFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditFind(WORD, WORD, HWND, BOOL&);

private:
	void InitProcessToolBar(CToolBarCtrl& tb);
	void InitCommandBar();
	void ClearToolBarButtons(CToolBarCtrl& tb);
	void InitEtwToolBar(CToolBarCtrl& tb, int size = 24);
	void InitRegToolBar(CToolBarCtrl& tb, int size = 24);
	CString GetDefaultSettingsFile();
	void SetColor(ThemeColor* colors, int count);

private:
	CContainedWindowT<CTabCtrl> m_TabCtrl;
	// CTabCtrl m_TabCtrl;
	CToolBarCtrl m_tb;
	
	CProcessTable* m_pProcTable{ nullptr };
	CNetwrokTable* m_pNetTable{ nullptr };
	CKernelModuleTable* m_pKernelModuleTable{ nullptr };
	CDriverTable* m_pDriverTable{ nullptr };
	CServiceTable* m_pServiceTable{ nullptr };
	
	CMultiPaneStatusBarCtrl m_StatusBar;

	CExplorerView m_ExplorerView;
	CRegistryManagerView m_RegView;
	CDeviceManagerView m_DevView;
	CWindowsView m_WinView;
	CKernelHookView m_KernelHookView;
	CKernelView* m_KernelView{ nullptr };

	CSystemConfigDlg m_SysConfigView;
	CMiscView* m_MiscView{ nullptr };

	CEtwView* m_pEtwView{ nullptr };
	TraceManager m_tm;
	CIcon m_RunIcon, m_StopIcon, m_PauseIcon;
	CFont m_MonoFont;
	CQuickFindDlg* m_pQuickFindDlg{ nullptr };

	
	int _opacity = 255;

	CString m_StatusText;

	// table array
	HWND m_hwndArray[20];
	// current select tab
	int _index = 0;

	CommandManager m_CmdMgr;
	CEdit m_Edit;
	bool m_AllowModify{ true };

	CFindReplaceDialog* m_pFindDlg{ nullptr };
	inline static CString m_FindText;
	inline static DWORD m_FindFlags{ 0 };
	inline static IView* m_IView;
};
