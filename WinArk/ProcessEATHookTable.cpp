#include "stdafx.h"
#include "ProcessEATHookTable.h"
#include <PEParser.h>
#include <DriverHelper.h>
#include "Helpers.h"
#include "SymbolManager.h"

CProcessEATHookTable::CProcessEATHookTable(BarInfo& bars, TableInfo& table, DWORD pid, bool x64)
	:CTable(bars, table),m_ModuleTracker(pid),m_Pid(pid){
	SetTableWindowInfo(bars.nbar);
}

int CProcessEATHookTable::ParseTableEntry(CString& s, char& mask, int& select, EATHookInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
		case Column::HookObject:
			s = info.Name.c_str();
			break;

		case Column::Address:
		{
			auto& symbols = SymbolManager::Get();
			DWORD64 offset = 0;
			auto symbol = symbols.GetSymbolFromAddress(m_Pid, info.Address, &offset);
			CStringA text;
			if (symbol) {
				auto sym = symbol->GetSymbolInfo();
				text.Format("%s!%s+0x%X", symbol->ModuleInfo.ModuleName, sym->Name, (DWORD)offset);
				std::string details = text.GetString();
				std::wstring wdetails = Helpers::StringToWstring(details);
				s.Format(L"0x%p (%s)", info.Address, wdetails.c_str());
			}
			else
				s.Format(L"0x%p", info.Address);
			break;
		}

		case Column::Module:
			s = info.TargetModule.c_str();
			break;

		case Column::TargetAddress:
		{
			auto& symbols = SymbolManager::Get();
			DWORD64 offset = 0;
			auto symbol = symbols.GetSymbolFromAddress(m_Pid, info.TargetAddress, &offset);
			CStringA text;
			if (symbol) {
				auto sym = symbol->GetSymbolInfo();
				text.Format("%s!%s+0x%X", symbol->ModuleInfo.ModuleName, sym->Name, (DWORD)offset);
				std::string details = text.GetString();
				std::wstring wdetails = Helpers::StringToWstring(details);
				s.Format(L"0x%p (%s)", info.TargetAddress, wdetails.c_str());
			}
			else
				s.Format(L"0x%p", info.TargetAddress);
			break;
		}

		default:
			break;
	}
	return s.GetLength();
}

bool CProcessEATHookTable::CompareItems(const EATHookInfo& s1, const EATHookInfo& s2, int col, bool asc) {
	return false;
}

LRESULT CProcessEATHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	m_hProcess = DriverHelper::OpenProcess(m_Pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	Refresh();
	return 0;
}

LRESULT CProcessEATHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {
	if (m_hProcess != INVALID_HANDLE_VALUE)
		::CloseHandle(m_hProcess);
	return 0;
}

LRESULT CProcessEATHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessEATHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessEATHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessEATHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessEATHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessEATHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessEATHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CProcessEATHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {

	return 0;
}

LRESULT CProcessEATHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessEATHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

std::shared_ptr<WinSys::ModuleInfo> CProcessEATHookTable::GetModuleByAddress(ULONG_PTR address) {
	for (auto m : m_Modules) {
		ULONG_PTR base = (ULONG_PTR)m->Base;
		if (address >= base && address < base + m->ModuleSize)
			return m;
	}
	return nullptr;
}

void CProcessEATHookTable::Refresh() {
	m_Table.data.info.clear();
	m_ModuleTracker.EnumModules();
	m_Modules = m_ModuleTracker.GetModules();
	for (const auto& m : m_Modules) {
		PEParser parser(m->Path.c_str());
		if (!parser.HasExports())
			continue;
		std::vector<ExportedSymbol> exports = parser.GetExports();
		void* eat = (byte*)m->Base + parser.GetEAT();
		SIZE_T size = 0;
		size = exports.size() * sizeof(DWORD);
		void* buffer = malloc(size);
		if (!buffer)
			continue;
		bool ok = ReadProcessMemory(m_hProcess, eat, buffer, size, &size);
		if (ok) {
			PULONG pAddr = (PULONG)buffer;
			for (auto& item : exports) {
				DWORD address = *pAddr;
				pAddr++;
				if (address != item.Address) {
					EATHookInfo info;
					info.TargetAddress = (ULONG_PTR)m->Base + address;
					info.Address = item.Address + (ULONG_PTR)m->Base;
					info.Name = m->Name;
					auto m = GetModuleByAddress(info.TargetAddress);
					if (m != nullptr) {
						info.TargetModule = m->Path;
					}
					m_Table.data.info.push_back(info);
				}
			}
		}
		free(buffer);
		buffer = nullptr;
	}

	m_Table.data.n = m_Table.data.info.size();
}










