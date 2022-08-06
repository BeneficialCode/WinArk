#include "stdafx.h"
#include "SystemConfigDlg.h"
#include <DriverHelper.h>
#include "FormatHelper.h"

using namespace WinSys;

LRESULT CSystemConfigDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_CheckImageLoad.Attach(GetDlgItem(IDC_INTERCEPT_DRIVER));

	m_BasicSysInfo = SystemInformation::GetBasicSystemInfo();
	
	GetDlgItem(IDC_REMOVE_CALLBACK).EnableWindow(FALSE);

	CString text;
	auto& ver = SystemInformation::GetWindowsVersion();



	text.Format(L"%u.%u.%u", ver.Major, ver.Minor, ver.Build);
	SetDlgItemText(IDC_WIN_VERSION, text);

	text = FormatHelper::TimeToString(SystemInformation::GetBootTime());
	SetDlgItemText(IDC_BOOT_TIME, text);

	text = FormatHelper::FormatWithCommas(m_BasicSysInfo.TotalPhysicalInPages >> 8) + L" MB";
	text.Format(L"%s (%u GB)", text, (ULONG)((m_BasicSysInfo.TotalPhysicalInPages + (1 << 17)) >> 18));
	SetDlgItemText(IDC_USABLE_RAM, text);

	SetDlgItemInt(IDC_PROCESSOR_COUNT, m_BasicSysInfo.NumberOfProcessors);

	std::string brand = SystemInformation::GetCpuBrand();
	SetDlgItemTextA(m_hWnd,IDC_PROCESSOR, brand.c_str());

	return TRUE;
}
           
LRESULT CSystemConfigDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	bool enable = GetDlgItem(IDC_SET_CALLBACK).IsWindowEnabled();
	if (!enable)
		SendMessage(WM_COMMAND, IDC_REMOVE_CALLBACK);
	return TRUE;
}

LRESULT CSystemConfigDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	bool enable = GetDlgItem(IDC_SET_CALLBACK).IsWindowEnabled();
	if (!enable)
		SendMessage(WM_COMMAND, IDC_REMOVE_CALLBACK);
	return TRUE;
}

LRESULT CSystemConfigDlg::OnSetCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int count = 0;
	bool success = false;

	int checkCode = m_CheckImageLoad.GetCheck();
	if (checkCode == BST_CHECKED) {
		count += 1;
		success = DriverHelper::SetImageLoadNotify();
		if(success)
			m_CheckImageLoad.EnableWindow(FALSE);
	}

	if (count == 0) {
		AtlMessageBox(m_hWnd, L"未选择任何配置项", L"错误",MB_ICONERROR);
		return FALSE;
	}
	GetDlgItem(IDC_SET_CALLBACK).EnableWindow(FALSE);
	GetDlgItem(IDC_REMOVE_CALLBACK).EnableWindow();
	return TRUE;
}

LRESULT CSystemConfigDlg::OnRemoveCallback(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool success = false;
	int checkCode = m_CheckImageLoad.GetCheck();
	if (checkCode == BST_CHECKED) {
		success = DriverHelper::RemoveImageLoadNotify();
		if (success)
			m_CheckImageLoad.EnableWindow(TRUE);
	}

	GetDlgItem(IDC_SET_CALLBACK).EnableWindow(TRUE);
	GetDlgItem(IDC_REMOVE_CALLBACK).EnableWindow(FALSE);
	return TRUE;
}