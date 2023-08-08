#include "stdafx.h"
#include "ServiceTable.h"
#include "SortHelper.h"
#include "ProcessPropertiesDlg.h"
#include "ProcessInfoEx.h"
#include "ProgressDlg.h"

using namespace WinSys;

const CString AccessDenied(L"<access denied>");

CServiceTable::CServiceTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

LRESULT CServiceTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CServiceTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CServiceTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CServiceTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CServiceTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CServiceTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CServiceTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CServiceTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CServiceTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CServiceTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_CONTEXT);
	hSubMenu = menu.GetSubMenu(11);
	POINT pt;
	::GetCursorPos(&pt);
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];
	auto& pdata = svc.GetStatusProcess();
	auto& state = pdata.CurrentState;
	bool enable = state == ServiceState::Stopped &&
		GetServiceInfoEx(svc.GetName()).GetConfiguration()->StartType != ServiceStartType::Disabled;
	if(!enable)
		EnableMenuItem(hSubMenu, ID_SERVICE_START, MF_DISABLED);
	enable = state == ServiceState::Running;
	if (!enable) {
		EnableMenuItem(hSubMenu, ID_SERVICE_STOP, MF_DISABLED);
		EnableMenuItem(hSubMenu, ID_SERVICE_PAUSE, MF_DISABLED);
		EnableMenuItem(hSubMenu, ID_SERVICE_STOP, MF_DISABLED);
	}
	enable = state == ServiceState::Paused;
	if(!enable)
		EnableMenuItem(hSubMenu, ID_SERVICE_CONTINUE, MF_DISABLED);

	enable = pdata.ProcessId > 0 ? true : false;
	if (!enable)
		EnableMenuItem(hSubMenu, ID_SERVICE_EXPORT_BY_PID, MF_DISABLED);
	
	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
}

LRESULT CServiceTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CServiceTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

void CServiceTable::Refresh() {
	m_ProcMgr.EnumProcesses();
	m_ServicesEx.clear();
	m_Table.data.info = WinSys::ServiceManager::EnumServices(ServiceEnumType::AllServices);
	m_Table.data.n = m_Table.data.info.size();
	m_ServicesEx.reserve(m_Table.data.n);
}

int CServiceTable::ParseTableEntry(CString& s, char& mask, int& select, WinSys::ServiceInfo& info, int column) {
	auto& pdata = info.GetStatusProcess();
	auto& svcex = GetServiceInfoEx(info.GetName());
	auto config = svcex.GetConfiguration();
	switch (column) {
		case 0:
			s = info.GetName().c_str();
			break;
		case 1:
			s = info.GetDisplayName().c_str();
			break;
		case 2:
			s = ServiceStateToString(pdata.CurrentState);
			break;
		case 3:
			s = ServiceTypeToString(pdata.Type);
			break;
		case 4:
			if (pdata.ProcessId > 0) {
				s.Format(L"%u (0x%X)", pdata.ProcessId, pdata.ProcessId);
			}
			break;
		case 5:
			s = pdata.ProcessId > 0 ? m_ProcMgr.GetProcessNameById(pdata.ProcessId).c_str() : L"";
			break;
		case 6:
			s = config ? ServiceStartTypeToString(*config) : AccessDenied;
			break;
		case 7:
			s = config ? CString(config->BinaryPathName.c_str()) : AccessDenied;
			break;
		case 8:
			s = config ? CString(config->AccountName.c_str()) : AccessDenied;
			break;
		case 9:
			s = config ? ErrorControlToString(config->ErrorControl) : AccessDenied;
			break;
		case 10:
			s = svcex.GetDescription();
			break;
		case 11:
			s = svcex.GetPrivileges();
			break;
		case 12:
			s = svcex.GetTriggers();
			break;
		case 13:
			s = config ? svcex.GetDependencies() : AccessDenied;
			break;
		case 14:
			s = ServiceControlsAcceptedToString(pdata.ControlsAccepted);
			break;
		case 15:
			s = svcex.GetSID();
			break;
		case 16:
			s = ServiceSidTypeToString(svcex.GetSidType());
			break;
	}

	return s.GetLength();
}

PCWSTR CServiceTable::ServiceStateToString(WinSys::ServiceState state) {
	switch (state) {
		case ServiceState::Running:return L"Running";
		case ServiceState::Stopped:return L"Stopped";
		case ServiceState::Paused:return L"Paused";
		case ServiceState::StartPending:return L"Start Pending";
		case ServiceState::StopPending:return L"Stop Pending";
		case ServiceState::PausePending:return L"Pause Pending";
	}
	return L"Unknown";
}

CString CServiceTable::ServiceTypeToString(WinSys::ServiceType type) {
	using namespace WinSys;

	static struct {
		ServiceType type;
		PCWSTR text;
	}types[] = {
		{ServiceType::KernelDriver,L"Kernel Driver"},
		{ServiceType::FileSystemDriver,L"FS/Filter Driver"},
		{ServiceType::Win32OwnProcess,L"Own"},
		{ServiceType::Win32SharedProcess,L"Shared"},
		{ServiceType::InteractiveProcess,L"Interactive"},
		{ServiceType::UserServiceInstance,L"Instance"},
		{ServiceType::PackageService,L"Packaged"},
	};

	CString text;
	for (auto& item : types)
		if ((item.type & type) == item.type)
			text += CString(item.text) + L", ";

	return text.Left(text.GetLength() - 2);
}

PCWSTR CServiceTable::TriggerToText(const WinSys::ServiceTrigger& trigger) {
	using namespace WinSys;
	switch (trigger.Type) {
		case ServiceTriggerType::Custom:return L"Custom";
		case ServiceTriggerType::DeviceInterfaceArrival:return L"Device Arrival";
		case ServiceTriggerType::DomainJoin:return L"Domain Join";
		case ServiceTriggerType::FirewallPortEvent:return L"Firewall Port Event";
		case ServiceTriggerType::GroupPolicy:return L"Group Policy";
		case ServiceTriggerType::IpAddressAvailability: return L"IP Address Availability";
		case ServiceTriggerType::NetworkEndPoint:return L"Network EndPoint";
		case ServiceTriggerType::SystemStateChanged:return L"System State Changed";
		case ServiceTriggerType::Aggregate:return L"Aggregate";
	}
	ATLASSERT(false);
	return L"";
}

CString CServiceTable::DependenciesToString(const std::vector<std::wstring>& deps) {
	CString text;
	for (auto& dep : deps)
		text += CString(dep.c_str()) + L", ";
	return text.Left(text.GetLength() - 2);
}

CString CServiceTable::ServiceStartTypeToString(const ServiceConfiguration& config) {
	CString type;

	switch (config.StartType) {
		case ServiceStartType::Boot:type = L"Boot"; break;
		case ServiceStartType::System:type = L"System"; break;
		case ServiceStartType::Auto:type = L"Automatic"; break;
		case ServiceStartType::Demand:type = L"Manual"; break;
		case ServiceStartType::Disabled:type = L"Disabled"; break;
	}

	if (config.DeleayedAutoStart)
		type += L" (Delayed)";
	if (config.TriggerStart)
		type += L" (Trigger)";
	return type;
}

ServiceInfoEx& CServiceTable::GetServiceInfoEx(const std::wstring& name) const {
	auto it = m_ServicesEx.find(name);
	if (it != m_ServicesEx.end())
		return it->second;

	ServiceInfoEx infox(name.c_str());
	auto pos = m_ServicesEx.insert({ name,std::move(infox) });
	return pos.first->second;
}

CString CServiceTable::ErrorControlToString(WinSys::ServiceErrorControl ec) {
	using enum ServiceErrorControl;

	switch (ec)
	{
		case Ignore:
			return L"Ignore (0)";
			
		case Normal:
			return L"Normal (1)";

		case Severe:
			return L"Servere (2)";

		case Critical:
			return L"Critical (3)";
	}

	ATLASSERT(false);
	return L"";
}

CString CServiceTable::ServiceControlsAcceptedToString(ServiceControlsAccepted accepted) {
	using enum ServiceControlsAccepted;
	if (accepted == None)
		return L"(None)";

	static struct {
		ServiceControlsAccepted type;
		PCWSTR text;
	}types[] = {
		{Stop,L"Stop"},
		{PauseContinue,L"Pause, Continue"},
		{Shutdown,L"Shutdown"},
		{ParamChange,L"Param Change"},
		{NetBindChange,L"NET Bind Change"},
		{HardwareProfileChange,L"Hardware Profile Change"},
		{PowerEvent,L"Power Event"},
		{SessionChange,L"Session Change"},
		{Preshutdown,L"Pre Shutdown"},
		{TimeChanged,L"Time Change"},
		{TriggerEvent,L"Trigger Event"},
		{UserLogOff,L"User Log off"},
		{LowResources,L"Low Resources"},
		{SystemLowResources,L"System Low Resources"},
		{InternalReserved,L"(Internal)"},
	};

	CString text;
	for (auto& item : types) {
		if ((item.type & accepted) == item.type)
			text += CString(item.text) + L", ";
	}

	text = text.Left(text.GetLength() - 2);
	text.Format(L"%s (0x%X)", (PCWSTR)text, (DWORD)accepted);
	return text;
}

PCWSTR CServiceTable::ServiceSidTypeToString(WinSys::ServiceSidType type) {
	switch (type)
	{
		case WinSys::ServiceSidType::None:
			return L"None";
		case WinSys::ServiceSidType::Unrestricted:
			return L"Unrestricted";
		case WinSys::ServiceSidType::Restricted:
			return L"Restricted";
	}
	return L"(Unknown)";
}

bool CServiceTable::CompareItems(const WinSys::ServiceInfo& s1, const WinSys::ServiceInfo& s2, int col, bool asc) {
	
	switch (static_cast<ServiceColumn>(col))		
	{
		case ServiceColumn::Name:
			return SortHelper::SortStrings(s1.GetName(), s2.GetName(), asc);
		case ServiceColumn::DisplayName:
			return SortHelper::SortStrings(s1.GetDisplayName(), s2.GetDisplayName(), asc);
		case ServiceColumn::State:
			return SortHelper::SortStrings(
				ServiceStateToString(s1.GetStatusProcess().CurrentState),
				ServiceStateToString(s2.GetStatusProcess().CurrentState), 
				asc);
		case ServiceColumn::Type:
			return SortHelper::SortNumbers(s1.GetStatusProcess().Type, s2.GetStatusProcess().Type, asc);
		case ServiceColumn::PID:
			return SortHelper::SortNumbers(s1.GetStatusProcess().ProcessId, s2.GetStatusProcess().ProcessId, asc);
		case ServiceColumn::ProcessName:
			return SortHelper::SortStrings(
				m_ProcMgr.GetProcessNameById(s1.GetStatusProcess().ProcessId),
				m_ProcMgr.GetProcessNameById(s2.GetStatusProcess().ProcessId),
					asc);
		case ServiceColumn::StartType:
			return SortHelper::SortStrings(
				ServiceStartTypeToString(*GetServiceInfoEx(s1.GetName()).GetConfiguration()),
				ServiceStartTypeToString(*GetServiceInfoEx(s2.GetName()).GetConfiguration()),
				asc
			);
		case ServiceColumn::BinaryPath:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetConfiguration()->BinaryPathName,
				GetServiceInfoEx(s2.GetName()).GetConfiguration()->BinaryPathName,
				asc
			);
		case ServiceColumn::AccountName:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetConfiguration()->AccountName,
				GetServiceInfoEx(s2.GetName()).GetConfiguration()->AccountName,
				asc
			);
		case ServiceColumn::ErrorControl:
			return SortHelper::SortNumbers(
				GetServiceInfoEx(s1.GetName()).GetConfiguration()->ErrorControl,
				GetServiceInfoEx(s2.GetName()).GetConfiguration()->ErrorControl,
				asc
			);
		case ServiceColumn::Description:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetDescription(),
				GetServiceInfoEx(s2.GetName()).GetDescription(),
				asc
			);
		case ServiceColumn::Privileges:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetPrivileges(),
				GetServiceInfoEx(s2.GetName()).GetPrivileges(),
				asc
			);
		case ServiceColumn::Triggers:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetTriggers(),
				GetServiceInfoEx(s2.GetName()).GetTriggers(),
				asc
			);
		case ServiceColumn::Dependencies:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetDependencies(),
				GetServiceInfoEx(s2.GetName()).GetDependencies(),
				asc
			);
		case ServiceColumn::ControlsAccepted:
			return SortHelper::SortNumbers(
				s1.GetStatusProcess().ControlsAccepted, 
				s2.GetStatusProcess().ControlsAccepted, asc);
		case ServiceColumn::SID:
			return SortHelper::SortStrings(
				GetServiceInfoEx(s1.GetName()).GetSID(), 
				GetServiceInfoEx(s2.GetName()).GetSID(), asc);
		case ServiceColumn::SidType:
			return SortHelper::SortNumbers(GetServiceInfoEx(s1.GetName()).GetSidType(),
				GetServiceInfoEx(s2.GetName()).GetSidType(), asc);
	}
	
	return false;
}

LRESULT CServiceTable::OnServiceStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	auto service = Service::Open(svc.GetName(), ServiceAccessMask::Start | ServiceAccessMask::QueryStatus);
	if (service == nullptr) {
		AtlMessageBox(*this, L"Failed to open service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	if (!service->Start()) {
		AtlMessageBox(*this, L"Failed to start service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	CProgressDlg dlg;
	dlg.ShowCancelButton(false);
	dlg.SetMessageText((L"Starting service" + svc.GetName() + L"...").c_str());
	dlg.SetProgressMarquee(true);
	auto now = ::GetTickCount64();
	dlg.SetTimerCallback([&]() {
		service->Refresh(svc);
		if (svc.GetStatusProcess().CurrentState == ServiceState::Running)
			dlg.Close();
		if (::GetTickCount64() - now > 5000)
			dlg.Close(IDCANCEL);
		}, 500);
	if (dlg.DoModal() == IDCANCEL) {
		AtlMessageBox(*this, L"Failed to start service within 5 seconds", IDS_TITLE, MB_ICONEXCLAMATION);
	}
	Refresh();

	return 0;
}

LRESULT CServiceTable::OnServiceStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	ATLASSERT(svc.GetStatusProcess().CurrentState == ServiceState::Running);

	auto service = Service::Open(svc.GetName(), ServiceAccessMask::Stop | ServiceAccessMask::QueryStatus);
	if (service == nullptr) {
		AtlMessageBox(*this, L"Failed to open service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	if (!service->Stop()) {
		AtlMessageBox(*this, L"Failed to stop service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	CProgressDlg dlg;
	dlg.ShowCancelButton(false);
	dlg.SetMessageText((L"Stopping service " + svc.GetName() + L"...").c_str());
	dlg.SetProgressMarquee(true);
	dlg.SetTimerCallback([&]() {
		service->Refresh(svc);
		if (svc.GetStatusProcess().CurrentState == ServiceState::Stopped)
			dlg.Close();
		}, 500);
	dlg.DoModal();
	Refresh();

	return 0;
}

LRESULT CServiceTable::OnServicePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	ATLASSERT(svc.GetStatusProcess().CurrentState == ServiceState::Running);

	auto service = Service::Open(svc.GetName(), ServiceAccessMask::PauseContinue | ServiceAccessMask::QueryStatus);
	if (service == nullptr) {
		AtlMessageBox(*this, L"Failed to open service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	if (!service->Pause()) {
		AtlMessageBox(*this, L"Failed to pause service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	CProgressDlg dlg;
	dlg.ShowCancelButton(false);
	dlg.SetMessageText((L"Pausing service " + svc.GetName() + L"...").c_str());
	dlg.SetProgressMarquee(true);
	dlg.SetTimerCallback([&]() {
		service->Refresh(svc);
		if (svc.GetStatusProcess().CurrentState == ServiceState::Paused)
			dlg.Close();
		}, 500);
	dlg.DoModal();

	return 0;
}

LRESULT CServiceTable::OnServiceContinue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	ATLASSERT(svc.GetStatusProcess().CurrentState == ServiceState::Paused);

	auto service = Service::Open(svc.GetName(), ServiceAccessMask::PauseContinue | ServiceAccessMask::QueryStatus);
	if (service == nullptr) {
		AtlMessageBox(*this, L"Failed to open service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	if (!service->Continue()) {
		AtlMessageBox(*this, L"Failed to resume service", IDS_TITLE, MB_ICONERROR);
		return 0;
	}

	CProgressDlg dlg;
	dlg.ShowCancelButton(false);
	dlg.SetMessageText((L"Resuming service " + svc.GetName() + L"...").c_str());
	dlg.SetProgressMarquee(true);
	dlg.SetTimerCallback([&]() {
		service->Refresh(svc);
		if (svc.GetStatusProcess().CurrentState == ServiceState::Running)
			dlg.Close();
		}, 500);
	dlg.DoModal();

	return 0;
}

LRESULT CServiceTable::OnServiceProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	AtlMessageBox(*this, L"Not implement yet!");

	return LRESULT();
}

LRESULT CServiceTable::OnServiceDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	CString text;
	text.Format(L"Uninstall the service %s?\n", svc.GetName().c_str());
	if (AtlMessageBox(*this, (PCWSTR)text, IDS_TITLE, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
		auto success = WinSys::ServiceManager::Uninstall(svc.GetName());
		AtlMessageBox(*this, success ? L"Service uninstalled successfully" : L"Failed to uninstall service",
			IDS_TITLE, success ? MB_ICONINFORMATION : MB_ICONERROR);
		Refresh();
	}
	return 0;
}

LRESULT CServiceTable::OnServiceStartAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& svc = m_Table.data.info[selected];

	for (auto& svc : m_Table.data.info) {
		if (svc.GetStatusProcess().CurrentState 
			!= ServiceState::Stopped) {
			continue;
		}
		auto service = Service::Open(svc.GetName(), ServiceAccessMask::Start | ServiceAccessMask::QueryStatus);
		if (service == nullptr) {
			continue;
		}

		if (!service->Start()) {
			continue;
		}

		CProgressDlg dlg;
		dlg.ShowCancelButton(false);
		dlg.SetMessageText((L"Starting service" + svc.GetName() + L"...").c_str());
		dlg.SetProgressMarquee(true);
		auto now = ::GetTickCount64();
		dlg.SetTimerCallback([&]() {
			service->Refresh(svc);
			if (svc.GetStatusProcess().CurrentState == ServiceState::Running)
				dlg.Close();
			if (::GetTickCount64() - now > 5000)
				dlg.Close(IDCANCEL);
			}, 500);
		if (dlg.DoModal() == IDCANCEL) {
			continue;
		}
	}
	Refresh();
	return 0;
}

LRESULT CServiceTable::OnExportByPid(WORD, WORD, HWND, BOOL&) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& p = m_Table.data.info[selected];
	auto& pdata = p.GetStatusProcess();
	DWORD pid = pdata.ProcessId;

	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			if (pid == info.GetStatusProcess().ProcessId) {
				std::wstring text = GetSingleServiceInfo(info);
				Helpers::WriteString(hFile, text);
			}
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CServiceTable::GetSingleServiceInfo(ServiceInfo& info) {
	CString text;
	CString s;

	s = info.GetName().c_str();
	s += L"\t\t\t";
	text += s;

	s = info.GetDisplayName().c_str();
	text += s;

	text += L"\r\n";

	return text.GetString();
}
