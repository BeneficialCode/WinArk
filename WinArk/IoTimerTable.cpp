#include "stdafx.h"
#include "IoTimerTable.h"
#include "SymbolHelper.h"
#include "DriverHelper.h"
#include "FileVersionInfoHelper.h"
#include "Helpers.h"

LRESULT CIoTimerTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Refresh();
	return 0;
}

LRESULT CIoTimerTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CIoTimerTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CIoTimerTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CIoTimerTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CIoTimerTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CIoTimerTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CIoTimerTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CIoTimerTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CIoTimerTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_CONTEXT);
	hSubMenu = menu.GetSubMenu(5);
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
LRESULT CIoTimerTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CIoTimerTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CIoTimerTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}


CIoTimerTable::CIoTimerTable(BarInfo& bars, TableInfo& table)
	: CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);

}

int CIoTimerTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<IoTimerInfo>& info, int column) {
	// DeviceObject,Type,TimerFlag,TimerRoutine, CompanyName, DriverPath
	switch (static_cast<Column>(column)) {
		case Column::CompanyName:
		{
			std::string path = Helpers::GetKernelModuleByAddress((ULONG_PTR)info->TimerRoutine);
			s = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path)).c_str();
			break;
		}

		case Column::DeviceObject:
			s.Format(L"0x%p", info->DeviceObject);
			break;

		case Column::Type:
			s.Format(L"%u", info->Type);
			break;

		case Column::TimerRoutine:
			s.Format(L"0x%p", info->TimerRoutine);

			break;

		case Column::DriverPath:
			s = Helpers::GetKernelModuleByAddress((ULONG_PTR)info->TimerRoutine).c_str();
			break;

		case Column::TimerFlag:
			s.Format(L"%u", info->TimerFlag);
			break;
	}
	return s.GetLength();
}


void CIoTimerTable::Refresh() {
	m_Table.data.n = 0;
	m_Table.data.info.clear();
	IoTimerData data;

	
	PULONG pCount = (PULONG)SymbolHelper::GetKernelSymbolAddressFromName("IopTimerCount");
	ULONG count = DriverHelper::GetIoTimerCount(&pCount);
	if (count != 0) {
		data.pIopTimerLock = (void*)SymbolHelper::GetKernelSymbolAddressFromName("IopTimerLock");
		data.pIopTimerQueueHead = (void*)SymbolHelper::GetKernelSymbolAddressFromName("IopTimerQueueHead");
		data.pIopTimerCount = pCount;
		SIZE_T size = (count+10) * sizeof(IoTimerInfo);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
		IoTimerInfo* p = (IoTimerInfo*)buffer.get();
		if (p != nullptr) {
			memset(p, 0, size);
			DriverHelper::EnumIoTimer(&data, p, size);
			for (ULONG i = 0; i < count; i++) {
				std::shared_ptr<IoTimerInfo> info = std::make_shared<IoTimerInfo>();
				info->DeviceObject = p[i].DeviceObject;
				info->TimerFlag = p[i].TimerFlag;
				info->TimerRoutine = p[i].TimerRoutine;
				info->Type = p[i].Type;
				m_Table.data.info.push_back(std::move(info));
			}
		}
	}

	count = static_cast<int>(m_Table.data.info.size());
	m_Table.data.n = count;

	return;
}

LRESULT CIoTimerTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

bool CIoTimerTable::CompareItems(const std::shared_ptr<IoTimerInfo>& p1, const std::shared_ptr<IoTimerInfo>& p2, int col, bool asc) {
	switch (static_cast<Column>(col)) {
		case Column::CompanyName:
		{
			std::string path = Helpers::GetKernelModuleByAddress((ULONG_PTR)p1->TimerRoutine);
			std::wstring name1 = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path));
			path = Helpers::GetKernelModuleByAddress((ULONG_PTR)p2->TimerRoutine);
			std::wstring name2 = FileVersionInfoHelpers::GetCompanyName(Helpers::StringToWstring(path));
			return SortHelper::SortStrings(name1, name2, asc);
		}
	}
	return false;
}