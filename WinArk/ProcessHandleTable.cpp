#include "stdafx.h"
#include "resource.h"
#include "ProcessHandleTable.h"
#include <algorithm>
#include <execution>
#include "SortHelper.h"
#include <string>
#include "ObjectType.h"
#include "ObjectTypeFactory.h"
#include "NtDll.h"
#include "SecurityHelper.h"
#include "SecurityInfo.h"
#include "AccessMaskDecoder.h"
#include "ProcessHelper.h"
#include "DriverHelper.h"


using namespace WinSys;

LRESULT CProcessHandleTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Refresh();
	return 0;
}

LRESULT CProcessHandleTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CProcessHandleTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessHandleTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessHandleTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessHandleTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessHandleTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessHandleTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessHandleTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CProcessHandleTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessHandleTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessHandleTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}


CProcessHandleTable::CProcessHandleTable(BarInfo& bars, TableInfo& table, PCWSTR type, DWORD pid)
	: CTable(bars, table), m_HandleType(type), m_Pid(pid) {
	SetTableWindowInfo(bars.nbar);
	m_hProcess.reset(DriverHelper::OpenProcess(pid, SYNCHRONIZE));
	if (pid) {
		auto hProcess = DriverHelper::OpenProcess(pid, SYNCHRONIZE | PROCESS_QUERY_INFORMATION);
		m_HandleTracker.reset(new ProcessHandleTracker(hProcess));
		if (!m_HandleTracker->IsValid()) {
			AtlMessageBox(nullptr, (L"Failed to open handle to process " + std::to_wstring(pid)).c_str(), IDS_TITLE, MB_ICONERROR);
			m_HandleTracker.reset();
		}
	}
}

int CProcessHandleTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::HandleInfo>& info, int column) {
	switch (column) {
	case 0: // type
		s = m_ObjMgr.GetType(info->ObjectTypeIndex)->TypeName.c_str();
		break;
	case 1: // address
		s.Format(L"0x%p", info->Object);
		break;
	case 2: // name
		if (info->HandleAttributes & 0x8000)
			s = info->Name.c_str();
		else {
			s = m_ObjMgr.GetObjectName(UlongToHandle(info->HandleValue), info->ProcessId, info->ObjectTypeIndex).c_str();
			info->Name = s;
			info->HandleAttributes |= 0x8000;
		}
		break;
	case 3: // handle
		s.Format(L"%d (0x%X)", info->HandleValue, info->HandleValue);
		break;
	case 4: // process name
		s = m_ProcMgr.GetProcessNameById(info->ProcessId).c_str();
		break;
	case 5: // PID
		s.Format(L"%d (0x%X)", info->ProcessId, info->ProcessId);
		break;
	case 6: // attributes
		s.Format(L"%s (%d)", (PCWSTR)HandleAttributesToString(info->HandleAttributes),info->HandleAttributes & 0x7fff);
		break;

	case 7: // access mask
		s.Format(L"0x%08X", info->GrantedAccess);
		break;

	case 8: // decoded access mask
		s = AccessMaskDecoder::DecodeAccessMask(m_ObjMgr.GetType(info->ObjectTypeIndex)->TypeName.c_str(), info->GrantedAccess);
		break;

	case 9: // details
		if (::GetTickCount64() > m_TargetUpdateTime || m_DetailsCache.find(info.get()) == m_DetailsCache.end()) {
			auto h = m_ObjMgr.DupHandle(ULongToHandle(info->HandleValue), info->ProcessId, info->ObjectTypeIndex);
			if (h) {
				auto type = ObjectTypeFactory::CreateObjectType(info->ObjectTypeIndex, ObjectManager::GetType(info->ObjectTypeIndex)->TypeName.c_str());
				s = type ? type->GetDetails(h) : CString();
				m_DetailsCache[info.get()] = s;
				::CloseHandle(h);
			}
			m_TargetUpdateTime = ::GetTickCount64() + 5000;
		}
		else {
			s = m_DetailsCache[info.get()];
		}
		break;
	}
	return s.GetLength();
}




void CProcessHandleTable::Refresh() {
	if (m_hProcess && ::WaitForSingleObject(m_hProcess.get(), 0) == WAIT_OBJECT_0) {
		KillTimer(1);
		AtlMessageBox(*this, (L"Process " + std::to_wstring(m_Pid) + L" is no longer running.").c_str(), IDS_TITLE, MB_OK | MB_ICONWARNING);
		return;
	}
	m_ObjMgr.EnumHandles(m_HandleType, m_Pid, m_NamedObjectsOnly);
	if (m_HandleTracker) {
		m_DetailsCache.clear();
		m_DetailsCache.reserve(1024);
		m_Changes.clear();
		m_Changes.reserve(8);
		m_HandleTracker->EnumHandles(true);
		if (!m_Paused)
			SetTimer(1, 1000, nullptr);
	}
	m_ProcMgr.EnumProcesses();
	m_Table.data.info = m_ObjMgr.GetHandles();

	auto count = static_cast<int>(m_Table.data.info.size());
	m_Table.data.n = count;

	return;
}

CString CProcessHandleTable::HandleAttributesToString(ULONG attributes) {
	CString result;
	if (attributes & 2)
		result += L", Inherit";
	if (attributes & 1)
		result += L", Protect";
	if (attributes & 4)
		result += L", Audit";

	if (result.IsEmpty())
		result = L"None";
	else
		result = result.Mid(2);
	return result;
}

bool CProcessHandleTable::CompareItems(const std::shared_ptr<WinSys::HandleInfo>& p1, const std::shared_ptr<WinSys::HandleInfo>& p2, int col, bool asc) {
	switch (col) {
		case 0:
		{
			return SortHelper::SortStrings(m_ObjMgr.GetType(p1->ObjectTypeIndex)->TypeName, m_ObjMgr.GetType(p2->ObjectTypeIndex)->TypeName, asc);
		}
		case 9: {
			return SortHelper::SortStrings(m_DetailsCache[p1.get()], m_DetailsCache[p2.get()], asc);
		}
		case 2: {
			return SortHelper::SortStrings(p1->Name, p2->Name, asc);
		}
	}
	return false;
}