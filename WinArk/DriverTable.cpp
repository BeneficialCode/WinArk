#include "stdafx.h"
#include "DriverTable.h"
#include "ServiceManager.h"
#include "Helpers.h"

using namespace WinSys;

const CString AccessDenied(L"<access denied>");

CDriverTable::CDriverTable(BarInfo& bars, TableInfo& table)
	:CTable(bars,table){
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

LRESULT CDriverTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CDriverTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CDriverTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CDriverTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDriverTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDriverTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDriverTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDriverTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDriverTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CDriverTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDriverTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}

void CDriverTable::Refresh() {
	
	m_ServicesEx.clear();
	m_Table.data.info = WinSys::ServiceManager::EnumServices(ServiceEnumType::AllDrivers);
	m_Table.data.n = m_Table.data.info.size();
	m_ServicesEx.reserve(m_Table.data.n);
}

int CDriverTable::ParseTableEntry(CString& s, char& mask, int& select, WinSys::ServiceInfo& info, int column) {
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
			s = config ? ServiceStartTypeToString(*config) : AccessDenied;
			break;
		case 5:
			s = svcex.GetDescription();
			break;
		case 6:
			s = config ? CString(config->BinaryPathName.c_str()) : AccessDenied;
			break;
		default:
			break;
	}

	return s.GetLength();
}

PCWSTR CDriverTable::ServiceStateToString(WinSys::ServiceState state) {
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

CString CDriverTable::ServiceTypeToString(WinSys::ServiceType type) {
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

CString CDriverTable::TriggerToText(const WinSys::ServiceTrigger& trigger) {
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

CString CDriverTable::DependenciesToString(const std::vector<std::wstring>& deps) {
	CString text;
	for (auto& dep : deps)
		text += CString(dep.c_str()) + L", ";
	return text.Left(text.GetLength() - 2);
}

CString CDriverTable::ServiceStartTypeToString(const ServiceConfiguration& config) {
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

ServiceInfoEx& CDriverTable::GetServiceInfoEx(const std::wstring& name) const {
	auto it = m_ServicesEx.find(name);
	if (it != m_ServicesEx.end())
		return it->second;

	ServiceInfoEx infox(name.c_str());
	auto pos = m_ServicesEx.insert({ name,std::move(infox) });
	return pos.first->second;
}


