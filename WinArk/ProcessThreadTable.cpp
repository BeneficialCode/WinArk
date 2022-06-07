#include "stdafx.h"
#include "ProcessThreadTable.h"
#include "FormatHelper.h"


LRESULT CProcessThreadTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Refresh();
	return 0;
}

LRESULT CProcessThreadTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CProcessThreadTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessThreadTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessThreadTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessThreadTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessThreadTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessThreadTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessThreadTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CProcessThreadTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessThreadTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessThreadTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}

int CProcessThreadTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::ThreadInfo>& info, int column) {
	const auto& tx = GetThreadInfoEx(info.get());
	
	switch (static_cast<ThreadColumn>(column))
	{
	case ThreadColumn::State:
		s = ThreadStateToString(info->ThreadState);
		break;
	case ThreadColumn::Id:
		s.Format(L"%d (0x%05X)", info->Id, info->Id);
		break;
	case ThreadColumn::ProcessId:
		s.Format(L"%d (0x%05X)", info->ProcessId, info->ProcessId);
		break;
	case ThreadColumn::ProcessName:
		s = info->GetProcessImageName().c_str();
		break;
	case ThreadColumn::CPUTime:
		s = FormatHelper::TimeSpanToString(info->UserTime + info->KernelTime);
		break;
	case ThreadColumn::CreateTime:
		s = info->CreateTime < (1LL << 32) ? CString() : FormatHelper::TimeToString(info->CreateTime);
		break;
	case ThreadColumn::Priority:
		s.Format(L"%d", info->Priority);
		break;
	case ThreadColumn::BasePriority:
		s.Format(L"%d", info->BasePriority);
		break;
	case ThreadColumn::Teb:
		if (info->TebBase != nullptr)
			s.Format(L"0x%p", info->TebBase);
		break;
	case ThreadColumn::WaitReason:
		s = info->ThreadState == WinSys::ThreadState::Waiting ? WaitReasonToString(info->WaitReason) : L"";
		break;
	case ThreadColumn::StartAddress:
		if (info->StartAddress)
			s.Format(L"0x%p", info->StartAddress);
		break;
	case ThreadColumn::Win32StartAddress:
		if (info->Win32StartAddress != info->StartAddress)
			s.Format(L"0x%p", info->Win32StartAddress);
		break;
	case ThreadColumn::StackBase:
		s.Format(L"0x%p", info->StackBase);
		break;
	case ThreadColumn::StackLimit:
		s.Format(L"0x%p", info->StackLimit);
		break;
	case ThreadColumn::ContextSwitches:
		s = FormatHelper::FormatWithCommas(info->ContextSwitches);
		break;
	case ThreadColumn::KernelTime:
		s = FormatHelper::TimeSpanToString(info->KernelTime);
		break;
	case ThreadColumn::UserTime:
		s = FormatHelper::TimeSpanToString(info->UserTime);
		break;
	case ThreadColumn::IoPriority:
	{
		auto priority = tx.GetIoPriority();
		if (priority != WinSys::IoPriority::Unknown)
			s.Format(L"%d (%s)", priority, FormatHelper::IoPriorityToString(priority));
		break;
	}
	case ThreadColumn::MemoryPriority:
	{
		auto mp = tx.GetMemoryPriority();
		if (mp >= 0)
			s.Format(L"%d", mp);
	}
		break;
	case ThreadColumn::ComFlags:
	{
		auto flags = tx.GetComFlags();
		if (flags != WinSys::ComFlags::Error && flags != WinSys::ComFlags::None)
			s.Format(L"0x%08X (%s)", flags, FormatHelper::ComFlagsToString(flags));
	}
		break;
	case ThreadColumn::ComApartment:
		s = FormatHelper::ComApartmentToString(tx.GetComFlags());
		break;
	case ThreadColumn::WaitTime:
		s.Format(L"%u.%03d", info->WaitTime / 1000, info->WaitTime % 1000);
		break;
	default:
		break;
	}
	return s.GetLength();
}

CProcessThreadTable::CProcessThreadTable(BarInfo& bars, TableInfo& table, DWORD pid)
	: CTable(bars, table),m_Pid(pid) {
	SetTableWindowInfo(bars.nbar);
	m_Table.data.info.clear();
	Refresh();
}

void CProcessThreadTable::Refresh() {
	auto first = m_Table.data.info.empty();
	m_ProcMgr.EnumProcessAndThreads(m_Pid);
	auto count = (int)m_ProcMgr.GetThreadCount();

	if (first) {
		m_Table.data.info = m_ProcMgr.GetThreads();
		m_Table.data.n = count;
		m_TermThreads.reserve(32);
		m_NewThreads.reserve(32);
		return;
	}

	auto tick = ::GetTickCount64();
	count = (int)m_Table.data.info.size();
	for (int i = 0; i < count; i++) {
		auto& t = m_Table.data.info[i];
		auto& tx = GetThreadInfoEx(t.get());
		if (tx.IsTerminated && tick > tx.TargetTime) {
			m_ThreadsEx.erase(t.get());
			m_Table.data.info.erase(m_Table.data.info.begin() + i);
			i--;
			count--;
			continue;
		}
		if (tx.IsNew && tick > tx.TargetTime) {
			tx.IsNew = false;
		}
	}

	count = (int)m_TermThreads.size();
	tick = ::GetTickCount64();

	for (int i = 0; i < count; i++) {
		auto& t = m_TermThreads[i];
		auto& tx = GetThreadInfoEx(t.get());
		if (tick > tx.TargetTime) {
			tx.IsTerminated = true;
			m_TermThreads.erase(m_TermThreads.begin() + i);
			i--;
			count--;
		}
	}

	count = (int)m_NewThreads.size();
	for (int i = 0; i < count; i++) {
		auto& t = m_NewThreads[i];
		auto& tx = GetThreadInfoEx(t.get());
		if (tick > tx.TargetTime) {
			tx.IsNew = false;
			m_NewThreads.erase(m_NewThreads.begin() + i);
			i--;
			count--;
		}
	}

	for (auto& t : m_ProcMgr.GetTerminatedThreads()) {
		auto& tx = GetThreadInfoEx(t.get());
		t->ThreadState = WinSys::ThreadState::Terminated;
		tx.TargetTime = ::GetTickCount64() + 1000;
		tx.IsTerminating = true;
		m_TermThreads.push_back(t);
	}

	for (auto& t : m_ProcMgr.GetNewThreads()) {
		auto& tx = GetThreadInfoEx(t.get());
		if (tx.IsTerminated || tx.IsTerminating)
			continue;
		tx.IsNew = true;
		tx.TargetTime = ::GetTickCount64();
		m_NewThreads.push_back(t);
		m_Table.data.info.push_back(t);
	}

	count = (int)m_Table.data.info.size();
	m_Table.data.n = count;
}

ThreadInfoEx& CProcessThreadTable::GetThreadInfoEx(WinSys::ThreadInfo* ti) const {
	auto it = m_ThreadsEx.find(ti);
	if (it != m_ThreadsEx.end())
		return it->second;

	ThreadInfoEx tx(ti);
	auto pair = m_ThreadsEx.insert({ ti,std::move(tx) });
	return pair.first->second;
}

PCWSTR CProcessThreadTable::WaitReasonToString(WinSys::WaitReason reason) {
	static PCWSTR reasons[] = {
		L"Executive",
		L"FreePage",
		L"PageIn",
		L"PoolAllocation",
		L"DelayExecution",
		L"Suspended",
		L"UserRequest",
		L"WrExecutive",
		L"WrFreePage",
		L"WrPageIn",
		L"WrPoolAllocation",
		L"WrDelayExecution",
		L"WrSuspended",
		L"WrUserRequest",
		L"WrEventPair",
		L"WrQueue",
		L"WrLpcReceive",
		L"WrLpcReply",
		L"WrVirtualMemory",
		L"WrPageOut",
		L"WrRendezvous",
		L"WrKeyedEvent",
		L"WrTerminated",
		L"WrProcessInSwap",
		L"WrCpuRateControl",
		L"WrCalloutStack",
		L"WrKernel",
		L"WrResource",
		L"WrPushLock",
		L"WrMutex",
		L"WrQuantumEnd",
		L"WrDispatchInt",
		L"WrPreempted",
		L"WrYieldExecution",
		L"WrFastMutex",
		L"WrGuardedMutex",
		L"WrRundown",
		L"WrAlertByThreadId",
		L"WrDeferredPreempt"
	};

	return reasons[(int)reason];
}

PCWSTR CProcessThreadTable::ThreadStateToString(WinSys::ThreadState state) {
	static PCWSTR states[] = {
		L"Initialized (0)",
		L"Ready (1)",
		L"Running (2)",
		L"Standby (3)",
		L"Terminated (4)",
		L"Waiting (5)",
		L"Transition (6)",
		L"DeferredReady (7)",
		L"GateWaitObsolete (8)",
		L"WaitingForProcessInSwap (9)"
	};
	ATLASSERT(state >= WinSys::ThreadState::Initialized && state <= WinSys::ThreadState::WaitingForProcessInSwap);

	return states[(int)state];
}