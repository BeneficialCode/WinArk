#include "stdafx.h"
#include "PiDDBCacheTable.h"
#include "Helpers.h"
#include "DriverHelper.h"
#include "PEParser.h"
#include "SymbolHandler.h"
#include <filesystem>
#include "RegHelpers.h"
#include "FormatHelper.h"

LRESULT CPiDDBCacheTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CPiDDBCacheTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CPiDDBCacheTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CPiDDBCacheTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CPiDDBCacheTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CPiDDBCacheTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CPiDDBCacheTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CPiDDBCacheTable::CPiDDBCacheTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CPiDDBCacheTable::ParseTableEntry(CString& s, char& mask, int& select, PiDDBCacheInfo& info, int column) {
	// Name,LoadStatus,TimeDateStamp
	switch (column)
	{
		case 0:
			s = info.DriverName.c_str();
			break;

		case 1:
			s = RegHelpers::GetErrorText(info.LoadStatus);
			break;

		case 2:
		{
			// https://devblogs.microsoft.com/oldnewthing/20180103-00/?p=97705
			time_t t = (time_t)info.TimeDateStamp;
			CString time = CTime(t).Format(L"%A, %B %d, %Y");
			CString stamp;
			stamp.Format(L"0x%X ", info.TimeDateStamp);
			s = stamp + time;
			break;
		}
		default:
			break;
	}
	return s.GetLength();
}

bool CPiDDBCacheTable::CompareItems(const PiDDBCacheInfo& s1, const PiDDBCacheInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:

		default:
			break;
	}
	return false;
}

void CPiDDBCacheTable::Refresh() {
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

	ULONG_PTR address = 0;
	address = handler.GetSymbolFromName("PiDDBCacheTable")->GetSymbolInfo()->Address;

	ULONG len = DriverHelper::GetPiDDBCacheDataSize(address);

	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	DriverHelper::EnumPiDDBCacheTable(address, buffer.get(), len);

	PiDDBCacheData* p = (PiDDBCacheData*)buffer.get();
	
	PiDDBCacheInfo info;
	m_Table.data.info.clear();
	m_Table.data.n = 0;
	for (;;) {
		info.LoadStatus = p->LoadStatus;
		info.DriverName = (WCHAR*)((PUCHAR)p + p->StringOffset);
		info.TimeDateStamp = p->TimeDateStamp;
		m_Table.data.info.push_back(info);
		if (p->NextEntryOffset == 0)
			break;
		p = (PiDDBCacheData*)((PUCHAR)p + p->NextEntryOffset);
		if (p == nullptr)
			break;
	}
	m_Table.data.n = m_Table.data.info.size();
}