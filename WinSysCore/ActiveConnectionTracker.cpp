#include "pch.h"
#include "ActiveConnectionTracker.h"

#pragma comment(lib,"Iphlpapi")

using namespace WinSys;


int WinSys::ActiveConnectionTracker::EnumConnections() {
	DWORD size = 1 << 16;
	auto orgSize = size;
	DWORD error = NO_ERROR;
	void* buffer = nullptr;

	bool first = _connections.empty();
	_newConnections.clear();
	_closedConnections.clear();
	if (first) {
		_connectionMap.reserve(512);
		_newConnections.reserve(16);
		_closedConnections.reserve(16);
	}

	auto map = _connectionMap;
	std::vector<std::shared_ptr<Connection>> local;
	local.reserve(256);
	if ((_trackedConnections & ConnectionType::Tcp) == ConnectionType::Tcp) {
		do
		{
			buffer = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (!buffer)
				return -1;
			error = ::GetExtendedTcpTable(buffer, &size, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);
			if (error != NO_ERROR)
				::VirtualFree(buffer, 0, MEM_RELEASE);
		} while (error == ERROR_INSUFFICIENT_BUFFER);

		if (error == NOERROR) {
			auto table = (PMIB_TCPTABLE_OWNER_MODULE)buffer;
			AddTcp4Connections(table, map, local, first);
			::VirtualFree(buffer, 0, MEM_RELEASE);
		}
	}
	if ((_trackedConnections & ConnectionType::TcpV6) == ConnectionType::TcpV6) {
		size = orgSize;
		do
		{
			buffer = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (!buffer)
				return -1;
			error = ::GetExtendedTcpTable(buffer, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0);
			if (error != NO_ERROR)
				::VirtualFree(buffer, 0, MEM_RELEASE);
		} while (error == ERROR_INSUFFICIENT_BUFFER);
		if (error == NOERROR) {
			auto table = (PMIB_TCP6TABLE_OWNER_MODULE)buffer;
			AddTcp6Connections(table, map, local, first);
			::VirtualFree(buffer, 0, MEM_RELEASE);
		}
	}
	if ((_trackedConnections & ConnectionType::Udp) == ConnectionType::Udp) {
		size = orgSize;
		do
		{
			buffer = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (!buffer)
				return -1;
			error = ::GetExtendedUdpTable(buffer, &size, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0);
			if (error != NO_ERROR)
				::VirtualFree(buffer, 0, MEM_RELEASE);
		} while (error == ERROR_INSUFFICIENT_BUFFER);
		if (error == NOERROR) {
			auto table = (PMIB_UDPTABLE_OWNER_MODULE)buffer;
			AddUdp4Connections(table, map, local, first);
			::VirtualFree(buffer, 0, MEM_RELEASE);
		}
	}
	if ((_trackedConnections & ConnectionType::UdpV6) == ConnectionType::UdpV6) {
		size = orgSize;
		do
		{
			buffer = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (!buffer)
				return -1;
			error = ::GetExtendedUdpTable(buffer, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0);
			if (error != NO_ERROR)
				::VirtualFree(buffer, 0, MEM_RELEASE);
		} while (error == ERROR_INSUFFICIENT_BUFFER);
		if (error == NOERROR) {
			auto table = (PMIB_UDP6TABLE_OWNER_MODULE)buffer;
			AddUdp6Connections(table, map, local, first);
			::VirtualFree(buffer, 0, MEM_RELEASE);
		}
	}

	for (const auto& [key, conn] : map) {
		_closedConnections.push_back(conn);
		_connectionMap.erase(*conn);
	}

	if (!first) {
		_connections = std::move(local);
	}

	return static_cast<int>(_connections.size());
}

void WinSys::ActiveConnectionTracker::SetTrackingFlags(ConnectionType type) {
	_trackedConnections = type;
}

ConnectionType ActiveConnectionTracker::GetTrackingFlags() const {
	return _trackedConnections;
}

const std::vector<std::shared_ptr<Connection>>& ActiveConnectionTracker::GetConnections() const {
	return _connections;
}

const std::vector<std::shared_ptr<Connection>>& ActiveConnectionTracker::GetCloseConnections() const {
	return _closedConnections;
}

const std::vector<std::shared_ptr<Connection>>& ActiveConnectionTracker::GetNewConnections() const {
	return _newConnections;
}

void ActiveConnectionTracker::AddTcp4Connections(PMIB_TCPTABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first) {
	auto count = table->dwNumEntries;
	auto& items = table->table;

	for (DWORD i = 0; i < count; i++) {
		const auto& item = items[i];
		std::shared_ptr<Connection> conn;
		if (first) {
			conn = std::make_shared<Connection>();
			InitTcp4Connection(conn.get(), item);
			_connections.push_back(conn);
			_connectionMap.insert({ *conn,conn });
		}
		else {
			Connection key;
			key.LocalAddress = item.dwLocalAddr;
			key.RemoteAddress = item.dwRemoteAddr;
			key.LocalPort = item.dwLocalPort;
			key.RemotePort = item.dwRemotePort;
			key.Pid = item.dwOwningPid;
			key.Type = ConnectionType::Tcp;

			if (auto it = map.find(key); it != map.end()) {
				conn = it->second;
				map.erase(key);
			}
			else {
				conn = std::make_shared<Connection>();
				InitTcp4Connection(conn.get(), item);
				_connectionMap.insert({ *conn,conn });
				_newConnections.push_back(conn);
			}
			conn->State = (MIB_TCP_STATE)item.dwState;
			local.push_back(conn);
		}
	}
}

void ActiveConnectionTracker::AddTcp6Connections(PMIB_TCP6TABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first) {
	auto count = table->dwNumEntries;
	const auto& items = table->table;

	for (DWORD i = 0; i < count; i++) {
		const auto& item = items[i];
		std::shared_ptr<Connection> conn;
		if (first) {
			conn = std::make_shared<Connection>();
			InitTcp6Connection(conn.get(), item);
			_connections.push_back(conn);
			_connectionMap.insert({ *conn,conn });
		}
		else {
			Connection key;
			key.LocalPort = item.dwLocalPort;
			::memcpy(key.ucLocalAddress, item.ucLocalAddr, sizeof(key.ucLocalAddress));
			::memcpy(key.ucRemoteAddress, item.ucRemoteAddr, sizeof(key.ucRemoteAddress));
			key.RemotePort = item.dwOwningPid;
			key.Pid = item.dwOwningPid;
			key.Type = ConnectionType::TcpV6;

			if (auto it = map.find(key); it != map.end()) {
				conn = it->second;
				map.erase(key);
			}
			else {
				conn = std::make_shared<Connection>();
				InitTcp6Connection(conn.get(), item);
				_connectionMap.insert({ *conn,conn });
				_newConnections.push_back(conn);
			}
			conn->State = (MIB_TCP_STATE)item.dwState;
			local.push_back(conn);
		}
	}
}

void ActiveConnectionTracker::AddUdp4Connections(PMIB_UDPTABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first) {
	auto count = table->dwNumEntries;
	const auto& items = table->table;

	for (DWORD i = 0; i < count; i++) {
		const auto& item = items[i];
		std::shared_ptr<Connection> conn;
		if (first) {
			conn = std::make_shared<Connection>();
			InitUdp4Connection(conn.get(), item);
			_connections.push_back(conn);
			_connectionMap.insert({ *conn,conn });
		}
		else {
			Connection key;
			key.LocalAddress = item.dwLocalAddr;
			key.LocalPort = item.dwLocalPort;
			key.Pid = item.dwOwningPid;
			key.Type = ConnectionType::Udp;

			if (auto it = map.find(key); it != map.end()) {
				conn = it->second;
				map.erase(key);
			}
			else {
				conn = std::make_shared<Connection>();
				InitUdp4Connection(conn.get(), item);
				_connectionMap.insert({ key,conn });
				_newConnections.push_back(conn);
			}
			local.push_back(conn);
		}
	}
}

void ActiveConnectionTracker::AddUdp6Connections(PMIB_UDP6TABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first) {
	auto count = table->dwNumEntries;
	const auto& items = table->table;

	for (DWORD i = 0; i < count; i++) {
		const auto& item = items[i];
		std::shared_ptr<Connection> conn;
		if (first) {
			conn = std::make_shared<Connection>();
			InitUdp6Connection(conn.get(), item);
			_connections.push_back(conn);
			_connectionMap.insert({ *conn, conn });
		}
		else {
			Connection key;
			::memcpy(key.ucLocalAddress, item.ucLocalAddr, sizeof(item.ucLocalAddr));
			key.LocalPort = item.dwLocalPort;
			key.Pid = item.dwOwningPid;
			key.Type = ConnectionType::UdpV6;

			if (auto it = map.find(key); it != map.end()) {
				conn = it->second;
				map.erase(key);
			}
			else {
				conn = std::make_shared<Connection>();
				InitUdp6Connection(conn.get(), item);
				_connectionMap.insert({ key, conn });
				_newConnections.push_back(conn);
			}
			local.push_back(conn);
		}
	}
}

void WinSys::ActiveConnectionTracker::InitUdp4Connection(Connection* conn, const MIB_UDPROW_OWNER_MODULE& item) const {
	DWORD size = 1 << 10;
	auto moduleInfo = (PTCPIP_OWNER_MODULE_BASIC_INFO)_buffer;
	if (ERROR_SUCCESS == ::GetOwnerModuleFromUdpEntry(const_cast<PMIB_UDPROW_OWNER_MODULE>(&item), TCPIP_OWNER_MODULE_INFO_BASIC, moduleInfo, &size)) {
		conn->ModuleName = moduleInfo->pModuleName;
		conn->ModulePath = moduleInfo->pModulePath;
	}
	conn->LocalPort = item.dwLocalPort;
	conn->LocalAddress = item.dwLocalAddr;
	conn->Type = ConnectionType::Udp;
	conn->State = (MIB_TCP_STATE)0;
	conn->TimeStamp = item.liCreateTimestamp.QuadPart;
}

void ActiveConnectionTracker::InitUdp6Connection(Connection* conn, const MIB_UDP6ROW_OWNER_MODULE& item) const {
	conn->LocalPort = item.dwLocalPort;
	::memcpy(conn->ucLocalAddress, item.ucLocalAddr, sizeof(item.ucLocalAddr));
	conn->Pid = item.dwOwningPid;
	conn->Type = ConnectionType::UdpV6;
	conn->State = (MIB_TCP_STATE)0;
	conn->TimeStamp = item.liCreateTimestamp.QuadPart;
	DWORD size = 1 << 10;
	auto moduleInfo = (PTCPIP_OWNER_MODULE_BASIC_INFO)_buffer;
	if (ERROR_SUCCESS == ::GetOwnerModuleFromUdp6Entry(const_cast<PMIB_UDP6ROW_OWNER_MODULE>(&item), TCPIP_OWNER_MODULE_INFO_BASIC, moduleInfo, &size)) {
		conn->ModuleName = moduleInfo->pModuleName;
		conn->ModulePath = moduleInfo->pModulePath;
	}

}

void ActiveConnectionTracker::InitTcp4Connection(Connection* conn, const MIB_TCPROW_OWNER_MODULE& item) const {
	DWORD size = 1 << 10;
	auto moduleInfo = (PTCPIP_OWNER_MODULE_BASIC_INFO)_buffer;
	if (ERROR_SUCCESS == ::GetOwnerModuleFromTcpEntry(const_cast<PMIB_TCPROW_OWNER_MODULE>(&item), TCPIP_OWNER_MODULE_INFO_BASIC, moduleInfo, &size)) {
		conn->ModuleName = moduleInfo->pModuleName;
		conn->ModulePath = moduleInfo->pModulePath;
	}
	conn->Pid = item.dwOwningPid;
	conn->TimeStamp = item.liCreateTimestamp.QuadPart;
	conn->LocalPort = item.dwLocalPort;
	conn->LocalAddress = item.dwLocalAddr;
	conn->RemoteAddress = item.dwRemoteAddr;
	conn->RemotePort = item.dwRemotePort;
	conn->Type = ConnectionType::Tcp;
	conn->State = (MIB_TCP_STATE)item.dwState;
}

void ActiveConnectionTracker::InitTcp6Connection(Connection* conn, const MIB_TCP6ROW_OWNER_MODULE& item) const {
	DWORD size = 1 << 10;
	auto moduleInfo = (PTCPIP_OWNER_MODULE_BASIC_INFO)_buffer;
	if (ERROR_SUCCESS == ::GetOwnerModuleFromTcp6Entry(const_cast<PMIB_TCP6ROW_OWNER_MODULE>(&item), TCPIP_OWNER_MODULE_INFO_BASIC, moduleInfo, &size)) {
		conn->ModuleName = moduleInfo->pModuleName;
		conn->ModulePath = moduleInfo->pModulePath;
	}
	conn->Pid = item.dwOwningPid;
	conn->LocalPort = item.dwLocalPort;
	conn->RemotePort = item.dwRemotePort;
	::memcpy(conn->ucLocalAddress, item.ucLocalAddr, sizeof(conn->ucLocalAddress));
	::memcpy(conn->ucRemoteAddress, item.ucRemoteAddr, sizeof(conn->ucRemoteAddress));
	conn->TimeStamp = item.liCreateTimestamp.QuadPart;
	conn->Type = ConnectionType::TcpV6;
	conn->State = (MIB_TCP_STATE)item.dwState;
}



