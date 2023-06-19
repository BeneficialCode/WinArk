#include "stdafx.h"
#include "ProcessATHookTable.h"
#include <DriverHelper.h>
#include "Helpers.h"
#include "SymbolManager.h"
#include "SortHelper.h"

#pragma comment(lib,"onecore.lib")

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
	m_ApiSets.Build(m_hProcess);
	m_Entries = m_ApiSets.GetApiSets();
	m_ApiSets.SearchFiles();
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
		int idx = 0;
		for (auto& item : exports) {
			DWORD address = *pAddr;
			if (address != item.Address) {
				EATHookInfo info;
				info.TargetAddress = (ULONG_PTR)m->Base + address;
				info.Address = item.Address + (ULONG_PTR)m->Base;
				CString addr;
				addr.Format(L"0x%p", (ULONG_PTR)eat + idx * 4);
				info.Name = m->Name;
				info.Name += L"_" + addr;
				info.Type = ATHookType::EAT;
				auto m = GetModuleByAddress(info.TargetAddress);
				if (m != nullptr) {
					info.TargetModule = m->Path;
				}
				m_Table.data.info.push_back(info);
			}
			idx++;
			pAddr++;
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
	std::string forwardName;
	/*for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		if (lib.Name == libName) {
			for (const auto& symbol : lib.Symbols) {
				if (symbol.Name == name) {
					forwardName = symbol.ForwardName;
					return forwardName;
				}
			}
		}
	}*/

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
	return forwardName;
}

std::vector<ULONG_PTR> CProcessATHookTable::GetExportedProcAddr(std::wstring libName,std::string name,bool isPe64) {
	std::vector<ULONG_PTR> addresses;
	for (const auto& lib : _libraries) {
		if (lib.isPe64 != isPe64) {
			continue;
		}
		if (lib.Name == libName) {
			for (const auto& symbol : lib.Symbols) {
				if (symbol.Name == name) {
					ULONG_PTR address = (ULONG_PTR)lib.Base + symbol.Address;
					addresses.push_back(address);
					return addresses;
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
				ULONG_PTR address = (ULONG_PTR)lib.Base + symbol.Address;
				addresses.push_back(address);
			}
		}
	}
	return addresses;
}

std::vector<std::wstring> CProcessATHookTable::GetApiSetHostName(std::wstring apiset) {
	static std::vector<std::wstring> empty;
	std::wstring libName = apiset.substr(0, apiset.rfind(L"."));
	for (const auto& entry : m_Entries) {
		if (_wcsicmp(libName.c_str(), entry.Name.c_str()) == 0)
			return entry.Values;
	}
	return empty;
}

void CProcessATHookTable::CheckIATHook(const std::shared_ptr<WinSys::ModuleInfo>& m) {
	PEParser parser(m->Path.c_str());
	if (!parser.IsValid())
		return;
	if (!parser.HasImports())
		return;

	bool isSystemFile = parser.IsSystemFile();
	if (isSystemFile)
		return;

	SubsystemType type  = parser.GetSubsystemType();
	if (type == SubsystemType::Native) {
		return;
	}
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
					std::vector<std::wstring> hosts = GetApiSetHostName(wlibName);
					std::vector<ULONG_PTR> orgAddresses;
					ULONG_PTR orgAddress = 0;
					if (hosts.size() > 0) {
						for (const auto& host : hosts) {
							orgAddresses = GetExportedProcAddr(host, item.Name, isPe64);
							if (orgAddresses.size() > 0 ) {
								break;
							}
						}
					}
					else
						orgAddresses = GetExportedProcAddr(wlibName, item.Name, isPe64);

					auto it = wlibName.find(L"api-ms-win-core");
					if (it != std::wstring::npos && hosts.size()<=0) {
						index++;
						continue;
					}

					bool isSame = false;
					
					for (auto orgAddress : orgAddresses) {
						if (orgAddress == address) {
							isSame = true;
							break;
						}
					}

					if (isSame) {
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

						

						std::string moduleName = symbol->ModuleInfo.ModuleName;
						moduleName+= ".dll";
						if (lib.Name == moduleName) {
							index++;
							continue;
						}

						bool hooked = true;
						// KernelBase.dll SetLastError NTDLL.RtlSetLastWin32Error
						int idx = 0;
						int j = 0;

						std::string orgLib;
						std::string orgFuncName;
						for (auto orgAddress : orgAddresses) {
							auto orgSymbol = symbols.GetSymbolFromAddress(m_Pid, orgAddress, &offset);
							auto orgSym = orgSymbol->GetSymbolInfo();
							std::string orgSymName(orgSym->Name);
							std::string host = orgSymbol->ModuleInfo.ModuleName;
							host += ".dll";
							if (it != std::wstring::npos && hosts.size() > 0) {
								for (const auto& lib : hosts) {
									std::wstring whost = Helpers::StringToWstring(host);
									if (_wcsicmp(whost.c_str(),lib.c_str()) == 0) {
										idx = j;
										orgLib = orgSymbol->ModuleInfo.ImageName;
										orgFuncName = orgSymName;
										break;
									}
								}
							}
							else if(_stricmp(host.c_str(), lib.Name.c_str()) == 0) {
								idx = j;
								orgLib = orgSymbol->ModuleInfo.ImageName;
								orgFuncName = orgSymName;
							}
							j++;
							std::wstring whost = Helpers::StringToWstring(host);
							std::string orgForwardName = GetForwardName(whost, orgSymName, isPe64);
							pos = orgForwardName.find(symName);
							if (pos != std::string::npos) {
								hooked = false;
								break;
							}
						}

						if (!hooked) {
							index++;
							continue;
						}

						bool isForward = false;
						std::wstring orgLibName = Helpers::StringToWstring(orgLib);
						PEParser libParser(orgLibName.c_str());
						std::vector<ExportedSymbol> exports = libParser.GetExports();
						for (const auto& symbol : exports) {
							if (symbol.ForwardName.length() > 0) {
								if (symbol.Name == orgFuncName) {
									pos = symbol.ForwardName.find(symName);
									if (pos!=std::string::npos) {
										isForward = true;
									}
								}
							}
						}

						if (isForward) {
							index++;
							continue;
						}

						EATHookInfo info;
						info.TargetAddress = address;
						if (orgAddresses.size() > 0)
							info.Address = orgAddresses[idx];
						info.Name = m->Name + L"_";
						CString text;
						ULONG_PTR iatAddress = (ULONG_PTR)iat + index * inc;
						
						wlibName += L"_";
						text.Format(L"0x%p", iatAddress);
						std::wstring waddr = text.GetString();
						wlibName += waddr;
						info.Name += wlibName + L"_" + Helpers::StringToWstring(item.Name);
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










