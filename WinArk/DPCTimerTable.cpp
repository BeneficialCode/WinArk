#include "stdafx.h"
#include "DpcTimerTable.h"
#include "SymbolHelper.h"
#include "DriverHelper.h"
#include "FileVersionInfoHelper.h"
#include "Helpers.h"

LRESULT CDpcTimerTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Refresh();
	return 0;
}

LRESULT CDpcTimerTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CDpcTimerTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CDpcTimerTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDpcTimerTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDpcTimerTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDpcTimerTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDpcTimerTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CDpcTimerTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CDpcTimerTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(4);
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
LRESULT CDpcTimerTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CDpcTimerTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CDpcTimerTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}


CDpcTimerTable::CDpcTimerTable(BarInfo& bars, TableInfo& table)
	: CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	
}

int CDpcTimerTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<DpcTimerInfo>& info, int column) {
	// Timer,DPC,Routine,DueTime,Period,CompanyName,DriverPath
	switch (static_cast<Column>(column)) {
		case Column::CompanyName:
		{
			std::string path = Helpers::GetKernelModuleByAddress((ULONG_PTR)info->Routine);
			s = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path)).c_str();
			break;
		}

		case Column::DPC:
			s.Format(L"0x%p", info->KDpc);
			break;

		case Column::Period:
			s.Format(L"%u", info->Period);
			break;

		case Column::Timer:
			s.Format(L"0x%p", info->KTimer);

			break;

		case Column::DriverPath:
			s = Helpers::GetKernelModuleByAddress((ULONG_PTR)info->Routine).c_str();
			break;

		case Column::Routine:
			s.Format(L"0x%p", info->Routine);
			break;

		case Column::DueTime:
			s.Format(L"0x%x", info->DueTime);
			break;
	}
	return s.GetLength();
}


void CDpcTimerTable::Refresh() {
	m_Table.data.info.clear();
	ULONG memberSize = SymbolHelper::GetKernelStructMemberSize("_KTIMER_TABLE", "TimerEntries");
	ULONG size = SymbolHelper::GetKernelStructSize("_KTIMER_TABLE_ENTRY");
	KernelTimerData data;
	if (size != 0) {
		
		ULONG entryCount = memberSize / size;
		data.entriesOffset = SymbolHelper::GetKernelStructMemberOffset("_KTIMER_TABLE", "TimerEntries");
		data.maxEntryCount = entryCount;
		data.tableOffset = SymbolHelper::GetKernelStructMemberOffset("_KPRCB", "TimerTable");
		data.pKiProcessorBlock = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiProcessorBlock");
#ifdef _WIN64
		data.pKiWaitAlways = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiWaitAlways");
		data.pKiWaitNever = (void*)SymbolHelper::GetKernelSymbolAddressFromName("KiWaitNever");
#endif // _WIN64
		ULONG count = DriverHelper::GetKernelTimerCount(&data);
		if (count != 0) {
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			int cpuCount = info.dwNumberOfProcessors;
			SIZE_T size = cpuCount * entryCount * sizeof(DpcTimerInfo);
			ULONG maxCount = cpuCount * entryCount;
			wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
			DpcTimerInfo* p = (DpcTimerInfo*)buffer.get();
			if (p != nullptr) {
				memset(p, 0, size);
				DriverHelper::EnumKernelTimer(&data, p, size);
				for (ULONG i = 0; i < count; i++) {
					std::shared_ptr<DpcTimerInfo> info = std::make_shared<DpcTimerInfo>();
					info->DueTime = p[i].DueTime;
					info->KDpc = p[i].KDpc;
					info->KTimer = p[i].KTimer;
					info->Routine = p[i].Routine;
					info->Period = p[i].Period;
					m_Table.data.info.push_back(std::move(info));
				}
			}
		}
	}
	
	auto count = static_cast<int>(m_Table.data.info.size());
	m_Table.data.n = count;

	return;
}

LRESULT CDpcTimerTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

bool CDpcTimerTable::CompareItems(const std::shared_ptr<DpcTimerInfo>& p1, const std::shared_ptr<DpcTimerInfo>& p2, int col, bool asc) {
	switch (static_cast<Column>(col)) {
		case Column::CompanyName:
		{
			std::string path = Helpers::GetKernelModuleByAddress((ULONG_PTR)p1->Routine);
			std::wstring name1 = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path));
			path = Helpers::GetKernelModuleByAddress((ULONG_PTR)p2->Routine);
			std::wstring name2 = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path));
			return SortHelper::SortStrings(name1, name2, asc);
		}
	}
	return false;
}