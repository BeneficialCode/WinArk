#include "stdafx.h"
#include "DispatchRoutinesTable.h"
#include <DriverHelper.h>
#include <Helpers.h>

const char* MajorCodeName[] = {
	"IRP_MJ_CREATE",
	"IRP_MJ_CREATE_NAMED_PIPE",
	"IRP_MJ_CLOSE",
	"IRP_MJ_READ",
	"IRP_MJ_WRITE",
	"IRP_MJ_QUERY_INFORMATION",
	"IRP_MJ_SET_INFORMATION",
	"IRP_MJ_QUERY_EA",
	"IRP_MJ_SET_EA",
	"IRP_MJ_FLUSH_BUFFERS",
	"IRP_MJ_QUERY_VOLUME_INFORMATION",
	"IRP_MJ_SET_VOLUME_INFORMATION",
	"IRP_MJ_DIRECTORY_CONTROL",
	"IRP_MJ_FILE_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CONTROL",
	"IRP_MJ_INTERNAL_DEVICE_CONTROL",
	"IRP_MJ_SHUTDOWN",
	"IRP_MJ_LOCK_CONTROL",
	"IRP_MJ_CLEANUP",
	"IRP_MJ_CREATE_MAILSLOT",
	"IRP_MJ_QUERY_SECURITY",
	"IRP_MJ_SET_SECURITY",
	"IRP_MJ_POWER",
	"IRP_MJ_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CHANGE",
	"IRP_MJ_QUERY_QUOTA",
	"IRP_MJ_SET_QUOTA",
	"IRP_MJ_PNP",
};



LRESULT CDispatchRoutinesTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CDispatchRoutinesTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CDispatchRoutinesTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDispatchRoutinesTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CDispatchRoutinesTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDispatchRoutinesTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CDispatchRoutinesTable::CDispatchRoutinesTable(BarInfo& bars, TableInfo& table,std::wstring name)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	// \driver \filesystem
	GetDispatchInfo(name);
}

int CDispatchRoutinesTable::ParseTableEntry(CString& s, char& mask, int& select, DispatchRoutineInfo& info, int column) {
	// Code,MajorCodeName,Address,TargetModule
	switch (column)
	{
		case 0:
			s.Format(L"%d <0x%02x>", info.Code, info.Code);
			break;

		case 1:
			s = info.MajorCodeName.c_str();
			break;

		case 2:
			s.Format(L"%p", info.Routine);
			break;

		case 3:
			s = Helpers::StringToWstring(info.TargetModule).c_str();
			break;
		default:
			break;
	}
	return s.GetLength();
}

bool CDispatchRoutinesTable::CompareItems(const DispatchRoutineInfo& s1, const DispatchRoutineInfo& s2, int col, bool asc) {
	return false;
}

void CDispatchRoutinesTable::GetDispatchInfo(std::wstring name) {
	void* routines[IRP_MJ_MAXIMUM_FUNCTION]{ nullptr };
	std::wstring path = Helpers::GetDriverDirFromObjectManager(name);
	path += L"\\" + name;
	DriverHelper::GetDriverObjectRoutines(path.c_str(), &routines);
	m_Table.data.info.clear();
	m_Table.data.n = 0;
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DispatchRoutineInfo info;
		info.Code = i;
		info.MajorCodeName = MajorCodeName[i];
		info.Routine = routines[i];
		info.TargetModule = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
		m_Table.data.info.push_back(info);
	}
	m_Table.data.n = m_Table.data.info.size();
}

