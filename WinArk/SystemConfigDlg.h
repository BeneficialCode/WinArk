#pragma once
#include "resource.h"
#include "SystemInformation.h"
#include "DriverHelper.h"

class CSystemConfigDlg :
	public CDialogImpl<CSystemConfigDlg>,
	public CDialogResize<CSystemConfigDlg> {
public:
	enum { IDD = IDD_CONFIG };

	BEGIN_MSG_MAP(CSystemConfigDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY,OnDestroy)
		COMMAND_ID_HANDLER(IDC_SET_CALLBACK,OnSetCallback)
		COMMAND_ID_HANDLER(IDC_REMOVE_CALLBACK,OnRemoveCallback)
		COMMAND_ID_HANDLER(IDC_ENABLE_DBGSYS,OnEnableDbgSys)
		COMMAND_ID_HANDLER(ID_DBG_ADD,OnAddDebugger)
		COMMAND_ID_HANDLER(ID_DBG_REMOVE,OnRemoveDebugger)
		NOTIFY_HANDLER(IDC_DEBUGGER_LIST,NM_RCLICK,OnListViewContext)
		CHAIN_MSG_MAP(CDialogResize<CSystemConfigDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CSystemConfigDlg)
		/*DLGRESIZE_CONTROL(IDC_GROUP_DBGSYS,DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_ENABLE_DBGSYS,DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_GROUP_CALLBACK,DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_SET_CALLBACK, DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL(IDC_REMOVE_CALLBACK, DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_GROUP_SYSINFO,DLSZ_SIZE_X)*/
	END_DLGRESIZE_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEnableDbgSys(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnListViewContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnAddDebugger(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveDebugger(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


private:
	bool InitDbgSymbols(DbgSysCoreInfo* pInfo);

	bool InitRoutines(DbgSysCoreInfo* pInfo);

	bool InitEprocessOffsets(DbgSysCoreInfo* pInfo);

	bool InitEthreadOffsets(DbgSysCoreInfo* pInfo);

	bool InitKthreadOffsets(DbgSysCoreInfo* pInfo);

	bool InitPebOffsets(DbgSysCoreInfo* pInfo);

	bool InitCiSymbols(CiSymbols* pSym);

private:
	CButton m_CheckImageLoad;
	CButton m_CheckDriverLoad;
	CButton m_CheckLogHash;
	WinSys::BasicSystemInfo m_BasicSysInfo;
	bool m_enableDbgSys = false;
	CListViewCtrl m_List;
};