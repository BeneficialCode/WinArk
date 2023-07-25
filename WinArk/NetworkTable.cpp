#include "stdafx.h"
#include "NetworkTable.h"
#include <ip2string.h>
#include "SortHelper.h"

using namespace WinSys;

#pragma comment(lib,"Ws2_32")

LRESULT CNetwrokTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return 0;
}

LRESULT CNetwrokTable::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
	return 0;
}

LRESULT CNetwrokTable::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CNetwrokTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	bHandled = false;
	return 0;
}

LRESULT CNetwrokTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CNetwrokTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CNetwrokTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_CONTEXT);
	hSubMenu = menu.GetSubMenu(9);
	POINT pt;
	::GetCursorPos(&pt);

	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
}

LRESULT CNetwrokTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CNetwrokTable::OnGetDlgCode(UINT, WPARAM, LPARAM, BOOL&) {
	return DLGC_WANTARROWS;
}

CNetwrokTable::ItemEx* CNetwrokTable::GetItemEx(WinSys::Connection* conn) const {
	if (auto it = m_ItemsEx.find(*conn); it != m_ItemsEx.end())
		return &it->second;
	return nullptr;
}

void CNetwrokTable::AddConnection(std::vector<std::shared_ptr<WinSys::Connection>>& vec, std::shared_ptr<WinSys::Connection> conn, bool reallyNew /* = true */) {
	vec.push_back(conn);
	auto pix = GetItemEx(conn.get());
	ItemEx ix;
	if (pix == nullptr)
		pix = &ix;
	if (pix->ProcessName.IsEmpty()) {
		pix->ProcessName = m_pm.GetProcessNameById(conn->Pid).c_str();
		m_ItemsEx.insert({ *conn,ix });
	}
}

const CString CNetwrokTable::GetProcessName(WinSys::Connection* conn) const {
	auto itemx = GetItemEx(conn);
	if (itemx)
		return itemx->ProcessName;
	ItemEx ix;
	ix.ProcessName = m_pm.GetProcessNameById(conn->Pid).c_str();
	m_ItemsEx.insert({ *conn,ix });
	return ix.ProcessName;
}

void CNetwrokTable::DoRefresh() {
	m_Table.data.n = 0;
	if (m_Table.data.info.empty()) {
		// first time
		auto count = m_Tracker.EnumConnections();
		for (auto& conn : m_Tracker.GetConnections()) {
			AddConnection(m_Table.data.info, conn, false);
		}
		m_Table.data.n = count;
		m_ItemsEx.reserve(count + 16);
	}
	else {
		decltype(m_Table.data.info) local;
		local.reserve(m_Table.data.info.size());

		auto time = ::GetTickCount64();
		int newCount = (int)m_NewConnections.size();
		for (int i = 0; i < newCount; i++) {
			auto& conn = m_NewConnections[i];
			auto ix = GetItemEx(conn.get());
			if (ix->TargetTime <= time) {
				ix->State = ItemState::None;
				m_NewConnections.erase(m_NewConnections.begin() + i);
				i--;
				newCount--;
			}
		}

		auto oldCount = (int)m_OldConnections.size();
		for (int i = 0; i < oldCount; i++) {
			auto& conn = m_OldConnections[i];
			auto ix = GetItemEx(conn.get());
			if (ix && ix->TargetTime <= time) {
				m_OldConnections.erase(m_OldConnections.begin() + i);
				i--;
				oldCount--;
			}
			else {
				local.push_back(conn);
			}
		}

		m_Tracker.EnumConnections();
		for (auto& conn : m_Tracker.GetConnections()) {
			AddConnection(local, conn);
		}

		for (auto& conn : m_Tracker.GetNewConnections()) {
			auto ix = GetItemEx(conn.get());
			ATLASSERT(ix);
			ix->State = ItemState::New;
			ix->TargetTime = time + GetUpdateInterval();
			m_NewConnections.push_back(conn);
		}

		for (auto& conn : m_Tracker.GetCloseConnections()) {
			AddConnection(local, conn);
			auto ix = GetItemEx(conn.get());
			ATLASSERT(ix);
			ix->State = ItemState::Deleted;
			ix->TargetTime = time + GetUpdateInterval();
			m_OldConnections.push_back(conn);
		}

		m_Table.data.info = std::move(local);
		m_Table.data.n = m_Table.data.info.size();
	}
}

PCWSTR CNetwrokTable::ConnectionTypeToString(ConnectionType type) {
	switch (type) {
	case WinSys::ConnectionType::Tcp:
		return L"TCP";
	case WinSys::ConnectionType::TcpV6:
		return L"TCPv6";
	case WinSys::ConnectionType::Udp:
		return L"UDP";
	case WinSys::ConnectionType::UdpV6:
		return L"UDPv6";
	}
	ATLASSERT(false);
	return nullptr;
}

PCWSTR CNetwrokTable::ConnectionStateToString(MIB_TCP_STATE state) {
	switch (state) {
	case MIB_TCP_STATE_CLOSED: return L"Closed";
	case MIB_TCP_STATE_LISTEN: return L"Listen";
	case MIB_TCP_STATE_SYN_SENT: return L"Syn Sent";
	case MIB_TCP_STATE_SYN_RCVD: return L"Syn Received";
	case MIB_TCP_STATE_ESTAB: return L"Established";
	case MIB_TCP_STATE_FIN_WAIT1: return L"Fin Wait 1";
	case MIB_TCP_STATE_FIN_WAIT2: return L"Fin Wait 2";
	case MIB_TCP_STATE_CLOSE_WAIT:return L"Close Wait";
	case MIB_TCP_STATE_CLOSING:return L"Closing";
	case MIB_TCP_STATE_LAST_ACK:return L"Ack";
	case MIB_TCP_STATE_TIME_WAIT:return L"Time Wait";
	case MIB_TCP_STATE_DELETE_TCB:return L"Delete TCB";
	}
	return L"";
}

CString CNetwrokTable::IPAddressToString(DWORD ip) {
	WCHAR buffer[46];
	::RtlIpv4AddressToString((in_addr*)&ip, buffer);
	return buffer;
}

CString CNetwrokTable::IPAddressToString(const IN6_ADDR& ip) {
	WCHAR buffer[46];
	::RtlIpv6AddressToString((in6_addr*)&ip, buffer);
	return buffer;
}

DWORD CNetwrokTable::SwapBytes(DWORD x) {
	auto p = (BYTE*)&x;
	std::swap(p[0], p[3]);
	std::swap(p[1], p[2]);
	return x;
}

CNetwrokTable::CNetwrokTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	m_pm.EnumProcesses();
	DoRefresh();
}

u_short CNetwrokTable::HostByteOrderToNetworkByteOrder(UINT port) {
	return htons((WORD)port);
}

int CNetwrokTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::Connection>& info, int column) {
	switch (column) {
	case 0:
		s = info->Pid == 0 ? L"" : GetProcessName(info.get()).GetString();
		break;
	case 1:
		if (info->Pid > 0)
			s.Format(L"%6u (0x%05X)", info->Pid, info->Pid);
		break;
	case 2:
		s = ConnectionTypeToString(info->Type);
		break;
	case 3:
		s = ConnectionStateToString(info->State);
		if (info->State == MIB_TCP_STATE_ESTAB) {
			select |= DRAW_HILITE;
		}
		break;
	case 4:
		s = info->Type == ConnectionType::TcpV6 || info->Type == ConnectionType::UdpV6 ?
			IPAddressToString(info->LocalAddressV6) : IPAddressToString(info->LocalAddress);
		break;
	case 5:
		s.Format(L"%d", HostByteOrderToNetworkByteOrder(info->LocalPort));
		break;
	case 6:
		if (info->Type == ConnectionType::Udp || info->Type == ConnectionType::UdpV6) {
			break;
		}
		s = info->Type == ConnectionType::TcpV6 ? IPAddressToString(info->RemoteAddressV6) : IPAddressToString(info->RemoteAddress);
		break;
	case 7:
		if (info->Type == ConnectionType::Udp || info->Type == ConnectionType::UdpV6) {
			break;
		}
		s.Format(L"%d", HostByteOrderToNetworkByteOrder(info->RemotePort));
		break;
	case 8:
		if (info->TimeStamp)
			s.Format(L"%s.%03d", (PCWSTR)CTime(*(FILETIME*)&info->TimeStamp).Format(L"%x %X"), info->TimeStamp % 1000);
		break;
	case 9:
		s = info->ModuleName.c_str();
	case 10:
		s = info->ModulePath.c_str();
	default:
		break;
	}
	return s.GetLength();
}

bool CNetwrokTable::CompareItems(const std::shared_ptr<WinSys::Connection>& c1, const std::shared_ptr<WinSys::Connection>& c2, int col, bool asc) {
	switch (col)
	{
	case 0: return SortHelper::SortStrings(GetProcessName(c1.get()), GetProcessName(c2.get()), asc);
	case 1: return SortHelper::SortNumbers(c1->Pid, c2->Pid, asc);
	case 2: return SortHelper::SortStrings(ConnectionTypeToString(c1->Type), ConnectionTypeToString(c2->Type), asc);
	case 3: return SortHelper::SortNumbers(c1->State, c2->State, asc);
	case 4: return SortHelper::SortNumbers(SwapBytes(c1->LocalAddress), SwapBytes(c2->LocalAddress), asc);
	case 5:
	{
		auto port1 = HostByteOrderToNetworkByteOrder(c1->LocalPort);
		auto port2 = HostByteOrderToNetworkByteOrder(c2->LocalPort);
		return SortHelper::SortNumbers(port1, port2, asc);
	}
	case 6: return SortHelper::SortNumbers(SwapBytes(c1->RemoteAddress), SwapBytes(c2->RemoteAddress), asc);

	case 7:
	{
		auto port1 = HostByteOrderToNetworkByteOrder(c1->RemotePort);
		auto port2 = HostByteOrderToNetworkByteOrder(c2->RemotePort);
		return SortHelper::SortNumbers(port1, port2, asc);
	}
	case 8: return SortHelper::SortNumbers(c1->TimeStamp, c2->TimeStamp, asc);
	case 9: return SortHelper::SortStrings(c1->ModuleName, c2->ModuleName, asc);
	default:
		break;
	}
	return false;
}

LRESULT CNetwrokTable::OnRefresh(WORD, WORD, HWND, BOOL&) {
	m_pm.EnumProcesses();
	DoRefresh();
	return 0;
}