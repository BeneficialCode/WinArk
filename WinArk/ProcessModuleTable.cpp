#include "stdafx.h"
#include "ProcessModuleTable.h"
#include <algorithm>
#include "SortHelper.h"
#include "resource.h"
#include <PEParser.h>

LRESULT CProcessModuleTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CProcessModuleTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CProcessModuleTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CProcessModuleTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessModuleTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessModuleTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessModuleTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessModuleTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CProcessModuleTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CProcessModuleTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CProcessModuleTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CProcessModuleTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}


CProcessModuleTable::CProcessModuleTable(BarInfo& bars, TableInfo& table, DWORD pid, HWND hwnd)
	: CTable(bars, table), m_Tracker(pid), _hDlg(hwnd) {
	SetTableWindowInfo(bars.nbar);
	m_Table.data.info.clear();
	Refresh();
}

CProcessModuleTable::CProcessModuleTable(BarInfo& bars, TableInfo& table, HANDLE hProcess)
	: CTable(bars, table), m_Tracker(hProcess) {
	SetTableWindowInfo(bars.nbar);
	m_Table.data.info.clear();
	//DoRefresh();
}

CString CProcessModuleTable::CharacteristicsToString(WinSys::DllCharacteristics ch) {
	using namespace WinSys;

	CString result;
	if ((ch & DllCharacteristics::HighEntropyVA) != DllCharacteristics::None)
		result += L"High Entropy VA, ";
	if ((ch & DllCharacteristics::DynamicBase) != DllCharacteristics::None)
		result += L"Dynamic Base, ";
	if ((ch & DllCharacteristics::ForceIntegrity) != DllCharacteristics::None)
		result += L"Force Integrity, ";
	if ((ch & DllCharacteristics::NxCompat) != DllCharacteristics::None)
		result += L"NX Compat, ";
	if ((ch & DllCharacteristics::NoIsolation) != DllCharacteristics::None)
		result += L"No Isolation, ";
	if ((ch & DllCharacteristics::NoSEH) != DllCharacteristics::None)
		result += L"No SEH, ";
	if ((ch & DllCharacteristics::NoBind) != DllCharacteristics::None)
		result += L"No Bind, ";
	if ((ch & DllCharacteristics::AppContainer) != DllCharacteristics::None)
		result += L"App Container, ";
	if ((ch & DllCharacteristics::WDMDriver) != DllCharacteristics::None)
		result += L"WDM Driver, ";
	if ((ch & DllCharacteristics::ControlFlowGuard) != DllCharacteristics::None)
		result += L"CFG, ";
	if ((ch & DllCharacteristics::TerminalServerAware) != DllCharacteristics::None)
		result += L"TS Aware, ";

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 2);
	return result;
}

int CProcessModuleTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::ModuleInfo>& info, int column) {
	switch (static_cast<ModuleColumn>(column)) {
	case ModuleColumn::ModuleName:
		s = info->Name.empty() ? L"<Pagefile Backed>" : info->Name.c_str();
		break;

	case ModuleColumn::Type:
		s = info->Type == WinSys::MapType::Image ? L"Image" : L"Data";
		break;

	case ModuleColumn::Bit:
		if (info->Type == WinSys::MapType::Image) {
			PEParser parser(info->Path.c_str());
			s = parser.IsPe64() ? L"x64" : L"x86";

			bool isExecutable = parser.IsExecutable();
			parser.GetImports();
		}
		else {
			s = L"N/A";
		}
		break;

	case ModuleColumn::ModuleSize:
		s.Format(L"0x%08X", info->ModuleSize);
		break;

	case ModuleColumn::Base:
		s.Format(L"0x%p", info->Base);
		break;

	case ModuleColumn::ImageBase:
		if (info->Type == WinSys::MapType::Image)
			s.Format(L"0x%p", info->ImageBase);
		else
			s = L"N/A";
		break;

	case ModuleColumn::Characteristics:
		s = info->Type == WinSys::MapType::Image ? CharacteristicsToString(info->Characteristics) : CString();
		break;

	case ModuleColumn::Path:
		s = info->Path.c_str();
		break;

	case ModuleColumn::FileDescription:
		if (info->Type == WinSys::MapType::Image) {
			s = GetFileDescription(info->Path).c_str();
		}
		break;
	}
	return s.GetLength();
}

CProcessModuleTable::ModuleInfoEx& CProcessModuleTable::GetModuleEx(WinSys::ModuleInfo* mi) {
	auto it = m_ModulesEx.find(mi);
	if (it != m_ModulesEx.end())
		return it->second;

	ModuleInfoEx mx;
	m_ModulesEx.insert({ mi,mx });
	return GetModuleEx(mi);
}


void CProcessModuleTable::Refresh() {
	if (!m_Tracker.IsRunning()) {
		AtlMessageBox(*this, L"Process has terminated", IDS_TITLE, MB_OK | MB_ICONWARNING);
		EndDialog(_hDlg, 0);
		return;
	}
	auto first = m_Table.data.info.empty();
	m_Tracker.EnumModules();
	if (first) {
		m_Table.data.info = m_Tracker.GetModules();
	}
	else {
		auto count = static_cast<int>(m_Table.data.info.size());
		for (int i = 0; i < count; i++) {
			auto& mi = m_Table.data.info[i];
			auto& mx = GetModuleEx(mi.get());
			if (mx.IsUnloaded && ::GetTickCount64() > mx.TargetTime) {
				ATLTRACE(L"Module unload end: %u %s (0x%p)\n", ::GetTickCount(), mi->Name.c_str(), mi->Base);
				m_ModulesEx.erase(mi.get());
				m_Table.data.info.erase(m_Table.data.info.begin() + i);
				i--;
				count--;
			}
			else if (mx.IsNew && !mx.IsUnloaded && ::GetTickCount64() > mx.TargetTime) {
				mx.IsNew = false;
			}
		}

		for (auto& mi : m_Tracker.GetNewModules()) {
			m_Table.data.info.push_back(mi);
			auto& mx = GetModuleEx(mi.get());
			mx.IsNew = true;
			mx.TargetTime = ::GetTickCount64() + 2000;
		}
		for (auto& mi : m_Tracker.GetUnloadedModules()) {
			ATLTRACE(L"Module unload start: %u %s (0x%p)\n", ::GetTickCount(), mi->Name.c_str(), mi->Base);
			auto& mx = GetModuleEx(mi.get());
			ATLASSERT(!mx.IsUnloaded);
			mx.IsNew = false;
			mx.IsUnloaded = true;
			mx.TargetTime = ::GetTickCount64() + 2000;
		}
	}

	auto count = static_cast<int>(m_Table.data.info.size());
	m_Table.data.n = count;

	return;
}

std::wstring CProcessModuleTable::GetFileDescription(std::wstring path) {
	BYTE buffer[1 << 12];
	WCHAR* fileDescription = nullptr;
	CString filePath = path.c_str();
	if (filePath.Mid(1, 2) == "??")
		filePath = filePath.Mid(4);
	if (::GetFileVersionInfo(filePath, 0, sizeof(buffer), buffer)) {
		WORD* langAndCodePage;
		UINT len;
		if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
			CString text;
			text.Format(L"\\StringFileInfo\\%04x%04x\\FileDescription", langAndCodePage[0], langAndCodePage[1]);

			if (::VerQueryValue(buffer, text, (void**)&fileDescription, &len))
				return fileDescription;
		}
	}
	return L"";
}

bool CProcessModuleTable::CompareItems(const std::shared_ptr<WinSys::ModuleInfo>& p1, const std::shared_ptr<WinSys::ModuleInfo>& p2, int col, bool asc) {
	switch (static_cast<ModuleColumn>(col)) {
	case ModuleColumn::Base: return SortHelper::SortNumbers(p1->Base, p2->Base, asc);
	case ModuleColumn::ImageBase: return SortHelper::SortNumbers(p1->ImageBase, p2->ImageBase, asc);
	case ModuleColumn::ModuleName: return SortHelper::SortStrings(p1->Name, p2->Name, asc);
	case ModuleColumn::ModuleSize: return SortHelper::SortNumbers(p1->ModuleSize, p2->ModuleSize, asc);
	}

	return false;
}