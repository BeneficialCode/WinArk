#pragma once
#include "resource.h"
#include "KernelEATHookTable.h"
#include "KernelModuleTracker.h"

class CKernelEATHookDlg :public CDialogImpl<CKernelEATHookDlg> {
public:
	enum { IDD = IDD_KERNEL_EAT_HOOK };

	CKernelEATHookDlg(std::shared_ptr<WinSys::KernelModuleInfo>& info):_info(info){}

	BEGIN_MSG_MAP_EX(CKernelEATHookDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);

private:
	CKernelEATHookTable* m_KernelEATHookTable{ nullptr };

	std::shared_ptr<WinSys::KernelModuleInfo>& _info;
};