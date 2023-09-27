// MainDlg.cpp : implementation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainFrame.h"
#include "DriverHelper.h"
#include "SecurityHelper.h"
#include "ClipboardHelper.h"
#include "SecurityInformation.h"
#include "NewKeyDlg.h"
#include "RenameKeyCommand.h"
#include "ColorsSelectionDlg.h"
#include "ThemeSystem.h"
#include "IniFile.h"


#define IDC_VIEW_PROCESS 0xDAD

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (m_pQuickFindDlg && m_pQuickFindDlg->IsDialogMessage(pMsg))
		return TRUE;

	// 用于响应搜索快捷回车键
	if (m_pFindDlg && m_pFindDlg->IsDialogMessage(pMsg))
		return TRUE;

	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	return FALSE;
}

LRESULT CMainFrame::OnForwardToActiveView(WORD id, WORD code, HWND h, BOOL& /*bHandled*/) {
	auto hWnd = m_hwndArray[_index];
	if (::IsWindow(hWnd)) {
		auto msg = GetCurrentMessage();
		::SendMessage(hWnd, msg->message, msg->wParam, msg->lParam);
	}
	return 0;
}


BOOL CMainFrame::OnIdle() {
	UpdateUI();
	UIUpdateToolBar();
	UIUpdateStatusBar();

	return FALSE;
}

void CMainFrame::UpdateUI() {

	return;
}

void CMainFrame::InitProcessTable() {
	BarDesc bars[] = {
		{19,"Process Name",0},
		{19,"Process ID",0},
		{9,"Session",0},
		{20,"User Name",0},
		{20,"EPROCESS",0},
		{12,"Priority",0},
		{9,"Threads",0},
		{9,"Handles",0},
		{19,"Attributes",0},
		{20,"Create Time",0},
		{30,"Description",0},
		{20,"Company Name",0},
		{20,"File Version",0},
		{56,"Image Path",0},
		{856,"Commandline",0}
	};
	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	m_pProcTable = new CProcessTable(info, table);
	m_hWndClient = m_pProcTable->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED| WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_pProcTable->ShowWindow(SW_SHOW);
	m_pProcTable->m_Images.Create(16, 16, ILC_COLOR32, 64, 32);
	m_pProcTable->Refresh();
	m_hwndArray[static_cast<int>(TabColumn::Process)] = m_pProcTable->m_hWnd;
}

void CMainFrame::InitNetworkTable() {
	BarDesc bars[] = {
		{20,"Process Name",0},
		{20,"Process ID",0},
		{10,"Protocol",0},
		{15,"Status",0},
		{20,"Local Address",0},
		{10,"Local Port",0},
		{20,"Remote Address",0},
		{10,"Remote Port",0},
		{20,"Create Time",0},
		{30,"Module Name",0},
		{256,"Module Path",0},
	};

	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	m_pNetTable = new CNetwrokTable(info, table);
	m_pNetTable->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::Network)] = m_pNetTable->m_hWnd;
}

void CMainFrame::InitKernelModuleTable() {
	BarDesc bars[] = {
		{20,"Driver File Name",0},
		{20,"Image Base",0},
		{10,"Image Size",0},
		{16,"Load Order",0},
		{22,"Company Name",0},
		{260,"Full Path",0},
	};

	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	m_pKernelModuleTable = new CKernelModuleTable(info, table);
	m_pKernelModuleTable->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::KernelModule)] = m_pKernelModuleTable->m_hWnd;
}

void CMainFrame::InitDriverTable() {
	BarDesc bars[] = {
		{20,"Driver Service Name",0},
		{20,"Display Name",0},
		{10,"Status",0},
		{20,"Driver Type",0},
		{20,"Boot Type",0},
		{20,"Description",0},
		{260,"Binary Path",0},
	};

	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	m_pDriverTable = new CDriverTable(info, table);
	m_pDriverTable->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::Driver)] = m_pDriverTable->m_hWnd;
}

void CMainFrame::InitServiceTable() {
	BarDesc bars[] = {
		{20,"Serivce Name",0},
		{20,"Display Name",0},
		{10,"Status",0},
		{20,"Service Type",0},
		{20,"Process ID"},
		{30,"Process Name"},
		{20,"Boot Type",0},
		{260,"Binary Path",0},
		{30,"Account Name",0},
		{20,"Error Control",0},
		{50,"Description",0},
		{50,"Service Right",0},
		{20,"Trigger",0},
		{20,"Dependencies",0},
		{50,"Allow Control",0},
		{30,"Security ID",0},
		{20,"Security ID Type"}
	};

	TableInfo table = {
		1,1,TABLE_SORTMENU | TABLE_COPYMENU | TABLE_APPMENU,9,0,0,0
	};

	BarInfo info;
	info.nbar = _countof(bars);
	info.font = 9;
	for (int i = 0; i < info.nbar; i++) {
		info.bar[i].defdx = bars[i].defdx;
		info.bar[i].mode = bars[i].mode;
		info.bar[i].name = bars[i].name;
	}

	m_pServiceTable = new CServiceTable(info, table);
	m_pServiceTable->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_EX_LAYERED);
	m_hwndArray[static_cast<int>(TabColumn::Service)] = m_pServiceTable->m_hWnd;
}

void CMainFrame::InitDriverInterface() {
	// 定位驱动二进制文件，提取到系统目录，然后安装
	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_DRIVER), L"BIN");

	if (!hRes)
		return;

	auto hGlobal = ::LoadResource(nullptr, hRes);
	if (!hGlobal)
		return;

	auto size = ::SizeofResource(nullptr, hRes);
	void* pBuffer = ::LockResource(hGlobal);

	if (!DriverHelper::IsDriverLoaded()) {
		if (!SecurityHelper::IsRunningElevated()) {
			if (AtlMessageBox(nullptr, L"Kernel Driver not loaded. Some functionality will not be available. Install?", IDS_TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES) {
				if (SecurityHelper::IsSysRun()) {
					if(!SecurityHelper::SysRun(L"install"))
						AtlMessageBox(*this, L"Error running driver installer", IDS_TITLE, MB_ICONERROR);
				}
				else if (!SecurityHelper::RunElevated(L"install", false)) {
					AtlMessageBox(*this, L"Error running driver installer", IDS_TITLE, MB_ICONERROR);
				}
			}
		}
		else {
			if (!DriverHelper::InstallDriver(false,pBuffer,size) || !DriverHelper::LoadDriver()) {
				MessageBox(L"Failed to install driver. Some functionality will not be available.", L"WinArk", MB_ICONERROR);
			}
		}
	}
	if (DriverHelper::IsDriverLoaded()) {
		if (DriverHelper::GetVersion() < DriverHelper::GetCurrentVersion()) {
			auto response = AtlMessageBox(nullptr, L"Driver Update（A newer driver is available with new functionality. Update?）",
				IDS_TITLE, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
			if (response == IDYES) {
				if (SecurityHelper::IsRunningElevated()) {
					if (!DriverHelper::UpdateDriver(pBuffer,size))
						AtlMessageBox(nullptr, L"Failed to update driver", IDS_TITLE, MB_ICONERROR);
				}
				else {
					DriverHelper::CloseDevice();
					if (SecurityHelper::IsSysRun()) {
						if (!SecurityHelper::SysRun(L"update"))
							AtlMessageBox(*this, L"Error update driver service", IDS_TITLE, MB_ICONERROR);
					}
					else
						SecurityHelper::RunElevated(L"update", false);
				}
			}
		}
	}

	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

void CMainFrame::InitRegistryView() {
	// 注册表
	m_RegView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	m_RegView.m_MainSplitter.ShowWindow(SW_SHOW);
	m_hwndArray[static_cast<int>(TabColumn::Registry)] = m_RegView.m_hWnd;
}

// 设备列表
void CMainFrame::InitDeviceView() {
	auto hWnd = m_DevView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::Device)] = m_DevView.m_hWnd;
}

// 窗口视图
void CMainFrame::InitWindowsView() {
	// WS_CLIPCHILDREN 子窗口区域父窗口不负责绘制,子窗口自行绘制
	// 不设置的话，父窗口绘制会遮挡住子窗口
	HWND hWnd = m_WinView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD|WS_VISIBLE | WS_CLIPSIBLINGS| WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::Windows)] = m_WinView.m_hWnd;
}

void CMainFrame::InitKernelHookView() {
	HWND hWnd = m_KernelHookView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::KernelHook)] = m_KernelHookView.m_hWnd;
}

void CMainFrame::InitKernelView() {
	m_KernelView = new CKernelView(this);
	HWND hWnd = m_KernelView->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::Kernel)] = m_KernelView->m_hWnd;
}

void CMainFrame::InitConfigView() {
	HWND hWnd = m_SysConfigView.Create(m_hWnd, rcDefault);
	m_hwndArray[static_cast<int>(TabColumn::Config)] = m_SysConfigView.m_hWnd;
}

void CMainFrame::InitMiscView() {
	m_MiscView = new CMiscView(this);
	HWND hWnd = m_MiscView->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hwndArray[static_cast<int>(TabColumn::Misc)] = m_MiscView->m_hWnd;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	LoadSettings();
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);

	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	CMenuHandle hMenu = GetMenu();
	if (SecurityHelper::IsRunningElevated()) {
		
		CString text;
		GetWindowText(text);
		CString append = L"  (Administrator)";
		if (SecurityHelper::IsSysRun()) {
			append = L"  (System)";
			hMenu.GetSubMenu(0).DeleteMenu(ID_RUNAS_SYSTEM, MF_BYCOMMAND);
		}
		SetWindowText(text + append);
	}

	UIAddMenu(hMenu);
	/*m_CmdBar.AttachMenu(hMenu);
	m_CmdBar.m_bAlphaImages = true;
	SetMenu(nullptr);*/

	InitCommandBar();

	/*auto submenu = menu.GetSubMenu(1);
	WCHAR text[64];
	menu.GetMenuString(1, text, _countof(text), MF_BYPOSITION);
	menu.RemoveMenu(1, MF_BYPOSITION);
	menu.InsertMenu(1, MF_BYPOSITION, submenu.m_hMenu, text);
	DrawMenuBar();*/
	
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	CreateSimpleStatusBar();
	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	int parts[] = { 100,200,300,430,560,700,830,960,1100 };
	m_StatusBar.SetParts(_countof(parts), parts);
	m_StatusBar.ShowWindow(SW_SHOW);
	UIAddStatusBar(m_StatusBar);

	CTabCtrl tabCtrl;
	CRect r;
	GetClientRect(&r);
	r.bottom = 25;
	auto hTabCtrl = tabCtrl.Create(m_hWnd, &r, nullptr, WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
		| TCS_HOTTRACK | TCS_SINGLELINE | TCS_RIGHTJUSTIFY | TCS_TABS,
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY,TabId);
	m_TabCtrl.SubclassWindow(hTabCtrl);
	// 初始化选择夹
	struct {
		PCWSTR Name;
	}columns[] = {
		L"Process",
		L"Kernel Module",
		L"Kernel",
		L"Kernel Hook",
		L"Network",
		L"Driver",
		L"Registry",
		L"Device",
		L"Windows",
		L"Services",
		L"Config",
		L"Event Trace",
		L"Explorer",
		L"Misc",
	};

	int i = 0;
	for (auto& col : columns) {
		m_TabCtrl.InsertItem(i++, col.Name);
	}
	m_TabCtrl.SetFont(g_hAppFont, true);
	AddSimpleReBarBand(m_TabCtrl, nullptr, TRUE, 0, TRUE);

	auto hWndToolBar = m_tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
	m_tb.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
	InitProcessToolBar(m_tb);

	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	UIAddToolBar(hWndToolBar);

	UIEnable(ID_MONITOR_PAUSE, FALSE);
	UIEnable(ID_MONITOR_STOP, FALSE);

	CReBarCtrl rb(m_hWndToolBar);
	rb.LockBands(true);


	SetWindowLong(GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hWnd, 0xffffff, _opacity, LWA_ALPHA);
	
	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	InitDriverInterface();

	InitProcessTable();
	InitNetworkTable();
	InitKernelModuleTable();
	InitDriverTable();
	InitServiceTable();

	InitRegistryView();
	InitDeviceView();
	InitWindowsView();
	InitKernelHookView();
	InitKernelView();
	InitConfigView();
	InitEtwView();
	
	InitExplorerView();
	InitMiscView();

	UpdateLayout();
	UpdateUI();

	return TRUE;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	AppSettings::Get().Save();

	SaveSettings();

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = false;
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

void CMainFrame::CloseDialog(int nVal) {
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainFrame::OnTcnSelChange(int, LPNMHDR hdr, BOOL&) {
	int index = 0;

	index = m_TabCtrl.GetCurSel();
	for (auto hwnd : m_hwndArray) {
		if (::IsWindow(hwnd)) {
			::ShowWindow(hwnd, SW_HIDE);
		}
	}

	ClearToolBarButtons(m_tb);
	
	m_hWndClient = m_hwndArray[index];
	switch (static_cast<TabColumn>(index)) {
		case TabColumn::Process:
			InitProcessToolBar(m_tb);
			UIAddToolBar(m_tb.m_hWnd);
			m_pProcTable->ShowWindow(SW_SHOW);
			m_pProcTable->SetFocus();
			break;
		case TabColumn::Network:
			m_pNetTable->ShowWindow(SW_SHOW);
			m_pNetTable->SetFocus();
			break;
		case TabColumn::KernelModule:
			m_pKernelModuleTable->ShowWindow(SW_SHOW);
			m_pKernelModuleTable->SetFocus();
			break;
		case TabColumn::Driver:
			m_pDriverTable->ShowWindow(SW_SHOW);
			m_pDriverTable->SetFocus();
			break;
		case TabColumn::Registry:
			InitRegToolBar(m_tb);
			UIAddToolBar(m_tb.m_hWnd);
			m_RegView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Device:
			m_DevView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Windows:
			m_WinView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::KernelHook:
			m_KernelHookView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Service:
			m_pServiceTable->ShowWindow(SW_SHOW);
			m_pServiceTable->SetFocus();
			break;
		case TabColumn::Kernel:
			m_KernelView->ShowWindow(SW_SHOW);
			break;
		case TabColumn::Config:
			m_SysConfigView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Etw:
			InitEtwToolBar(m_tb);
			UIAddToolBar(m_tb.m_hWnd);
			m_pEtwView->ShowWindow(SW_SHOW);
			break;
		case TabColumn::Explorer:
			m_ExplorerView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Misc:
			m_MiscView->ShowWindow(SW_SHOW);
			break;
		default:
			break;
	}
	::PostMessage(m_hWnd, WM_SIZE, 0, 0);
	_index = index;
	return 0;
}

void CMainFrame::OnGetMinMaxInfo(LPMINMAXINFO lpMMI) {
	lpMMI->ptMinTrackSize.x = 550;
	lpMMI->ptMinTrackSize.y = 450;
}

bool CMainFrame::CanPaste() const {
	return false;
}

UINT CMainFrame::TrackPopupMenu(CMenuHandle menu, int x, int y) {
	return (UINT)m_CmdBar.TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y);
}

void CMainFrame::SetStartKey(const CString& key) {
	// m_RegView.SetStartKey(key);
}

void CMainFrame::SetStatusText(PCWSTR text) {
	m_StatusBar.SetText(1, m_StatusText = text, SBT_NOBORDERS);
}

LRESULT CMainFrame::OnMonitorStart(WORD, WORD, HWND, BOOL&) {
	m_pEtwView->StartMonitoring(m_tm, true);
	bool isWin8Plus = ::IsWindows8OrGreater();
	if (isWin8Plus) {
		m_tm.Start([&](auto data) {
			m_pEtwView->AddEvent(data);
			});
	}
	else {
		DWORD flags = 0;
		for (auto type : m_tm.GetKernelEventTypes()) {
			flags |= (DWORD)type;
		}
		if (flags == 0)
			flags = (DWORD)KernelEventTypes::Process;
		m_tm.Start([&](auto data) {
			m_pEtwView->AddEvent(data);
			}, flags);
	}

	ClearToolBarButtons(m_tb);
	InitEtwToolBar(m_tb);

	SetTimer(1, 5000, nullptr);
	SetPaneIcon(1, m_RunIcon);
	return 0;
}

LRESULT CMainFrame::OnMonitorStop(WORD, WORD, HWND, BOOL&) {
	KillTimer(1);
	m_tm.Stop();
	m_tm.Pause(false);

	m_pEtwView->StartMonitoring(m_tm, false);
	SetPaneIcon(1, m_StopIcon);

	ClearToolBarButtons(m_tb);
	InitEtwToolBar(m_tb);

	return 0;
}

LRESULT CMainFrame::OnMonitorPause(WORD, WORD, HWND, BOOL&) {
	m_tm.Pause(!m_tm.IsPaused());
	UISetCheck(ID_MONITOR_PAUSE, m_tm.IsPaused());
	int image = 2;
	HICON hIcon = m_PauseIcon;
	if (!m_tm.IsPaused()) {
		hIcon = m_tm.IsRunning() ? m_RunIcon : m_StopIcon;
	}
	SetPaneIcon(1, hIcon);
	return 0;
}

BOOL CMainFrame::TrackPopupMenu(HMENU hMenu, HWND hWnd, POINT* pt, UINT flags) {
	POINT cursorPos;
	if (pt == nullptr) {
		::GetCursorPos(&cursorPos);
		pt = &cursorPos;
	}
	return m_CmdBar.TrackPopupMenu(hMenu, flags, pt->x, pt->y);
}

HFONT CMainFrame::GetMonoFont() {
	return m_MonoFont;
}

void CMainFrame::ViewDestroyed(void* view) {
}

TraceManager& CMainFrame::GetTraceManager() {
	return m_tm;
}

BOOL CMainFrame::SetPaneText(int index, PCWSTR text) {
	return m_StatusBar.SetText(index, text);
}

BOOL CMainFrame::SetPaneIcon(int index, HICON hIcon) {
	return m_StatusBar.SetIcon(index, hIcon);
}

CUpdateUIBase* CMainFrame::GetUpdateUI() {
	return this;
}

LRESULT CMainFrame::OnTimer(UINT, WPARAM id, LPARAM, BOOL&) {
	if (id == 1 && m_pEtwView) {
		KillTimer(1);
		MEMORYSTATUSEX ms = { sizeof(ms) };
		::GlobalMemoryStatusEx(&ms);
		if (ms.dwMemoryLoad > 94 && m_pEtwView) {
			if (AtlMessageBox(*this, L"Physical memory is low. Continue monitoring?",
				IDS_TITLE, MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONWARNING | MB_SETFOREGROUND | MB_SYSTEMMODAL) == IDCANCEL) {
				PostMessage(WM_COMMAND, ID_MONITOR_STOP);
			}
		}
	}

	return 0;
}

void CMainFrame::InitEtwToolBar(CToolBarCtrl& tb, int size) {
	CImageList tbImages;
	tbImages.Create(size, size, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);

	struct {
		UINT id;
		int image;
		int style = BTNS_BUTTON;
	}buttons[] = {
		{ ID_MONITOR_START,IDI_PLAY},
		{ ID_MONITOR_PAUSE, IDI_PAUSE },
		{ ID_MONITOR_STOP, IDI_STOP },
		{ 0 },
		{ ID_VIEW_AUTOSCROLL, IDI_SCROLL },
		{ 0 },
		{ ID_MONITOR_CLEARALL, IDI_CANCEL },
		{ 0 },
		{ ID_CONFIGURE_EVENTS, IDI_TOOLS },
		{ ID_CONFIGURE_FILTERS, IDI_FILTER },
		{ 0 },
		{ ID_EVENT_CALLSTACK, IDI_STACK },
		{ ID_EVENT_PROPERTIES, IDI_PROPERTIES },
	};

	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = tbImages.AddIcon(AtlLoadIconImage(b.image, 0, size, size));
			BYTE fsState = TBSTATE_ENABLED;
			
			if (b.id == ID_MONITOR_START) {
				if (m_tm.IsRunning()) {
					fsState = TBSTATE_INDETERMINATE;
				}
			}
			else if (b.id == ID_MONITOR_PAUSE) {
				if (m_tm.IsPaused()||!m_tm.IsRunning()) {
					fsState = TBSTATE_INDETERMINATE;
				}
			}
			else if(b.id == ID_MONITOR_STOP) {
				if (!m_tm.IsRunning()) {
					fsState = TBSTATE_INDETERMINATE;
				}
			}
			tb.AddButton(b.id, b.style, fsState, image, nullptr, 0);
		}
	}
}

void CMainFrame::ClearToolBarButtons(CToolBarCtrl& tb)
{
	while (tb.DeleteButton(0));
}

void CMainFrame::InitEtwView() {
	m_RunIcon.LoadIconWithScaleDown(IDI_PLAY, 20, 20);
	m_StopIcon.LoadIconWithScaleDown(IDI_STOP, 20, 20);
	m_PauseIcon.LoadIconWithScaleDown(IDI_PAUSE, 20, 20);
	SetPaneIcon(1, m_StopIcon);
	UIEnable(ID_MONITOR_STOP, FALSE);
	UIEnable(ID_MONITOR_PAUSE, FALSE);

	m_MonoFont.CreatePointFont(100, L"Consolas");

	m_pEtwView = new CEtwView(this);
	m_pEtwView->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_hwndArray[static_cast<int>(TabColumn::Etw)] = m_pEtwView->m_hWnd;
}




void CMainFrame::InitProcessToolBar(CToolBarCtrl& tb) {
	CImageList tbImages;
	tbImages.Create(24, 24, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);

	struct {
		UINT id;
		int image;
		int style = BTNS_BUTTON;
		PCWSTR text = nullptr;
	}buttons[] = {
		{ID_PROCESS_REFRESH,IDI_REFRESH},
	};

	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = tbImages.AddIcon(AtlLoadIconImage(b.image, 0, 24, 24));
			tb.AddButton(b.id, b.style, TBSTATE_ENABLED, image, b.text, 0);
		}
	}
}

LRESULT CMainFrame::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
	if (m_tm.IsRunning())
		m_tm.Stop();

	return DefWindowProc();
}

void CMainFrame::InitCommandBar() {
	struct {
		UINT id, icon;
	} cmds[] = {
		{ ID_MONITOR_START, IDI_PLAY },
		{ ID_MONITOR_STOP, IDI_STOP },
		{ ID_MONITOR_CLEARALL, IDI_CANCEL },
		{ ID_MONITOR_CONFIGUREEVENTS, IDI_TOOLS },
		{ ID_CONFIGURE_FILTERS, IDI_FILTER },
		{ ID_EVENT_CALLSTACK, IDI_STACK },
		{ ID_EVENT_PROPERTIES, IDI_PROPERTIES },
		{ ID_MONITOR_PAUSE, IDI_PAUSE },
		{ ID_VIEW_AUTOSCROLL, IDI_SCROLL },
		{ ID_SEARCH_QUICKFIND,IDI_FIND},
		{ID_SEARCH_FINDALL,IDI_SEARCH},
	};
	for (auto& cmd : cmds)
		m_CmdBar.AddIcon(AtlLoadIcon(cmd.icon), cmd.id);
}

void CMainFrame::DoFind(PCWSTR text, const QuickFindOptions& options) {
	m_pEtwView->SendMessage(WM_COMMAND, ID_SEARCH_FINDNEXT);
}

void CMainFrame::WindowClosed() {
	m_pQuickFindDlg = nullptr;
}

LRESULT CMainFrame::OnQuickFind(WORD, WORD, HWND, BOOL&) {
	if (!m_pQuickFindDlg) {
		m_pQuickFindDlg = new CQuickFindDlg(this);
		m_pQuickFindDlg->Create(*this);
		m_pQuickFindDlg->ShowWindow(SW_SHOW);
	}
	m_pQuickFindDlg->BringWindowToTop();
	m_pQuickFindDlg->SetFocus();

	return 0;
}

void CMainFrame::InitRegToolBar(CToolBarCtrl& tb, int size) {
	CImageList tbImages;
	tbImages.Create(size, size, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);

	const struct {
		UINT id;
		int image;
		BYTE style = BTNS_BUTTON;
		PCWSTR text = nullptr;
	}buttons[] = {
		{ ID_KEY_GOTO, IDI_GOTO },
	};

	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			auto hIcon = AtlLoadIconImage(b.image, 0, size, size);
			ATLASSERT(hIcon);
			int image = tbImages.AddIcon(hIcon);
			tb.AddButton(b.id, b.style, TBSTATE_ENABLED, image, b.text, 0);
		}
	}
}

LRESULT CMainFrame::OnColors(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ThemeColor Colors[(int)TableColorIndex::COUNT]{
		ThemeColor(L"文本颜色",g_myColor[Black]),
		ThemeColor(L"文本高亮颜色",g_myColor[Red]),
		ThemeColor(L"辅助文本颜色",g_myColor[DarkGray]),
		ThemeColor(L"背景颜色",g_myColor[White]),
		ThemeColor(L"选中时的背景颜色",g_myColor[Yellow]),
		ThemeColor(L"分割线的颜色",g_myColor[DarkBlue]),
		ThemeColor(L"辅助对象的颜色",g_myColor[Blue]),
		ThemeColor(L"条件断点颜色",g_myColor[Magenta]),
	};
	CColorsSelectionDlg dlg(Colors, _countof(Colors),_opacity);
	if (dlg.DoModal() == IDOK) {
		_opacity = dlg.GetOpacity();
	}
	return TRUE;
}

LRESULT CMainFrame::OnOptionsFont(WORD, WORD, HWND, BOOL&) {
	LOGFONT lf;
	CFontHandle(g_hAppFont).GetLogFont(lf);
	CFontDialog dlg(&lf);
	if (dlg.DoModal() == IDOK) {
		dlg.GetCurrentFont(&lf);
		g_hAppFont = CreateFontIndirect(&lf);
		AppSettings::Get().Font(lf);
	}
	return 0;
}

CString CMainFrame::GetDefaultSettingsFile() {
	WCHAR path[MAX_PATH];
	::SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path);
	::StringCchCat(path, _countof(path), L"\\WinArk.ini");
	return path;
}

void CMainFrame::SetColor(ThemeColor* colors,int count) {
	for (int i = 0; i < count; i++) {
		switch (i) {
			case 0:
				g_myColor[g_myScheme[0].textcolor] = colors[i].Color;
				break;

			case 1:
				g_myScheme[0].hitextcolor = colors[i].Color;
				break;

			case 2:
				g_myColor[g_myScheme[0].lowcolor] = colors[i].Color;
				break;

			case 3:
				g_myColor[g_myScheme[0].bkcolor] = colors[i].Color;
				break;

			case 4:
				g_myColor[g_myScheme[0].selbkcolor] = colors[i].Color;
				break;

			case 5:
				g_myColor[g_myScheme[0].linecolor] = colors[i].Color;
				break;

			case 6:
				g_myColor[g_myScheme[0].auxcolor] = colors[i].Color;
				break;

			case 7:
				g_myColor[g_myScheme[0].condbkcolor] = colors[i].Color;
				break;
		}
	}
}

void CMainFrame::LoadSettings(PCWSTR filename) {
	CString path;
	if (filename == nullptr) {
		path = GetDefaultSettingsFile();
		filename = path;
	}
	ThemeColor colors[(int)TableColorIndex::COUNT];
	bool success = LoadColors(filename, L"TableColor", colors, _countof(colors));
	if (success) {
		SetColor(colors, _countof(colors));
		InitPenSys();
		InitBrushSys();
		IniFile file(filename);
		_opacity = file.ReadInt(L"Opacity", L"Value", 255);
	}
}

void CMainFrame::SaveSettings(PCWSTR filename) {
	CString path;
	if (filename == nullptr) {
		path = GetDefaultSettingsFile();
		filename = path;
	}
	ThemeColor Colors[(int)TableColorIndex::COUNT]{
		ThemeColor(L"文本颜色",g_myColor[Black]),
		ThemeColor(L"文本高亮颜色",g_myColor[Red]),
		ThemeColor(L"辅助文本颜色",g_myColor[DarkGray]),
		ThemeColor(L"背景颜色",g_myColor[White]),
		ThemeColor(L"选中时的背景颜色",g_myColor[Yellow]),
		ThemeColor(L"分割线的颜色",g_myColor[DarkBlue]),
		ThemeColor(L"辅助对象的颜色",g_myColor[Blue]),
		ThemeColor(L"条件断点颜色",g_myColor[Magenta]),
	};
	SaveColors(filename, L"TableColor", Colors, _countof(Colors));
	IniFile file(filename);
	file.WriteInt(L"Opacity", L"Value", _opacity);
}

LRESULT CMainFrame::OnEditFind(WORD, WORD, HWND, BOOL&) {
	int index = m_TabCtrl.GetCurSel();

	m_hWndClient = m_hwndArray[index];
	switch (static_cast<TabColumn>(index)) {
		case TabColumn::Process:
			//m_pProcTable->ShowWindow(SW_SHOW);
			//m_pProcTable->SetFocus();
			break;
		case TabColumn::Network:
			/*m_pNetTable->ShowWindow(SW_SHOW);
			m_pNetTable->SetFocus();*/
			break;
		case TabColumn::KernelModule:
		/*	m_pKernelModuleTable->ShowWindow(SW_SHOW);
			m_pKernelModuleTable->SetFocus();*/
			break;
		case TabColumn::Driver:
			/*m_pDriverTable->ShowWindow(SW_SHOW);
			m_pDriverTable->SetFocus();*/
			break;
		case TabColumn::Registry:
			//m_RegView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Device:
			//m_DevView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Windows:
			//m_WinView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::KernelHook:
			//m_KernelHookView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Service:
			/*m_pServiceTable->ShowWindow(SW_SHOW);
			m_pServiceTable->SetFocus();*/
			break;
		case TabColumn::Kernel:
		{
			m_IView = m_KernelView->GetCurView();
			if (m_IView && m_IView->IsFindSupported()) {
				if (!m_pFindDlg) {
					m_pFindDlg = new CFindReplaceDialog;
					m_pFindDlg->Create(TRUE, m_FindText, nullptr, FR_DOWN, m_hWnd);
				}
				if (!m_pFindDlg->IsWindowVisible()) {
					m_pFindDlg->ShowWindow(SW_SHOW);
				}
				m_pFindDlg->BringWindowToTop();
				m_pFindDlg->SetFocus();
			}
			break;
		}
		case TabColumn::Config:
			//m_SysConfigView.ShowWindow(SW_SHOW);
			break;
		case TabColumn::Etw:
			//m_pEtwView->ShowWindow(SW_SHOW);
			break;
		default:
			break;
	}
	return 0;
}

LRESULT CMainFrame::OnFindReplaceMessage(UINT /*uMsg*/, WPARAM id, LPARAM lParam, BOOL& handled) {
	auto fr = reinterpret_cast<FINDREPLACE*>(lParam);
	if (fr->Flags & FR_DIALOGTERM) {
		m_pFindDlg->DestroyWindow();
		m_pFindDlg = nullptr;
		return 0;
	}
	m_FindText = fr->lpstrFindWhat;
	m_FindFlags = fr->Flags;

	m_IView->DoFind(m_FindText, m_FindFlags);

	return 0;
}

LRESULT CMainFrame::OnRunAsSystem(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (SecurityHelper::EnablePrivilege(SE_DEBUG_NAME, true)) {
		if (!SecurityHelper::IsSysRun()) {
			if (SecurityHelper::SysRun(L"runas"))
				SendMessage(WM_CLOSE);
		}
	}
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);

	return 0;
}

void CMainFrame::InitExplorerView() {
	m_ExplorerView.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_ExplorerView.m_WndSplitter.ShowWindow(SW_SHOW);
	m_hwndArray[static_cast<int>(TabColumn::Explorer)] = m_ExplorerView.m_hWnd;
}

