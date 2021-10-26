#pragma once
#include <memory>
#include <string>
#include "ServiceManager.h"

namespace WinSys {
	enum class ServiceTriggerType {
		Custom = 20, // Custom event based on Event Tracing for Windows(ETW)
		DeviceInterfaceArrival = 1,// Device arrival into the system
		DomainJoin = 3,// Machine joined a domain
		FirewallPortEvent = 4,// Firewall port open or closed
		GroupPolicy = 5,// Machine policy or user policy changed
		IpAddressAvailability = 2,// TCP/IP address available
		NetworkEndPoint = 6, // Packet arrives on a particular network interface (Windows8+)
		SystemStateChanged = 7,
		Aggregate = 30
	};

	enum class ServiceSidType {
		None = SERVICE_SID_TYPE_NONE,
		Unrestricted = SERVICE_SID_TYPE_UNRESTRICTED,
		Restricted = SERVICE_SID_TYPE_RESTRICTED,
		Unknow = -1
	};

	enum class ServiceTriggerAction {
		Start=1,
		Stop=2
	};

	struct ServiceTrigger {
		ServiceTriggerType Type;
		ServiceTriggerAction Action;
		GUID* TriggerSubType;
		uint32_t DataItems;
		void* pSpecificDataItems;
	};

	static_assert(sizeof(ServiceTrigger) == sizeof(SERVICE_TRIGGER));

	class Service final {
	public:
		explicit Service(wil::unique_schandle) noexcept;

		static std::unique_ptr<Service> Open(const std::wstring& name, WinSys::ServiceAccessMask access) noexcept;

		ServiceStatusProcess GetStatus() const;
		std::vector<ServiceTrigger> GetTriggers() const;
		std::vector<std::wstring> GetRequiredPrivileges() const;

		ServiceSidType GetSidType() const;

		bool Start();
		bool Start(const std::vector<const wchar_t*>& args);
		bool Stop();
		bool Pause();
		bool Continue();
		//bool Delete();

		bool Refresh(ServiceInfo& info);

	private:
		wil::unique_schandle _handle;
	};
}