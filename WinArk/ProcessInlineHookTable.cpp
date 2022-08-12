#include "stdafx.h"
#include "ProcessInlineHookTable.h"
#include "DriverHelper.h"
#include "SymbolManager.h"
#include "Helpers.h"
#include <Processes.h>
#include "ClipboardHelper.h"

#ifdef _WIN64
#pragma comment(lib,"x64/capstone.lib")
#else
#pragma comment(lib,"x86/capstone.lib")
#endif // _WIN64



CProcessInlineHookTable::CProcessInlineHookTable(BarInfo& bars, TableInfo& table, DWORD pid, bool x64)
	:CTable(bars, table), m_Pid(pid), m_ModuleTracker(pid), _x64(x64) {
	SetTableWindowInfo(bars.nbar);
}

CString CProcessInlineHookTable::TypeToString(HookType type) {
	switch (type)
	{
		case HookType::x64HookType1:
			return L"x64HookType1";
		case HookType::x64HookType2:
			return L"x64HookType2";
		case HookType::x64HookType3:
			return L"x64HookType3";
		case HookType::x64HookType4:
			return L"x64HookType4";
		case HookType::x86HookType1:
			return L"x86HookType1";
		case HookType::x86HookType2:
			return L"x86HookType2";
		case HookType::x86HookType3:
			return L"x86HookType3";
		case HookType::x86HookType6:
			return L"x86HookType6";
		default:
			return L"Unknown Type";
	}
}

int CProcessInlineHookTable::ParseTableEntry(CString& s, char& mask, int& select, InlineHookInfo& info, int column) {
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
			s.Format(L"0x%p", info.TargetAddress);
			break;

		default:
			break;
	}
	return s.GetLength();
}

bool CProcessInlineHookTable::CompareItems(const InlineHookInfo& s1, const InlineHookInfo& s2, int col, bool asc) {
	return false;
}

LRESULT CProcessInlineHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	m_hProcess = DriverHelper::OpenProcess(m_Pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
	if (m_hProcess == nullptr)
		return -1;

	m_VMTracker.reset(new WinSys::ProcessVMTracker(m_hProcess));
	if (m_VMTracker == nullptr)
		return -1;
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &_x64handle) != CS_ERR_OK)
		return -1;
	cs_option(_x64handle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(_x64handle, CS_OPT_UNSIGNED, CS_OPT_ON);
	cs_option(_x64handle, CS_OPT_SKIPDATA, CS_OPT_ON);

	if (cs_open(CS_ARCH_X86, CS_MODE_32, &_x86handle) != CS_ERR_OK)
		return -1;

	cs_option(_x86handle, CS_OPT_DETAIL, CS_OPT_ON);
	cs_option(_x86handle, CS_OPT_UNSIGNED, CS_OPT_ON);
	cs_option(_x86handle, CS_OPT_SKIPDATA, CS_OPT_ON);

	Refresh();

	return 0;
}

LRESULT CProcessInlineHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {
	if (m_hProcess != INVALID_HANDLE_VALUE)
		::CloseHandle(m_hProcess);
	return 0;
}

LRESULT CProcessInlineHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessInlineHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessInlineHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CProcessInlineHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_PROC_CONTEXT);
	hSubMenu = menu.GetSubMenu(1);
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

LRESULT CProcessInlineHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessInlineHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

const std::vector<std::shared_ptr<WinSys::MemoryRegionItem>>& CProcessInlineHookTable::GetItems() {
	std::vector<std::shared_ptr<WinSys::MemoryRegionItem>> regions = m_VMTracker->GetRegions();
	DWORD value;
	for (auto region : regions) {
		value = region->Protect & 0xFF;
		if (value & PAGE_EXECUTE || value & PAGE_EXECUTE_READ ||
			value & PAGE_EXECUTE_WRITECOPY || value & PAGE_EXECUTE_READWRITE) {
			m_Items.push_back(region);
		}
	}
	return m_Items;
}

bool CProcessInlineHookTable::IsInCodeBlock(ULONG_PTR address) {
	for (const auto& block : m_Items) {
		if (address >= (ULONG_PTR)block->BaseAddress && address <= (ULONG_PTR)block->BaseAddress + block->RegionSize) {
			return true;
		}
	}
	return false;
}

void CProcessInlineHookTable::CheckX86HookType1(cs_insn* insn, size_t j, size_t count, ULONG_PTR moduleBase, SIZE_T moduleSize) {
	cs_detail* d1, * d2;
	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;
	if (d1->x86.op_count != 1)
		return;

	if (d1->x86.operands[0].type != CS_OP_IMM)
		return;

	if (d1->x86.operands[0].size != 4)
		return;

	if (strcmp(insn[j].mnemonic, "jmp"))
		return;

	if (d1->x86.opcode[0] != 0xE9)
		return;

	int flag = true;

	ULONG_PTR targetAddress = d1->x86.operands[0].imm;

	if (!IsInCodeBlock(targetAddress))
		return;

	if (targetAddress >= moduleBase && targetAddress <= moduleBase + moduleSize)
		return;

	flag = false;

	for (const auto& m : m_Sys32Modules) {
		//printf("path: %ws\n", m->Path.c_str());
		if (targetAddress >= (ULONG_PTR)m->Base && targetAddress <= (ULONG_PTR)m->Base + m->ModuleSize) {
			flag = true;
		}
	}
	if (flag) {
		return;
	}

	InlineHookInfo info;
	info.TargetAddress = targetAddress;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(targetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x86HookType1;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

void CProcessInlineHookTable::CheckX86HookType2(cs_insn* insn, size_t j, size_t count) {
	cs_detail* d1, * d2;

	if (strcmp(insn[j].mnemonic, "push"))
		return;


	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;
	if (j + 1 >= count) {
		return;
	}
	d2 = insn[j + 1].detail;
	if (d2 == nullptr)
		return;

	if (d1->x86.op_count != 1)
		return;
	if (d1->x86.operands[0].type != CS_OP_IMM)
		return;
	if (d1->x86.operands[0].size != 4)
		return;

	if (strcmp(insn[j + 1].mnemonic, "ret"))
		return;

	if (d2->x86.op_count != 0)
		return;

	ULONG targetAddress = d1->x86.operands[0].imm;
	size_t size;
	ULONG dummy;
	bool success = ::ReadProcessMemory(m_hProcess, (LPVOID)targetAddress, &dummy, 4, &size);
	if (!success)
		return;

	InlineHookInfo info;
	info.TargetAddress = targetAddress;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(targetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x86HookType2;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

void CProcessInlineHookTable::CheckX86HookType3(cs_insn* insn, size_t j, size_t count) {
	cs_detail* d1, * d2;

	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;
	if (j + 1 >= count) {
		return;
	}
	d2 = insn[j + 1].detail;
	if (d2 == nullptr)
		return;

	if (d1->x86.operands[0].type != CS_OP_REG)
		return;

	if (d1->x86.operands[0].access != CS_AC_WRITE)
		return;

	if (d1->x86.operands[0].size != 4)
		return;

	if (d1->x86.operands[1].type != CS_OP_IMM)
		return;

	if (d1->x86.operands[1].size != 4)
		return;

	if (d2->x86.operands[0].type != CS_OP_REG)
		return;

	if (d2->x86.operands[0].access != CS_AC_READ)
		return;

	if (d2->x86.operands[0].size != 4)
		return;

	if (strcmp(insn[j + 1].mnemonic, "jmp"))
		return;

	if (d1->x86.operands[0].reg != d2->x86.operands[0].reg)
		return;

	ULONG targetAddress = d1->x86.operands[1].imm;
	// 排除无效的内存地址
	size_t size;
	ULONG dummy;
	bool success = ::ReadProcessMemory(m_hProcess, (LPVOID)targetAddress, &dummy, 4, &size);
	if (!success)
		return;

	InlineHookInfo info;
	info.TargetAddress = targetAddress;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(targetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x86HookType3;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

void CProcessInlineHookTable::CheckX86HookType6(cs_insn* insn, size_t j, size_t count) {
	if (strcmp(insn[j].mnemonic, "jmp"))
		return;

	cs_detail* d1, * d2;

	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;
	if (j + 1 >= count) {
		return;
	}
	d2 = insn[j + 1].detail;
	if (d2 == nullptr)
		return;

	if (d2->x86.opcode[0] != 0xE8)
		return;

	if (strcmp(insn[j + 1].mnemonic, "jmp"))
		return;

	if (d1->x86.op_count != 1)
		return;
	if (d1->x86.operands[0].type != CS_OP_IMM)
		return;
	if (d1->x86.operands[0].size != 4)
		return;

	if (d2->x86.operands[0].type != CS_OP_IMM)
		return;

	if (d2->x86.operands[0].size != 4)
		return;

	ULONG_PTR targetAddress = d2->x86.operands[0].imm;
	InlineHookInfo info;
	info.TargetAddress = targetAddress;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(targetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x86HookType3;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

void CProcessInlineHookTable::CheckInlineHook(uint8_t* code, size_t codeSize, uint64_t address, ULONG_PTR moduleBase, SIZE_T moduleSize) {
	// 反汇编时间较长的情况下，会卡UI
	size_t count;
	cs_insn* insn;
	if (_x64) {
		count = cs_disasm(_x64handle, code, codeSize, address, 0, &insn);
		if (count > 0) {
			for (size_t j = 0; j < count; j++) {
				CheckX64HookType1(insn, j, count, moduleBase, moduleSize, address, codeSize);
				CheckX64HookType2(insn, j, count);
				CheckX64HookType4(insn, j, count, moduleBase, moduleSize, address, codeSize);
			}
			cs_free(insn, count);
		}
	}
	else {
		count = cs_disasm(_x86handle, code, codeSize, address, 0, &insn);
		if (count > 0) {
			for (size_t j = 0; j < count; j++) {
				CheckX86HookType1(insn, j, count, moduleBase, moduleSize);
				CheckX86HookType2(insn, j, count);
				CheckX86HookType3(insn, j, count);
				CheckX86HookType6(insn, j, count);
			}
			cs_free(insn, count);
		}
	}
}

void CProcessInlineHookTable::Refresh() {
	m_VMTracker->EnumRegions();

	m_Items = GetItems();
	m_ModuleTracker.EnumModules();
	m_Modules = m_ModuleTracker.GetModules();

	m_Sys64Modules.clear();
	m_Table.data.info.clear();
	m_Sys32Modules.clear();
	for (const auto& m : m_Modules) {
		if (m->Path.find(L"System32") != std::wstring::npos) {
			m_Sys64Modules.push_back(m);
		}
		if (m->Path.find(L"SysWOW64") != std::wstring::npos)
			m_Sys32Modules.push_back(m);
	}

	uint8_t* code = nullptr;
	SIZE_T size = 0;
	ULONG_PTR moduleBase;
	SIZE_T moduleSize;
	ULONG_PTR address;
	for (const auto& block : m_Items) {
		code = static_cast<uint8_t*>(malloc(block->RegionSize));
		if (code != nullptr) {
			memset(code, 0, block->RegionSize);
			bool success = ::ReadProcessMemory(m_hProcess, block->BaseAddress, code, block->RegionSize, &size);
			if (!success) {
				free(code);
				continue;
			}
			address = (ULONG_PTR)block->BaseAddress;
			auto m = GetModuleByAddress(address);
			if (m != nullptr) {
				moduleSize = m->ModuleSize;
				moduleBase = (ULONG_PTR)m->Base;
			}
			else {
				moduleBase = 0;
				moduleSize = 0;
			}
			CheckInlineHook(code, size, address, moduleBase, moduleSize);
			free(code);
			code = nullptr;
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

std::shared_ptr<WinSys::ModuleInfo> CProcessInlineHookTable::GetModuleByAddress(ULONG_PTR address) {
	for (auto m : m_Modules) {
		ULONG_PTR base = (ULONG_PTR)m->Base;
		if (address >= base && address < base + m->ModuleSize)
			return m;
	}
	return nullptr;
}

void CProcessInlineHookTable::CheckX64HookType1(cs_insn* insn, size_t j, size_t count,
	ULONG_PTR moduleBase, size_t moduleSize,
	ULONG_PTR base, size_t size) {
	cs_detail* d1, * d2;

	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;

	if (j + 1 >= count)
		return;
	d2 = insn[j + 1].detail;
	if (d2 == nullptr)
		return;

	if (d1->x86.op_count != 2)
		return;

	cs_x86_op* op1, * op2;
	op1 = &(d1->x86.operands[0]);
	op2 = &(d2->x86.operands[1]);

	if (op1->type != x86_op_type::X86_OP_REG)
		return;

	if (op1->access != CS_AC_WRITE)
		return;

	if (op2->type != X86_OP_IMM)
		return;

	if (op1->size != 8)
		return;

	if (op2->size != 8)
		return;

	if (strcmp(insn[j + 1].mnemonic, "jmp"))
		return;

	if (d2->x86.op_count != 1)
		return;

	if (d2->x86.operands[0].type != X86_OP_REG)
		return;

	if (d2->x86.operands[0].access != CS_AC_READ)
		return;

	ULONG_PTR targetAddress = d1->x86.operands[1].imm;

	if (targetAddress >= moduleBase && targetAddress <= moduleBase + moduleSize)
		return;

	InlineHookInfo info;
	info.TargetAddress = targetAddress;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(targetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x64HookType1;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}


void CProcessInlineHookTable::CheckX64HookType2(cs_insn* insn, size_t j, size_t count) {
	cs_detail* d1, * d2;
	d1 = insn[j].detail;
	if (j + 1 >= count) {
		return;
	}
	d2 = insn[j + 1].detail;
	if (d2 == nullptr)
		return;

	if (strcmp(insn[j].mnemonic, "push")) {
		return;
	}
	if (j + 2 >= count) {
		return;
	}

	if (strcmp(insn[j + 2].mnemonic, "ret")) {
		return;
	}

	if (d1->regs_read_count != 1)
		return;

	if (strcmp("rsp", cs_reg_name(_x64handle, d1->regs_read[0])))
		return;

	if (d1->x86.op_count != 1 || d1->x86.operands[0].size != 8)
		return;

	if (d2->x86.op_count != 2)
		return;

	if (d2->x86.operands[0].access != CS_AC_WRITE)
		return;


	if (d2->x86.operands[1].type != CS_OP_IMM)
		return;

	if (d2->x86.operands[1].size != 4 || d2->x86.operands[0].size != 4)
		return;

	if (d2->x86.operands[0].mem.base != X86_REG_RSP)
		return;

	InlineHookInfo info;
	ULONG lowAddr = d1->x86.operands[0].imm;
	ULONG highAddr = d2->x86.operands[1].imm;
	info.TargetAddress = ((ULONG_PTR)highAddr << 32) | lowAddr;
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(info.TargetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x64HookType2;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

void CProcessInlineHookTable::CheckX64HookType4(cs_insn* insn, size_t j, size_t count, ULONG_PTR moduleBase, size_t moduleSize,
	ULONG_PTR base, size_t size) {
	cs_detail* d1, * d2;
	d1 = insn[j].detail;
	if (d1 == nullptr)
		return;
	if (j + 1 >= count)
		return;

	if (d1->x86.opcode[0] == 0xEB)
		return;
	if (d1->x86.op_count != 1)
		return;
	if (d1->x86.operands[0].type != CS_OP_IMM)
		return;
	if (d1->x86.operands[0].size != 8)
		return;

	if (strcmp(insn[j].mnemonic, "jmp"))
		return;

	bool flag = true;
	ULONG_PTR targetAddress = d1->x86.operands[0].imm;
	ULONG_PTR address = base + size;
	if (targetAddress >= base && targetAddress <= base + size) {
		// 排除循环跳转
		return;
	}

	// 假设不在任何代码块
	flag = true;
	for (const auto& info : m_Items) {
		if (targetAddress >= (ULONG_PTR)info->BaseAddress && targetAddress <= (ULONG_PTR)info->BaseAddress + info->RegionSize) {
			flag = false;
		}
	}
	if (flag)
		return;

	flag = false;
	for (const auto& m : m_Sys64Modules) {
		// 排除调用API
		if (targetAddress >= (ULONG_PTR)m->Base && targetAddress <= (ULONG_PTR)m->Base + m->ModuleSize) {
			flag = true;
		}
	}
	if (flag) {
		return;
	}

	// 排除非法地址
	uint8_t code[7];
	SIZE_T dummy;
	bool success = ::ReadProcessMemory(m_hProcess, (LPVOID)targetAddress, code, 6, &dummy);
	if (!success)
		return;

	cs_insn* jmpCode;
	count = cs_disasm(_x64handle, code, sizeof(code) - 1, targetAddress, 0, &jmpCode);
	if (0 == count) {
		return;
	}
	ULONG_PTR codeAddress;
	cs_detail* d = jmpCode[0].detail;
	if (d != nullptr) {
		ULONG_PTR memAddress = targetAddress + jmpCode[0].size + d->x86.operands[0].mem.disp;
		success = ::ReadProcessMemory(m_hProcess, (LPVOID)memAddress, &codeAddress, sizeof(codeAddress), &dummy);
		if (!success)
			return;
	}

	success = ::ReadProcessMemory(m_hProcess, (LPVOID)codeAddress, &dummy, sizeof(dummy), &dummy);
	InlineHookInfo info;
	if (success) {
		info.TargetAddress = codeAddress;
	}
	else {
		info.TargetAddress = targetAddress;
	}
	info.TargetModule = L"Unknown";
	auto m = GetModuleByAddress(info.TargetAddress);
	if (m != nullptr) {
		info.TargetModule = m->Path;
	}
	info.Type = HookType::x64HookType4;
	info.Address = insn[j].address;
	m = GetModuleByAddress(info.Address);
	info.Name = L"Unknown";
	if (m != nullptr)
		info.Name = m->Name;
	m_Table.data.info.push_back(info);
}

LRESULT CProcessInlineHookTable::OnHookCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	std::wstring text = GetSingleHookInfo(info);
	ClipboardHelper::CopyText(m_hWnd, text.c_str());
	return 0;
}

LRESULT CProcessInlineHookTable::OnHookExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleHookInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CProcessInlineHookTable::GetSingleHookInfo(InlineHookInfo& info) {
	CString text;
	CString s;

	s = info.Name.c_str();
	s += L"\t";
	text += s;

	s = TypeToString(info.Type);
	s += L"\t";
	text += s;

	auto& symbols = SymbolManager::Get();
	DWORD64 offset = 0;
	auto symbol = symbols.GetSymbolFromAddress(m_Pid, info.Address, &offset);
	if (symbol) {
		CStringA m;
		auto sym = symbol->GetSymbolInfo();
		m.Format("%s!%s+0x%X", symbol->ModuleInfo.ModuleName, sym->Name, (DWORD)offset);
		std::string details = m.GetString();
		std::wstring wdetails = Helpers::StringToWstring(details);
		s.Format(L"0x%p (%s)", info.Address, wdetails.c_str());
	}
	else
		s.Format(L"0x%p", info.Address);
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.TargetAddress);
	s += L"\t";
	text += s;

	s = info.TargetModule.c_str();
	s += L"\t";
	text += s;


	

	text += L"\r\n";

	return text.GetString();
}