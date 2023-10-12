#include "stdafx.h"
#include "ObjectCallbackTable.h"
#include <SymbolManager.h>
#include "SymbolHelper.h"
#include "Helpers.h"
#include "resource.h"
#include "FileVersionInfoHelper.h"
#include "ClipboardHelper.h"

#define OB_OPERATION_HANDLE_CREATE              0x00000001
#define OB_OPERATION_HANDLE_DUPLICATE           0x00000002

CObjectCallbackTable::CObjectCallbackTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

LRESULT CObjectCallbackTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CObjectCallbackTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {

	return 0;
}
LRESULT CObjectCallbackTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CObjectCallbackTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CObjectCallbackTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CObjectCallbackTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CObjectCallbackTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CObjectCallbackTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CObjectCallbackTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CObjectCallbackTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(3);
	POINT pt;
	::GetCursorPos(&pt);
	int selected = m_Table.data.selected;
	if (selected < 0) {
		return FALSE;
	}
	ATLASSERT(selected >= 0);

	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
}
LRESULT CObjectCallbackTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CObjectCallbackTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

PCWSTR CObjectCallbackTable::TypeToString(ObjectCallbackType type) {
	switch (type)
	{
		case ObjectCallbackType::Process:
			return L"Process";
		case ObjectCallbackType::Thread:
			return L"Thread";
		case ObjectCallbackType::Desktop:
			return L"Desktop";
		default:
			break;
	}

	return L"";
}

CString CObjectCallbackTable::DecodeOperations(ULONG operations) {
	CString result;
	if ((operations & OB_OPERATION_HANDLE_CREATE) == OB_OPERATION_HANDLE_CREATE) {
		result += L"HANDLE_CREATE" + CString(L" | ");
	}

	if ((operations & OB_OPERATION_HANDLE_DUPLICATE) == OB_OPERATION_HANDLE_DUPLICATE) {
		result += L"HANDLE_DUPLICATE";
	}

	return result;
}

int CObjectCallbackTable::ParseTableEntry(CString& s, char& mask, int& select, ObjectCallbackInfo& info, int column) {

	switch (static_cast<ObCallbackColumn>(column))
	{
		case ObCallbackColumn::RegisterarionHandle:
			s.Format(L"0x%p", info.RegistrationHandle);
			break;
		case ObCallbackColumn::CallbackEntry:
			s.Format(L"0x%p", info.CallbackEntry);
			break;
		case ObCallbackColumn::Type:
			s = TypeToString(info.Type);
			break;
		case ObCallbackColumn::Enabled:
			s = info.Enabled ? L"Enabled" : L"Disabled";
			break;
		case ObCallbackColumn::PreOperation:
		{
			auto& symbols = SymbolManager::Get();
			DWORD64 offset = 0;
			auto symbol = symbols.GetSymbolFromAddress(0, (DWORD64)info.PreOperation, &offset);
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
				s.Format(L"0x%p (%s)", info.PreOperation, wdetails.c_str());
			}
			else
				s.Format(L"0x%p", info.PreOperation);
			break;
		}
		case ObCallbackColumn::PostOperation:
		{
			auto& symbols = SymbolManager::Get();
			DWORD64 offset = 0;
			auto symbol = symbols.GetSymbolFromAddress(0, (DWORD64)info.PostOperation, &offset);
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
				s.Format(L"0x%p (%s)", info.PostOperation, wdetails.c_str());
			}
			else
				s.Format(L"0x%p", info.PostOperation);
			break;
		}
			
		case ObCallbackColumn::Opeartions:
			s = DecodeOperations(info.Operations);
			break;
		case ObCallbackColumn::Company:
			s = info.Company.c_str();
			break;
		case ObCallbackColumn::ModuleName:
			s = Helpers::StringToWstring(info.Module).c_str();
			break;
	}

	return s.GetLength();
}

bool CObjectCallbackTable::CompareItems(const ObjectCallbackInfo& s1, const ObjectCallbackInfo& s2, int col, bool asc) {
	switch (static_cast<ObCallbackColumn>(col))
	{
		case ObCallbackColumn::Company:
			return SortHelper::SortStrings(s1.Company, s2.Company, asc);
		default:
			break;
	}
	return false;
}

void CObjectCallbackTable::Refresh() {
#ifdef _WIN64
	static ULONG Max = 64;
#else
	static ULONG Max = 8;
#endif
	m_Table.data.n = 0;
	m_Table.data.info.clear();

	ULONG offset = SymbolHelper::GetKernelStructMemberOffset("_OBJECT_TYPE", "CallbackList");
	LONG count = DriverHelper::GetObCallbackCount(offset);
	if (count > 0) {
		ULONG size = Max * sizeof(ObCallbackInfo);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));

		ObCallbackInfo* p = (ObCallbackInfo*)buffer.get();
		if (p != nullptr) {
			DriverHelper::EnumObCallbackNotify(offset, p, size);
			for (ULONG i = 0; i < count; i++) {
				ObjectCallbackInfo info;
				info.CallbackEntry = p[i].CallbackEntry;
				info.Operations = p[i].Operations;
				info.Enabled = p[i].Enabled;
				info.RegistrationHandle = p[i].RegistrationHandle;
				info.PreOperation = p[i].PreOperation;
				info.PostOperation = p[i].PostOperation;
				info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.PreOperation);
				std::wstring path = Helpers::StringToWstring(info.Module);
				info.Company = FileVersionInfoHelpers::GetCompanyName(path);
				info.Type = p[i].Type;
				m_Table.data.info.push_back(std::move(info));
			}
		}
	}


	m_Table.data.n = m_Table.data.info.size();
}



LRESULT CObjectCallbackTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	Invalidate(True);
	return TRUE;
}

LRESULT CObjectCallbackTable::OnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& p = m_Table.data.info[selected];

	CString text;
	text.Format(L"Remove callback：%p?", p.CallbackEntry);
	if (AtlMessageBox(*this, (PCWSTR)text, IDS_TITLE, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL)
		return 0;



	BOOL ok = false;
	ok = DriverHelper::RemoveObCallback((ULONG_PTR)p.CallbackEntry);
	if (!ok)
		AtlMessageBox(*this, L"Failed to remove callback", IDS_TITLE, MB_ICONERROR);
	else
		Refresh();

	Invalidate(True);
	return TRUE;
}

LRESULT CObjectCallbackTable::OnRemoveByCompanyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& pCallback = m_Table.data.info[selected];

	std::wstring companyName = pCallback.Company;

	for (auto& p : m_Table.data.info) {
		if (p.Company != companyName) {
			continue;
		}


		BOOL ok = false;
		ok = DriverHelper::RemoveObCallback((ULONG_PTR)p.CallbackEntry);
		if (!ok)
			AtlMessageBox(*this, L"Failed to remove callback", IDS_TITLE, MB_ICONERROR);
		else {
			Refresh();
		}
		Invalidate(True);
	}

	return TRUE;
}


std::wstring CObjectCallbackTable::GetSingleInfo(ObjectCallbackInfo& info) {
	CString text;
	CString s;

	s.Format(L"0x%p", info.CallbackEntry);
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.RegistrationHandle);
	s += L"\t";
	text += s;

	s = TypeToString(info.Type);
	s += L"\t";
	text += s;

	s = info.Enabled ? L"Enabled" : L"Disabled";
	s += L"\t";
	text += s;


	s.Format(L"0x%p", info.PreOperation);
	s += L"\t";
	text += s;

	s.Format(L"0x%p", info.PostOperation);
	s += L"\t";
	text += s;

	s = DecodeOperations(info.Operations);
	s += L"\t";
	text += s;

	s = info.Company.c_str();
	s += L"\t";
	text += s;

	s = Helpers::StringToWstring(info.Module).c_str();
	s += L"\t";
	text += s;
	text += L"\r\n";

	return text.GetString();
}

LRESULT CObjectCallbackTable::OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	std::wstring text = GetSingleInfo(info);
	ClipboardHelper::CopyText(m_hWnd, text.c_str());
	return 0;
}


LRESULT CObjectCallbackTable::OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

LRESULT CObjectCallbackTable::OnDisable(WORD, WORD, HWND, BOOL&) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& p = m_Table.data.info[selected];

	CString text;
	text.Format(L"Disable callback：%p ?", p.CallbackEntry);
	if (AtlMessageBox(*this, (PCWSTR)text, IDS_TITLE, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL)
		return 0;

	BOOL ok = false;
	ok = DriverHelper::DisableObCallback((ULONG_PTR)p.CallbackEntry);
	if (!ok)
		AtlMessageBox(*this, L"Failed to disable callback", IDS_TITLE, MB_ICONERROR);
	else
		Refresh();

	Invalidate(True);

	return TRUE;
}


LRESULT CObjectCallbackTable::OnEnable(WORD, WORD, HWND, BOOL&) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& p = m_Table.data.info[selected];

	CString text;
	text.Format(L"Enable callback：%p ?", p.CallbackEntry);
	if (AtlMessageBox(*this, (PCWSTR)text, IDS_TITLE, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL)
		return 0;

	BOOL ok = false;
	ok = DriverHelper::EnableObCallback((ULONG_PTR)p.CallbackEntry);
	if (!ok)
		AtlMessageBox(*this, L"Failed to enable callback", IDS_TITLE, MB_ICONERROR);
	else
		Refresh();

	Invalidate(True);

	return TRUE;
}