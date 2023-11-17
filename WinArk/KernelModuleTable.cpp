#include "stdafx.h"
#include "KernelModuleTracker.h"
#include "KernelModuleTable.h"
#include <Helpers.h>
#include "ClipboardHelper.h"
#include "DriverHelper.h"
#include "KernelEATHookDlg.h"
#include "KernelInlineHookDlg.h"


LRESULT CKernelModuleTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CKernelModuleTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT CKernelModuleTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelModuleTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelModuleTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CKernelModuleTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_CONTEXT);
	hSubMenu = menu.GetSubMenu(10);
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

LRESULT CKernelModuleTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelModuleTable::OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return DLGC_WANTARROWS;
}

void CKernelModuleTable::DoRefresh() {
	m_Table.data.info.clear();
	auto count = m_Tracker.EnumModules();
	m_Table.data.info = m_Tracker.GetModules();
	m_Table.data.n = m_Table.data.info.size();
}

CKernelModuleTable::CKernelModuleTable(BarInfo& bars, TableInfo& table)
	: CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	DoRefresh();
}

int CKernelModuleTable::ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::KernelModuleInfo>& info, int column) {
	switch (column) {
		case 0:
			s = info->Name.c_str();
			break;
		case 1:
			s.Format(L"0x%p", info->ImageBase);
			break;
		case 2:
			s.Format(L"0x%X", info->ImageSize);
			break;
		case 3:
			s.Format(L"%u", info->LoadOrderIndex);
			break;
		case 4:
		{
			std::wstring path = Helpers::StringToWstring(info->FullPath);
			s = GetCompanyName(path).c_str();
			if (s.Find(L"Microsoft") == -1 && s.Find(L"YuanOS") == -1) {
				select |= DRAW_HILITE;
			}
			break;
		}
		case 5:
			s = info->FullPath.c_str();
			break;
		default:
			break;
	}
	return s.GetLength();
}

bool CKernelModuleTable::CompareItems(const std::shared_ptr<WinSys::KernelModuleInfo>& p1, const std::shared_ptr<WinSys::KernelModuleInfo>& p2, int col, bool asc) {
	switch (static_cast<KernelModuleColumn>(col)) {
		case KernelModuleColumn::Name: return SortHelper::SortStrings(p1->Name, p2->Name, asc);
		case KernelModuleColumn::ImageBase:return SortHelper::SortNumbers(p1->ImageBase, p2->ImageBase, asc);
		case KernelModuleColumn::ImageSize:return SortHelper::SortNumbers(p1->ImageSize, p2->ImageSize, asc);
		case KernelModuleColumn::LoadOrderIndex: return SortHelper::SortNumbers(p1->LoadOrderIndex, p2->LoadOrderIndex, asc);
		case KernelModuleColumn::FullPath: return SortHelper::SortStrings(p1->FullPath, p2->FullPath, asc);
		case KernelModuleColumn::CommpanyName: 
		{
			std::wstring path = Helpers::StringToWstring(p1->FullPath);
			std::wstring name1 = GetCompanyName(path);
			path = Helpers::StringToWstring(p2->FullPath);
			std::wstring name2 = GetCompanyName(path);
			return SortHelper::SortStrings(name1, name2, asc);
		}
	}
	return false;
}

std::wstring CKernelModuleTable::GetCompanyName(std::wstring path) {
	BYTE buffer[1 << 12];
	WCHAR* companyName = nullptr;
	CString filePath = path.c_str();
	if (filePath.Mid(1, 2) == "??")
		filePath = filePath.Mid(4);
	if (::GetFileVersionInfo(filePath, 0, sizeof(buffer), buffer)) {
		WORD* langAndCodePage;
		UINT len;
		if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
			CString text;
			text.Format(L"\\StringFileInfo\\%04x%04x\\CompanyName", langAndCodePage[0], langAndCodePage[1]);

			if (::VerQueryValue(buffer, text, (void**)&companyName, &len))
				return companyName;
		}
	}
	return L"";
}

LRESULT CKernelModuleTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	DoRefresh();
	return 0;
}

LRESULT CKernelModuleTable::OnKernelModuleCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	CString text = GetSingleKernelModuleInfo(info).c_str();

	ClipboardHelper::CopyText(m_hWnd, text);
	return 0;
}

LRESULT CKernelModuleTable::OnKernelModuleExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleKernelModuleInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CKernelModuleTable::GetSingleKernelModuleInfo(std::shared_ptr<WinSys::KernelModuleInfo>& info) {
	CString text;
	CString s;

	s = info->Name.c_str();
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info->ImageBase);
	s += L"\t";
	text += s;

	s.Format(L"0x%X", info->ImageSize);
	s += L"\t";
	text += s;

	s.Format(L"%u", info->LoadOrderIndex);
	s += L"\t";
	text += s;

	s = GetCompanyName(Helpers::StringToWstring(info->FullPath)).c_str();
	s += L"\t";
	text += s;

	s = info->FullPath.c_str();
	s += L"\t";
	text += s;

	text += L"\r\n";

	return text.GetString();
}

LRESULT CKernelModuleTable::OnGoToFileLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& kernelModule = m_Table.data.info[selected];
	std::wstring path = Helpers::StringToWstring(kernelModule->FullPath);
	if ((INT_PTR)::ShellExecute(nullptr, L"open", L"explorer",
		(L"/select,\"" + path + L"\"").c_str(),
		nullptr, SW_SHOWDEFAULT) < 32)
		AtlMessageBox(*this, L"Failed to locate executable", IDS_TITLE, MB_ICONERROR);

	return 0;
}

LRESULT CKernelModuleTable::OnKernelDump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& kernelModule = m_Table.data.info[selected];
	std::wstring path = Helpers::StringToWstring(kernelModule->NtPath);
	bool ok = DriverHelper::DumpSysModule(path.c_str(), kernelModule->ImageBase, kernelModule->ImageSize);
	AtlMessageBox(*this, ok ? L"Dump success :)!" : L"Dump failed :(!", IDS_TITLE, MB_ICONINFORMATION);
	return 0;
}

LRESULT CKernelModuleTable::OnKernelEATHookScan(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& kernelModule = m_Table.data.info[selected];

	CKernelEATHookDlg dlg(kernelModule);
	dlg.DoModal(m_hWnd);

	return 0;
}

LRESULT CKernelModuleTable::OnKernelInlineHookScan(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& kernelModule = m_Table.data.info[selected];

	CKernelInlineHookDlg dlg(kernelModule);
	dlg.DoModal(m_hWnd);

	return 0;
}