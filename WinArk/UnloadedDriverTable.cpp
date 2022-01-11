#include "stdafx.h"
#include "UnloadedDriverTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>
#include "FormatHelper.h"

LRESULT CUnloadedDriverTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CUnloadedDriverTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CUnloadedDriverTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CUnloadedDriverTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CUnloadedDriverTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CUnloadedDriverTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CUnloadedDriverTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CUnloadedDriverTable::CUnloadedDriverTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CUnloadedDriverTable::ParseTableEntry(CString& s, char& mask, int& select, UnloadedDriverInfo& info, int column) {
	switch (column)
	{
		case 0: // Name
			s = info.DriverName.c_str();
			break;

		case 1: // StartAddress
			s.Format(L"0x%p", info.StartAddress);
			break;

		case 2: // EndAddress
			s.Format(L"0x%p", info.EndAddress);
			break;

		case 3: // CurrentTime
			s = FormatHelper::TimeToString(info.CurrentTime.QuadPart);
			break;
		default:
			break;
	}
	return s.GetLength();
}

bool CUnloadedDriverTable::CompareItems(const UnloadedDriverInfo& s1, const UnloadedDriverInfo& s2, int col, bool asc) {

	return false;
}

void CUnloadedDriverTable::Refresh() {
	void* kernelBase = Helpers::GetKernelBase();
	DWORD size = Helpers::GetKernelImageSize();
	char symPath[MAX_PATH];
	::GetCurrentDirectoryA(MAX_PATH, symPath);
	std::string pdbPath = "\\Symbols";
	pdbPath = symPath + pdbPath;

	std::string name;
	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("ntk") != std::string::npos) {
			name = filename;
			break;
		}
	}
	std::string pdbFile = pdbPath + "\\" + name;
	SymbolHandler handler;
	handler.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)kernelBase, size);


	auto symbol = handler.GetSymbolFromName("MmLastUnloadedDriver");
	PULONG pCount = (PULONG)symbol->GetSymbolInfo()->Address;
	ULONG count = DriverHelper::GetUnloadedDriverCount(&pCount);
	UnloadedDriversInfo info;
	info.Count = count;

	symbol = handler.GetSymbolFromName("MmUnloadedDrivers");
	info.pMmUnloadedDrivers = (void*)symbol->GetSymbolInfo()->Address;

	ULONG len = DriverHelper::GetUnloadedDriverDataSize(&info);

	if (len > sizeof UnloadedDriverData) {
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		DriverHelper::EnumUnloadedDrivers(&info, buffer.get(), len);

		UnloadedDriverData* p = (UnloadedDriverData*)buffer.get();
		UnloadedDriverInfo item;
		m_Table.data.info.clear();
		m_Table.data.n = 0;
		for (;;) {
			item.CurrentTime = p->CurrentTime;
			item.DriverName = (WCHAR*)((PUCHAR)p + p->StringOffset);
			item.StartAddress = p->StartAddress;
			item.EndAddress = p->EndAddress;
			m_Table.data.info.push_back(item);
			if (p->NextEntryOffset == 0)
				break;
			p = (UnloadedDriverData*)((PUCHAR)p + p->NextEntryOffset);
			if (p == nullptr)
				break;
		}
	}
	
	m_Table.data.n = m_Table.data.info.size();
}