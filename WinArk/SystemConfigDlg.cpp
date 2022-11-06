#include "stdafx.h"
#include "SystemConfigDlg.h"
#include <DriverHelper.h>
#include "FormatHelper.h"
#include "SymbolHelper.h"

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

LRESULT CSystemConfigDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	bool enable = GetDlgItem(IDC_SET_CALLBACK).IsWindowEnabled();
	if (!enable)
		SendMessage(WM_COMMAND, IDC_REMOVE_CALLBACK);
	if (m_enableDbgSys) {
		DriverHelper::DisableDbgSys();
	}
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

LRESULT CSystemConfigDlg::OnEnableDbgSys(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (m_enableDbgSys) {
		bool success = DriverHelper::DisableDbgSys();
		if (success) {
			SetDlgItemText(IDC_ENABLE_DBGSYS, L"启用调试子系统");
			m_enableDbgSys = false;
		}
		else {
			AtlMessageBox(m_hWnd, L"禁用失败!");
		}
	}
	else {
		DbgSysCoreInfo info;

		bool success = InitDbgSymbols(&info);
		do
		{
			if (!success)
				break;
			success = DriverHelper::EnableDbgSys(&info);
		} while (false);
		if (success) {
			SetDlgItemText(IDC_ENABLE_DBGSYS, L"禁用调试子系统");
			m_enableDbgSys = true;
		}
		else {
			AtlMessageBox(m_hWnd, L"启用失败!");
		}
	}
	return TRUE;
}

bool CSystemConfigDlg::InitDbgSymbols(DbgSysCoreInfo *pInfo) {
	pInfo->NtCreateDebugObjectAddress = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtCreateDebugObject");
	if (!pInfo->NtCreateDebugObjectAddress)
		return false;

	pInfo->DbgkDebugObjectTypeAddress = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkDebugObjectType");
	if (!pInfo->DbgkDebugObjectTypeAddress)
		return false;

	pInfo->ZwProtectVirtualMemory = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ZwProtectVirtualMemory");
	if (!pInfo->ZwProtectVirtualMemory)
		return false;

	pInfo->NtDebugActiveProcess = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtDebugActiveProcess");
	if (!pInfo->NtDebugActiveProcess)
		return false;

	pInfo->DbgkpPostFakeProcessCreateMessages = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpPostFakeProcessCreateMessages");
	if (!pInfo->DbgkpPostFakeProcessCreateMessages)
		return false;

	pInfo->DbgkpSetProcessDebugObject = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSetProcessDebugObject");
	if (!pInfo->DbgkpSetProcessDebugObject)
		return false;

	pInfo->EprocessOffsets.RundownProtect = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "RundownProtect");
	if (pInfo->EprocessOffsets.RundownProtect == -1)
		return false;

	return true;
}