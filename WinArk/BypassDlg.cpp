#include "stdafx.h"
#include "BypassDlg.h"
#include <DriverHelper.h>

LRESULT CBypassDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_BypassKDBG.Attach(GetDlgItem(IDC_BYPASS_KDBG));
	return TRUE;
}

LRESULT CBypassDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	int checkCode = m_BypassKDBG.GetCheck();
	DWORD flag = 0;
	if (checkCode == BST_CHECKED) {
		flag |= BYPASS_KERNEL_DEBUGGER;
	}
	if (m_enable) {
		DriverHelper::Unbypass(flag);
	}
	return TRUE;
}

LRESULT CBypassDlg::OnBypass(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int checkCode = m_BypassKDBG.GetCheck();
	DWORD flag = 0;
	if (checkCode == BST_CHECKED) {
		flag |= BYPASS_KERNEL_DEBUGGER;
	}

	if (flag == 0) {
		AtlMessageBox(m_hWnd, L"未选择任何绕过项", L"参数错误", MB_ICONERROR);
		return FALSE;
	}
	if (m_enable) {
		bool ok = DriverHelper::Unbypass(flag);
		if (ok) {
			SetDlgItemText(IDC_BYPASS, L"启用检测绕过");
			m_enable = false;
		}
		else {
			AtlMessageBox(m_hWnd, L"禁用失败");
		}
	}
	else {
		bool ok = DriverHelper::Bypass(flag);
		if (ok) {
			SetDlgItemText(IDC_BYPASS, L"禁用检测绕过");
			m_enable = true;
		}
		else {
			AtlMessageBox(m_hWnd, L"启用失败");
		}
	}

	return TRUE;
}