#pragma once

#include <iphlpapi.h>
#include <string>
#include "Keys.h"

namespace WinSys {
	class ActiveConnectionTracker {
	public:
		int EnumConnections();
		void Reset();

		void SetTrackingFlags(ConnectionType type);
		ConnectionType GetTrackingFlags() const;

		using ConnectionMap = std::unordered_map<Connection, std::shared_ptr<Connection>>;
		using ConnectionVec = std::vector<std::shared_ptr<Connection>>;

		const ConnectionVec& GetConnections() const;
		const ConnectionVec& GetNewConnections() const;
		const ConnectionVec& GetCloseConnections() const;

	private:
		void InitTcp4Connection(Connection* conn, const MIB_TCPROW_OWNER_MODULE& item) const;
		void InitTcp6Connection(Connection* conn, const MIB_TCP6ROW_OWNER_MODULE& item) const;
		void InitUdp4Connection(Connection* conn, const MIB_UDPROW_OWNER_MODULE& item) const;
		void InitUdp6Connection(Connection* conn, const MIB_UDP6ROW_OWNER_MODULE& item) const;

		void AddTcp4Connections(PMIB_TCPTABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first);
		void AddTcp6Connections(PMIB_TCP6TABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first);
		void AddUdp4Connections(PMIB_UDPTABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first);
		void AddUdp6Connections(PMIB_UDP6TABLE_OWNER_MODULE table, ConnectionMap& map, ConnectionVec& local, bool first);

		ConnectionType _trackedConnections{ ConnectionType::All };
		ConnectionVec _connections;
		ConnectionVec _newConnections;
		ConnectionVec _closedConnections;
		ConnectionMap _connectionMap;
		inline static thread_local BYTE _buffer[1 << 10];
	};
}


