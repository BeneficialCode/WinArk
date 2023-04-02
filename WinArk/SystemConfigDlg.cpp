#include "stdafx.h"
#include "SystemConfigDlg.h"
#include <DriverHelper.h>
#include "FormatHelper.h"
#include "SymbolHelper.h"

using namespace WinSys;

LRESULT CSystemConfigDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	//DlgResize_Init(true);
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
	SetDlgItemTextA(m_hWnd, IDC_PROCESSOR, brand.c_str());

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
		if (success)
			m_CheckImageLoad.EnableWindow(FALSE);
	}

	if (count == 0) {
		AtlMessageBox(m_hWnd, L"未选择任何配置项", L"错误", MB_ICONERROR);
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

bool CSystemConfigDlg::InitDbgSymbols(DbgSysCoreInfo* pInfo) {
	bool initSuccess = false;
	initSuccess = InitRoutines(pInfo);
	if (!initSuccess)
		return initSuccess;

	initSuccess = InitEprocessOffsets(pInfo);
	if (!initSuccess)
		return initSuccess;

	initSuccess = InitEthreadOffsets(pInfo);
	if (!initSuccess)
		return initSuccess;

	initSuccess = InitPebOffsets(pInfo);
	if (!initSuccess)
		return initSuccess;

	initSuccess = InitKthreadOffsets(pInfo);
	if (!initSuccess)
		return initSuccess;

	return true;
}

bool CSystemConfigDlg::InitRoutines(DbgSysCoreInfo* pInfo) {
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

	pInfo->DbgkpPostFakeThreadMessages = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpPostFakeThreadMessages");
	if (!pInfo->DbgkpPostFakeThreadMessages)
		return false;

	pInfo->DbgkpPostModuleMessages = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpPostModuleMessages");
	if (!pInfo->DbgkpPostModuleMessages)
		return false;

	pInfo->PsGetNextProcessThread = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsGetNextProcessThread");
	if (!pInfo->PsGetNextProcessThread)
		return false;

	pInfo->DbgkpProcessDebugPortMutex = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpProcessDebugPortMutex");
	if (!pInfo->DbgkpProcessDebugPortMutex)
		return false;

	pInfo->DbgkpWakeTarget = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpWakeTarget");
	if (!pInfo->DbgkpWakeTarget)
		return false;

	pInfo->DbgkpMarkProcessPeb = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpMarkProcessPeb");
	if (!pInfo->DbgkpMarkProcessPeb)
		return false;

	pInfo->DbgkpMaxModuleMsgs = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpMaxModuleMsgs");
	if (!pInfo->DbgkpMaxModuleMsgs)
		return false;

	pInfo->MmGetFileNameForAddress = (void*)SymbolHelper::GetKernelSymbolAddressFromName("MmGetFileNameForAddress");
	if (!pInfo->MmGetFileNameForAddress)
		return false;

	pInfo->DbgkpSendApiMessage = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSendApiMessage");
	if (!pInfo->DbgkpSendApiMessage)
		return false;

	pInfo->DbgkpQueueMessage = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpQueueMessage");
	if (!pInfo->DbgkpQueueMessage)
		return false;

	pInfo->DbgkpMaxModuleMsgs = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpMaxModuleMsgs");
	if (!pInfo->DbgkpMaxModuleMsgs)
		return false;

	pInfo->KeThawAllThreads = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KeThawAllThreads");
	if (pInfo->KeThawAllThreads) {

	}

	pInfo->DbgkpSectionToFileHandle = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSectionToFileHandle");
	if (!pInfo->DbgkpSectionToFileHandle)
		return false;

	pInfo->PsResumeThread = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsResumeThread");
	if (!pInfo->PsResumeThread)
		return false;

	pInfo->DbgkSendSystemDllMessages = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkSendSystemDllMessages");
	if (!pInfo->DbgkSendSystemDllMessages)
		return false;

	pInfo->PsSuspendThread = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsSuspendThread");
	if (!pInfo->PsSuspendThread)
		return false;

	pInfo->PsQuerySystemDllInfo = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsQuerySystemDllInfo");
	if (!pInfo->PsQuerySystemDllInfo)
		return false;

	pInfo->ObFastReferenceObject = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ObFastReferenceObject");
	if (!pInfo->ObFastReferenceObject)
		return false;

	pInfo->ExfAcquirePushLockShared = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ExfAcquirePushLockShared");
	if (!pInfo->ExfAcquirePushLockShared)
		return false;

	pInfo->ExfReleasePushLockShared = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ExfReleasePushLockShared");
	if (!pInfo->ExfReleasePushLockShared)
		return false;

	pInfo->ObFastReferenceObjectLocked = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ObFastReferenceObjectLocked");
	if (!pInfo->ObFastReferenceObjectLocked)
		return false;

	pInfo->PsGetNextProcess = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsGetNextProcess");
	if (!pInfo->PsGetNextProcess)
		return false;

	pInfo->DbgkpConvertKernelToUserStateChange = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpConvertKernelToUserStateChange");
	if (!pInfo->DbgkpConvertKernelToUserStateChange)
		return false;

	pInfo->PsCaptureExceptionPort = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsCaptureExceptionPort");
	if (!pInfo->PsCaptureExceptionPort)
		return false;


	pInfo->DbgkpSendErrorMessage = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSendErrorMessage");
	if (!pInfo->DbgkpSendErrorMessage)
		return false;

	pInfo->DbgkpSendApiMessageLpc = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSendApiMessageLpc");
	if (!pInfo->DbgkpSendApiMessageLpc)
		return false;

	pInfo->DbgkpOpenHandles = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpOpenHandles");
	if (!pInfo->DbgkpOpenHandles)
		return false;

	pInfo->DbgkpSuppressDbgMsg = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkpSuppressDbgMsg");
	if (!pInfo->DbgkpSuppressDbgMsg)
		return false;

	pInfo->ObFastDereferenceObject = (void*)SymbolHelper::GetKernelSymbolAddressFromName("ObFastDereferenceObject");
	if (!pInfo->ObFastDereferenceObject)
		return false;

	pInfo->DbgkCreateThread = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkCreateThread");
	if (!pInfo->DbgkCreateThread)
		return false;

	pInfo->DbgkExitThread = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkExitThread");
	if (!pInfo->DbgkExitThread)
		return false;

	pInfo->DbgkExitProcess = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkExitProcess");
	if (!pInfo->DbgkExitProcess)
		return false;

	pInfo->DbgkMapViewOfSection = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkMapViewOfSection");
	if (!pInfo->DbgkMapViewOfSection)
		return false;

	pInfo->DbgkUnMapViewOfSection = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkUnMapViewOfSection");
	if (!pInfo->DbgkUnMapViewOfSection)
		return false;

	pInfo->NtWaitForDebugEvent = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtWaitForDebugEvent");
	if (!pInfo->NtWaitForDebugEvent)
		return false;

	pInfo->NtDebugContinue = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtDebugContinue");
	if (!pInfo->NtDebugContinue)
		return false;

	pInfo->NtRemoveProcessDebug = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtRemoveProcessDebug");
	if (!pInfo->NtRemoveProcessDebug)
		return false;

	pInfo->DbgkForwardException = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkForwardException");
	if (!pInfo->DbgkForwardException)
		return false;

	pInfo->DbgkCopyProcessDebugPort = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkCopyProcessDebugPort");
	if (!pInfo->DbgkCopyProcessDebugPort)
		return false;

	pInfo->DbgkClearProcessDebugObject = (void*)SymbolHelper::GetKernelSymbolAddressFromName("DbgkClearProcessDebugObject");
	if (!pInfo->DbgkClearProcessDebugObject)
		return false;

	pInfo->NtSetInformationDebugObject = (void*)SymbolHelper::GetKernelSymbolAddressFromName("NtSetInformationDebugObject");
	if (!pInfo->NtSetInformationDebugObject)
		return false;

	pInfo->PsTerminateProcess = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PsTerminateProcess");
	if (!pInfo->PsTerminateProcess)
		return false;

	pInfo->PspNotifyEnableMask = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PspNotifyEnableMask");
	if (!pInfo->PspNotifyEnableMask)
		return false;

	pInfo->PspLoadImageNotifyRoutine = (void*)SymbolHelper::GetKernelSymbolAddressFromName("PspLoadImageNotifyRoutine");
	if (!pInfo->PspLoadImageNotifyRoutine)
		return false;



	pInfo->MiSectionControlArea = (void*)SymbolHelper::GetKernelSymbolAddressFromName("MiSectionControlArea");
	if (!pInfo->MiSectionControlArea)
		return false;

	pInfo->MiReferenceControlAreaFile = (void*)SymbolHelper::GetKernelSymbolAddressFromName("MiReferenceControlAreaFile");
	if (!pInfo->MiReferenceControlAreaFile)
		return false;
	return true;
}

bool CSystemConfigDlg::InitEprocessOffsets(DbgSysCoreInfo* pInfo) {
	pInfo->EprocessOffsets.RundownProtect = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "RundownProtect");
	if (pInfo->EprocessOffsets.RundownProtect == -1)
		return false;

#ifdef  _WIN64
	pInfo->EprocessOffsets.WoW64Process = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "WoW64Process");
	if (pInfo->EprocessOffsets.WoW64Process == -1)
		return false;
#endif //  _WIN64

	pInfo->EprocessOffsets.DebugPort = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "DebugPort");
	if (pInfo->EprocessOffsets.DebugPort == -1)
		return false;

	pInfo->EprocessOffsets.Peb = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "Peb");
	if (pInfo->EprocessOffsets.Peb == -1)
		return false;

	pInfo->EprocessOffsets.Flags = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "Flags");
	if (pInfo->EprocessOffsets.Flags == -1)
		return false;

	pInfo->EprocessOffsets.SectionBaseAddress = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "SectionBaseAddress");
	if (pInfo->EprocessOffsets.SectionBaseAddress == -1)
		return false;

	pInfo->EprocessOffsets.SectionObject = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "SectionObject");
	if (pInfo->EprocessOffsets.SectionObject == -1)
		return false;

	pInfo->EprocessOffsets.UniqueProcessId = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "UniqueProcessId");
	if (pInfo->EprocessOffsets.UniqueProcessId == -1)
		return false;

	pInfo->EprocessOffsets.ExitTime = SymbolHelper::GetKernelStructMemberOffset("_EPROCESS", "ExitTime");
	if (pInfo->EprocessOffsets.ExitTime == -1)
		return false;

	return true;
}

bool CSystemConfigDlg::InitEthreadOffsets(DbgSysCoreInfo* pInfo) {
	pInfo->EthreadOffsets.RundownProtect = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "RundownProtect");
	if (pInfo->EthreadOffsets.RundownProtect == -1)
		return false;

	pInfo->EthreadOffsets.CrossThreadFlags = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "CrossThreadFlags");
	if (pInfo->EthreadOffsets.CrossThreadFlags == -1)
		return false;

	pInfo->EthreadOffsets.ClonedThread = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "ClonedThread");
	if (pInfo->EthreadOffsets.ClonedThread != -1) {
		USHORT pos = SymbolHelper::GetKernelBitFieldPos("_ETHREAD", "ClonedThread");
		pInfo->EthreadOffsets.ClonedThreadBitField.Position = pos;
		USHORT size = SymbolHelper::GetKernelStructMemberSize("_ETHREAD", "ClonedThread");
		pInfo->EthreadOffsets.ClonedThreadBitField.Size = size;
	}
	else {
		return false;
	}

	pInfo->EthreadOffsets.Cid = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "Cid");
	if (pInfo->EthreadOffsets.Cid == -1)
		return false;

	pInfo->EthreadOffsets.ThreadInserted = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "ThreadInserted");
	if (pInfo->EthreadOffsets.ThreadInserted != -1) {
		USHORT pos = SymbolHelper::GetKernelBitFieldPos("_ETHREAD", "ThreadInserted");
		pInfo->EthreadOffsets.ThreadInsertedBitField.Position = pos;
		USHORT size = SymbolHelper::GetKernelStructMemberSize("_ETHREAD", "ThreadInserted");
		pInfo->EthreadOffsets.ThreadInsertedBitField.Size = size;
	}
	else {
		return false;
	}

	pInfo->EthreadOffsets.Tcb = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "Tcb");
	if (pInfo->EthreadOffsets.Tcb == -1)
		return false;

	pInfo->EthreadOffsets.StartAddress = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "StartAddress");
	if (pInfo->EthreadOffsets.StartAddress == -1)
		return false;

	pInfo->EthreadOffsets.Win32StartAddress = SymbolHelper::GetKernelStructMemberOffset("_ETHREAD", "Win32StartAddress");
	if (pInfo->EthreadOffsets.Win32StartAddress == -1)
		return false;

	return true;
}

bool CSystemConfigDlg::InitPebOffsets(DbgSysCoreInfo* pInfo) {
	pInfo->PebOffsets.Ldr = SymbolHelper::GetKernelStructMemberOffset("_PEB", "Ldr");
	if (!pInfo->PebOffsets.Ldr)
		return false;

	return true;
}

bool CSystemConfigDlg::InitKthreadOffsets(DbgSysCoreInfo* pInfo) {
	pInfo->KthreadOffsets.ApcState = SymbolHelper::GetKernelStructMemberOffset("_KTHREAD", "ApcState");
	if (pInfo->KthreadOffsets.ApcState == -1)
		return false;

	pInfo->KthreadOffsets.ApcStateIndex = SymbolHelper::GetKernelStructMemberOffset("_KTHREAD", "ApcStateIndex");
	if (pInfo->KthreadOffsets.ApcStateIndex == -1)
		return false;

	return true;
}