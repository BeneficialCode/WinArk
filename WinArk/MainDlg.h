// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "View.h"
#include "DeviceManagerView.h"
#include "CommandManager.h"
#include "RegistryManager.h"

// c2061 在一个类还没实现前，就互相交叉使用，前置声明不能解决
enum class TabColumn :int {
	Process, KernelModule, 
	//Kernel, KernelHook, Ring3Hook, 
	Network,Driver,Registry,Device
};
DEFINE_ENUM_FLAG_OPERATORS(TabColumn);

class CRegistryManagerView;
class CProcessTable;
class CNetwrokTable;
class CKernelModuleTable;
class CDriverTable;
class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CAutoUpdateUI<CMainDlg>,
	public CMessageFilter, 
	public CIdleHandler,
	public IMainApp
{
public:
	enum { IDD = IDD_MAINDLG };

	CMainDlg() :m_RegMgr(m_treeview, m_RegView) {
	}
	// 如果类的的构造函数为引用类型，则必须初始化

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	void UpdateUI();

	bool IsAllowModify() const override{
		return m_AllowModify;
	}

	CUpdateUIBase* GetUIUpdate() {
		return this;
	}

	UINT TrackPopupMenu(CMenuHandle menu, int x, int y);

	bool CanPaste() const;

	BEGIN_MSG_MAP_EX(CMainDlg)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		if (m_splitter.GetActivePane() == 1) { // 消息转发给m_view的消息节1
			CHAIN_MSG_MAP_ALT_MEMBER(m_RegView,1)
		}
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(ID_NEW_KEY,OnNewKey)
		COMMAND_ID_HANDLER(ID_EDIT_DELETE, OnDelete)
		COMMAND_ID_HANDLER(ID_EDIT_RENAME, OnEditRename)
		COMMAND_ID_HANDLER(ID_EDIT_PERMISSIONS, OnEditPermissions)
		COMMAND_ID_HANDLER(ID_TREE_COPYKEYNAME, OnCopyKeyName)
		COMMAND_ID_HANDLER(ID_TREE_COPYFULLKEYNAME, OnCopyFullKeyName)
		NOTIFY_CODE_HANDLER(TVN_DELETEITEM,OnTreeDeleteItem)
		NOTIFY_CODE_HANDLER(TVN_ENDLABELEDIT, OnEndRename)
		NOTIFY_CODE_HANDLER(TVN_BEGINLABELEDIT, OnBeginRename)
		NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnTreeItemExpanding)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeSelectionChanged)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnTreeContextMenu)
		NOTIFY_HANDLER(IDC_TAB_VIEW, TCN_SELCHANGE, OnTcnSelChange)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainDlg>)
		CHAIN_MSG_MAP_ALT_MEMBER(m_RegView,2)
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
	
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnTcnSelChange(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/);

	LRESULT OnTreeContextMenu(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeDeleteItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnTreeItemExpanding(int /*idCtrl*/, LPNMHDR nmhdr, BOOL& /*bHandled*/);
	LRESULT OnTreeSelectionChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

	LRESULT OnNewKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditPermissions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyKeyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyFullKeyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBeginRename(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnEndRename(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void ShowCommandError(PCWSTR message = nullptr) override;
	bool AddCommand(std::shared_ptr<AppCommandBase> cmd, bool execute = true) override;

	void InitProcessTable();
	void InitNetworkTable();
	void InitKernelModuleTable();
	void InitDriverTable();
	void InitDriverInterface();
	void InitRegistryView();
	void InitDeviceView();

private:
	CTabCtrl m_TabCtrl;
	CProcessTable* m_ProcTable{ nullptr };
	CNetwrokTable* m_NetTable{ nullptr };
	CKernelModuleTable* m_KernelModuleTable{ nullptr };
	CDriverTable* m_DriverTable{ nullptr };
	CMultiPaneStatusBarCtrl m_StatusBar;

	CTreeViewCtrl m_treeview;
	CSplitterWindow m_splitter;
	RegistryManager m_RegMgr;
	TreeNodeBase* m_SelectedNode;
	CRegistryManagerView m_RegView;
	CDeviceManagerView m_DevView;
	HWND m_hwndArray[6];
	int _index = 0;

	CommandManager m_CmdMgr;
	CEdit m_Edit;
	bool m_AllowModify{ true };
};
