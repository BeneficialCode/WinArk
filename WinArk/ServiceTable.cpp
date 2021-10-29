#include "stdafx.h"
#include "ServiceTable.h"
#include "SortHelper.h"
#include "ProcessPropertiesDlg.h"
#include "ProcessInfoEx.h"

using namespace WinSys;

const CString AccessDenied(L"(<access denied>");

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
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
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
		case ServiceState::Stoppend:return L"Stopped";
		case ServiceState::Pause:return L"Paused";
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