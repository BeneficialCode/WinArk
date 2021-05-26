#pragma once

#include <stdint.h>
#include <algorithm>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <tcpmib.h>

namespace WinSys {
	struct ProcessOrThreadKey {
		int64_t Created;
		uint32_t Id;

		bool operator==(const ProcessOrThreadKey& other) const {
			return other.Created == Created && other.Id == Id;
		}
	};

	struct HandleKey {
		uint32_t ProcessId;
		uint32_t Handle;
		size_t Object;

		HandleKey(uint32_t pid, uint32_t handle, size_t obj) :ProcessId(pid), Handle(handle), Object(obj) {}
		HandleKey() { }

		bool operator==(const HandleKey& other) const {
			return other.Handle == Handle && other.ProcessId == ProcessId && Object == other.Object;
		}
	};

	enum class ConnectionType :DWORD {
		Invalid = 0,
		Tcp = 1, TcpV6 = 2, Udp = 4, UdpV6 = 8,
		All = 15
	};
	DEFINE_ENUM_FLAG_OPERATORS(ConnectionType);

	struct Connection {
		MIB_TCP_STATE State{};
		DWORD Pid;
		ConnectionType Type;
		union {
			DWORD LocalAddress;
			IN6_ADDR LocalAddressV6;
			UCHAR ucLocalAddress[16]{ 0 };
		};
		union {
			DWORD RemoteAddress;
			IN6_ADDR RemoteAddressV6;
			UCHAR ucRemoteAddress[16]{ 0 };
		};
		DWORD LocalPort;
		DWORD RemotePort{ 0 };
		LONGLONG TimeStamp{ 0 };
		std::wstring ModuleName;
		std::wstring ModulePath;

		bool operator==(const Connection& other) const {
			return
				Type == other.Type &&
				LocalAddress == other.LocalAddress &&
				LocalPort == other.LocalPort &&
				Pid == other.Pid;
		}
	};
}

// 自定义hash函数
template<>
struct std::hash<WinSys::ProcessOrThreadKey> {
	size_t operator()(const WinSys::ProcessOrThreadKey& key) const {
		return key.Created ^ key.Id;
	}
};

// 模板特化
template<>
struct std::hash<WinSys::HandleKey> {
	size_t operator()(const WinSys::HandleKey& key) const {
		return key.Handle ^ key.ProcessId ^ key.Object;
	}
};

template<>
struct std::hash<WinSys::Connection> {
	const size_t operator()(const WinSys::Connection& conn) const {
		return conn.Pid ^ conn.LocalAddress ^ (conn.LocalPort << 6) ^ ((int)conn.Type << 20);
	}
};