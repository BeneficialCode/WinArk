#include "stdafx.h"
#include "KernelInlineHookTable.h"
#include "DriverHelper.h"
#include "SymbolHelper.h"
#include "SortHelper.h"


CKernelInlineHookTable::CKernelInlineHookTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
}

CString CKernelInlineHookTable::TypeToString(KernelHookType type) {
	switch (type)
	{
	case KernelHookType::x64HookType1:
		return L"x64HookType1";
	case KernelHookType::x64HookType2:
		return L"x64HookType2";
	case KernelHookType::x64HookType3:
		return L"x64HookType3";
	default:
		return L"Unknown Type";
	}
}

int CKernelInlineHookTable::ParseTableEntry(CString& s, char& mask, int& select, KernelInlineHookInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
	case Column::HookObject:
		s = info.ModuleName.c_str();
		break;

	case Column::HookType:
		s = TypeToString(info.Type);
		break;

	case Column::Address:
	{
		s.Format(L"0x%p  ", info.Address);
		DWORD64 offset = 0;
		auto symbol = SymbolHelper::GetSymbolFromAddress(info.Address,&offset);
		if (symbol) {
			std::string name = symbol->GetSymbolInfo()->Name;
			s += Helpers::StringToWstring(name).c_str();
			if (offset != 0) {
				CString temp;
				temp.Format(L"+ 0x%x", offset);
				s += temp;
			}
			
		}
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

bool CKernelInlineHookTable::CompareItems(const KernelInlineHookInfo& s1, const KernelInlineHookInfo& s2, int col, bool asc) {
	switch (static_cast<Column>(col))
	{
		case Column::Module:
			return SortHelper::SortStrings(s1.TargetModule, s2.TargetModule, asc);
			break;
		case Column::TargetAddress:
			return SortHelper::SortNumbers(s1.TargetAddress, s2.TargetAddress, asc);
			break;
		default:
			break;
	}
	return false;
}

// we have to specify the architectures explicitly when install capstone by vcpkg
LRESULT CKernelInlineHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Refresh();

	return 0;
}

LRESULT CKernelInlineHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {
	
	return 0;
}

LRESULT CKernelInlineHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelInlineHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CKernelInlineHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(2);
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

LRESULT CKernelInlineHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

void CKernelInlineHookTable::Refresh() {
	m_Table.data.info.clear();
	ULONG count = DriverHelper::GetKernelInlineHookCount();
	if (count == 0) {
		return;
	}
	DWORD size = (count + 10) * sizeof(KernelInlineHookData);
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size,
		MEM_COMMIT, PAGE_READWRITE));
	KernelInlineHookData* pData = (KernelInlineHookData*)buffer.get();
	bool success = DriverHelper::DetectInlineHook(count,pData, size);
	if (success) {
		KernelInlineHookInfo info;
		for (ULONG i = 0; i < count; i++) {
			info.Address = pData[i].Address;
			std::string name = Helpers::GetKernelModuleNameByAddress(
				pData[i].Address);
			info.ModuleName = Helpers::StringToWstring(name);
			info.TargetAddress = pData[i].TargetAddress;
			info.Type = pData[i].Type;
			std::string path = Helpers::GetKernelModuleByAddress(info.TargetAddress);
			info.TargetModule = Helpers::StringToWstring(path);
			m_Table.data.info.push_back(info);
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

LRESULT CKernelInlineHookTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	Invalidate(True);
	return TRUE;
}












