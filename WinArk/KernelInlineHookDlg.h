#pragma once
#include "resource.h"
#include "KernelInlineHookTable.h"
#include "KernelModuleTracker.h"

class CKernelInlineHookDlg :public CDialogImpl<CKernelInlineHookDlg> {
public:
	enum {IDD = IDD_BACKGROUND};

	CKernelInlineHookDlg(std::shared_ptr<WinSys::KernelModuleInfo>& info) :_info(info) {}
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

	BEGIN_MSG_MAP_EX(CKernelInlineHookDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:
	std::shared_ptr<WinSys::KernelModuleInfo>& _info;
	CKernelInlineHookTable* _table{ nullptr };
};