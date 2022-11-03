#include "stdafx.h"
#include "ProcessATHookTable.h"
#include <DriverHelper.h>
#include "Helpers.h"
#include "SymbolManager.h"
#include "SortHelper.h"

CProcessATHookTable::CProcessATHookTable(BarInfo& bars, TableInfo& table, DWORD pid, bool x64)
	:CTable(bars, table),m_ModuleTracker(pid),m_Pid(pid){
	SetTableWindowInfo(bars.nbar);
}

CString CProcessATHookTable::TypeToString(ATHookType type) {
	switch (type)
	{
		case ATHookType::IAT:
			return L"IAT";
		case ATHookType::EAT:
			return L"EAT";
		default:
			return L"Unknown Type";
	}
}

int CProcessATHookTable::ParseTableEntry(CString& s, char& mask, int& select, EATHookInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
		case Column::HookObject:
			s = info.Name.c_str();
			break;

		case Column::HookType:
			s = TypeToString(info.Type);
			break;

		case Column::Address:
		{
			auto& symbols = SymbolManager::Get();
			DWORD64 offset = 0;
			auto symbol = symbols.GetSymbolFromAddress(m_Pid, info.Address, &offset);
			CStringA text;
			if (symbol) {
				auto sym = symbol->GetSymbolInfo();
				if (offset != 0) {
					text.Format("%s!%s+0x%X", symbol->ModuleInfo.ModuleName, sym->Name, (DWORD)offset);
				}
				else
					text.Format("%s!%s", symbol->ModuleInfo.ModuleName, sym->Name);
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
				if (offset != 0) {
					text.Format("%s!%s+0x%X", symbol->ModuleInfo.ModuleName, sym->Name, (DWORD)offset);
				}
				else
					text.Format("%s!%s", symbol->ModuleInfo.ModuleName, sym->Name);
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

bool CProcessATHookTable::CompareItems(const EATHookInfo& s1, const EATHookInfo& s2, int col, bool asc) {
	switch (static_cast<Column>(col))
	{
		case Column::HookType:
			return SortHelper::SortStrings(TypeToString(s1.Type), TypeToString(s2.Type), asc);
		default:
			break;
	}
	return false;
}

LRESULT CProcessATHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	m_hProcess = DriverHelper::OpenProcess(m_Pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	Refresh();
	return 0;
}

LRESULT CProcessATHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {
	if (m_hProcess != INVALID_HANDLE_VALUE)
		::CloseHandle(m_hProcess);
	return 0;
}

LRESULT CProcessATHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessATHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessATHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessATHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessATHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessATHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessATHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CProcessATHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {

	return 0;
}

LRESULT CProcessATHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessATHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

std::shared_ptr<WinSys::ModuleInfo> CProcessATHookTable::GetModuleByAddress(ULONG_PTR address) {
	for (auto m : m_Modules) {
		ULONG_PTR base = (ULONG_PTR)m->Base;
		if (address >= base && address < base + m->ModuleSize)
			return m;
	}
	return nullptr;
}

void CProcessATHookTable::CheckEATHook(const std::shared_ptr<WinSys::ModuleInfo>& m) {
	PEParser parser(m->Path.c_str());
	if (!parser.HasExports())
		return;
	std::vector<ExportedSymbol> exports = parser.GetExports();
	void* eat = (byte*)m->Base + parser.GetEAT();
	SIZE_T size = 0;
	size = exports.size() * sizeof(DWORD);
	void* buffer = malloc(size);
	if (!buffer)
		return;
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
				info.Type = ATHookType::EAT;
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

std::shared_ptr<WinSys::ModuleInfo> CProcessATHookTable::GetModuleByName(std::wstring name) {
	for (auto m : m_Modules) {
		if (m->Name == name)
			return m;
	}
	return nullptr;
}

std::string CProcessATHookTable::GetForwardName(std::wstring libName, std::string name, bool isPe64) {
	for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		if (lib.Name == libName) {
			for (const auto& symbol : lib.Symbols) {
				if (symbol.Name == name) {
					return symbol.ForwardName;
				}
			}
		}
	}

	for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		for (const auto& symbol : lib.Symbols) {
			if (symbol.Name == name) {
				return symbol.ForwardName;
			}
		}
	}
	return "";
}

ULONG_PTR CProcessATHookTable::GetExportedProcAddr(std::wstring libName,std::string name,bool isPe64) {
	for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		if (lib.Name == libName) {
			for (const auto& symbol : lib.Symbols) {
				if (symbol.Name == name) {
					return (ULONG_PTR)lib.Base + symbol.Address;
				}
			}
		}
	}

	for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		for (const auto& symbol : lib.Symbols) {
			if (symbol.Name == name) {
				return (ULONG_PTR)lib.Base + symbol.Address;
			}
		}
	}
	return 0;
}

void CProcessATHookTable::CheckIATHook(const std::shared_ptr<WinSys::ModuleInfo>& m) {
	PEParser parser(m->Path.c_str());
	if (!parser.HasImports())
		return;
	std::vector<ImportedLibrary> libs = parser.GetImports();
	for (const auto& lib : libs) {
		void* iat = (byte*)m->Base + lib.IAT;
		SIZE_T size = 0;
		bool isPe64 = parser.IsPe64();
		if (isPe64)
			size = lib.Symbols.size() * sizeof(ULONG_PTR);
		else
			size = lib.Symbols.size() * sizeof(DWORD);
		void* buffer = malloc(size);
		if (!buffer)
			continue;
		bool ok = ReadProcessMemory(m_hProcess, iat, buffer, size, &size);
		if (ok) {
			int index = 0;
			for (auto& item : lib.Symbols) {
				DWORD64 address = 0;
				int inc = 0;
				if (isPe64) {
					address = *((PULONG_PTR)buffer + index);
					inc = 8;
				}
				else {
					address = *((PULONG)buffer + index);
					inc = 4;
				}
				
				auto& symbols = SymbolManager::Get();
				DWORD64 offset = 0;
				auto symbol = symbols.GetSymbolFromAddress(m_Pid, address, &offset);
				if (symbol) {
					auto sym = symbol->GetSymbolInfo();
					std::string symName(sym->Name);
					std::wstring wlibName = Helpers::StringToWstring(lib.Name);
					ULONG_PTR orgAddress = GetExportedProcAddr(wlibName, item.Name, isPe64);
					if (orgAddress == address) {
						index++;
						continue;
					}

					if (symName != item.Name) {
						std::string forwardName = GetForwardName(wlibName, item.Name, isPe64);
						std::size_t pos = forwardName.find(symName);
						if (pos != std::string::npos) {
							index++;
							continue;
						}

						EATHookInfo info;
						info.TargetAddress = address;
						info.Address = orgAddress;
						info.Name = m->Name + L"_";
						CString text;
						ULONG_PTR iatAddress = (ULONG_PTR)iat + index * inc;
						
						wlibName += L"_";
						text.Format(L"0x%p", iatAddress);
						std::wstring waddr = text.GetString();
						wlibName += waddr;
						info.Name += wlibName;
						info.Type = ATHookType::IAT;
						auto m = GetModuleByAddress(info.TargetAddress);
						if (m != nullptr) {
							info.TargetModule = m->Path;
						}
						m_Table.data.info.push_back(info);
					}
				}
				index++;
			}
		}
		free(buffer);
		buffer = nullptr;
	}
}

void CProcessATHookTable::Refresh() {
	m_Table.data.info.clear();
	m_ModuleTracker.EnumModules();
	m_Modules = m_ModuleTracker.GetModules();
	_libraries.clear();
	for (const auto& m : m_Modules) {
		if (m->Path.length() == 0)
			continue;
		std::size_t pos = m->Path.find(L".fon");
		if (pos != std::wstring::npos) {
			continue;
		}
		PEParser parser(m->Path.c_str());
		if (!parser.IsValid())
			continue;
		Library lib;
		lib.Name = m->Name;
		lib.Base = m->Base;
		lib.Symbols = parser.GetExports();
		lib.isPe64 = parser.IsPe64();
		_libraries.push_back(std::move(lib));
	}
	for (const auto& m : m_Modules) {
		std::size_t pos = m->Path.find(L".fon");
		if (pos != std::wstring::npos) {
			continue;
		}
		CheckEATHook(m);
		CheckIATHook(m);
	}

	m_Table.data.n = m_Table.data.info.size();
}










