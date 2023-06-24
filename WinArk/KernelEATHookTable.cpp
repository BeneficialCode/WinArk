#include "stdafx.h"
#include "KernelEATHookTable.h"
#include <PEParser.h>
#include <DriverHelper.h>

CKernelEATHookTable::CKernelEATHookTable(BarInfo& bars, TableInfo& table, std::shared_ptr<WinSys::KernelModuleInfo>& info)
	:CTable(bars, table),_info(info){
	SetTableWindowInfo(bars.nbar);
}


int CKernelEATHookTable::ParseTableEntry(CString& s, char& mask, int& select, KernelEATHookInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
		case Column::HookObject:
			s = info.Name.c_str();
			break;

		case Column::Address:
		{
			s.Format(L"0x%p", info.Address);
			break;
		}

		case Column::Module:
			s = info.TargetModule.c_str();
			break;

		case Column::TargetAddress:
		{
			s.Format(L"0x%p", info.TargetAddress);
			break;
		}

		default:
			break;
	}
	return s.GetLength();
}

bool CKernelEATHookTable::CompareItems(const KernelEATHookInfo& s1, const KernelEATHookInfo& s2, int col, bool asc) {
	switch (static_cast<Column>(col))
	{
	case Column::Address:
		return SortHelper::SortNumbers(s1.Address, s2.Address, asc);
		default:
			break;
	}
	return false;
}

LRESULT CKernelEATHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Refresh();
	return 0;
}

LRESULT CKernelEATHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {
	return 0;
}

LRESULT CKernelEATHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelEATHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelEATHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelEATHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelEATHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelEATHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelEATHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CKernelEATHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {

	return 0;
}

LRESULT CKernelEATHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelEATHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

void CKernelEATHookTable::CheckEATHook() {
	std::wstring path = Helpers::StringToWstring(_info->FullPath);
	PEParser parser(path.c_str());

	std::vector<ExportedSymbol> exports = parser.GetExports();
	void* eat = (byte*)_info->ImageBase + parser.GetEAT();
	ULONG size = 0;
	size = static_cast<ULONG>(exports.size() * sizeof(DWORD));
	if (size == 0)
		return;
	void* buffer = malloc(size);
	if (!buffer)
		return;

	bool ok = false;

	DumpMemData data;
	data.Address = eat;
	data.Size = size;

	ok = DriverHelper::DumpKernelMem(&data, buffer);

	if (ok) {
		PULONG pAddr = (PULONG)buffer;
		int idx = 0;
		for (auto& item : exports) {
			DWORD address = *pAddr;
			if (address != item.Address) {
				KernelEATHookInfo info;
				info.TargetAddress = (ULONG_PTR)_info->ImageBase + address;
				info.Address = item.Address + (ULONG_PTR)_info->ImageBase;
				CString addr;
				addr.Format(L"0x%p", (ULONG_PTR)eat + idx * 4);
				info.Name = Helpers::StringToWstring(item.Name);
				info.Name += L"_" + addr;
				auto m = Helpers::GetKernelModuleByAddress(info.TargetAddress);
				info.TargetModule = Helpers::StringToWstring(m);
				m_Table.data.info.push_back(info);
			}
			idx++;
			pAddr++;
		}
	}

	free(buffer);
	buffer = nullptr;
}

void CKernelEATHookTable::Refresh() {
	m_Table.data.info.clear();
	
	CheckEATHook();

	m_Table.data.n = m_Table.data.info.size();
}